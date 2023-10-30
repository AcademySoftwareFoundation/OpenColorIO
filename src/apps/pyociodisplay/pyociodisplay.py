# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# TODO: Much of the OpenGL code in this module is adapted from the
#       oglapphelpers library bundled with OCIO. We should fully
#       reimplement that in Python for direct use in applications.

from contextlib import contextmanager
from functools import partial
import ctypes
import logging
import sys

from PySide6 import QtCore, QtGui, QtWidgets, QtOpenGL
from OpenGL import GL
import PyOpenColorIO as ocio
import OpenImageIO as oiio
import numpy as np


logger = logging.getLogger(__name__)


GLSL_VERT_SRC = """#version 400 core

uniform mat4 mvpMat;
in vec3 in_position;
in vec2 in_texCoord;

out vec2 vert_texCoord;

void main() {
    vert_texCoord = in_texCoord;
    gl_Position = mvpMat * vec4(in_position, 1.0);
}

"""
"""
Simple vertex shader which transforms all vertices with a 
model-view-projection matrix uniform.
"""

GLSL_FRAG_SRC = """#version 400 core

uniform sampler2D imageTex;
in vec2 vert_texCoord;

out vec4 frag_color;

void main() {{
    frag_color = texture(imageTex, vert_texCoord);
}}
"""
"""
Simple fragment shader which performs a 2D texture lookup to map an 
image texture onto UVs. This is used when OCIO is unavailable, like 
before its shader initialization.
"""

GLSL_FRAG_OCIO_SRC_FMT = """#version 400 core

uniform sampler2D imageTex;
in vec2 vert_texCoord;

out vec4 frag_color;

{ocio_src}

void main() {{
    vec4 inColor = texture(imageTex, vert_texCoord);
    vec4 outColor = OCIOMain(inColor);
    frag_color = outColor;
}}
"""
"""
Fragment shader which performs a 2D texture lookup to map an image 
texture onto UVs and processes fragments through an OCIO-provided 
shader program segment, which itself utilizes additional texture 
lookups, dynamic property uniforms, and various native GLSL op 
implementations. Note that this shader's cost will increase with 
additional LUTs in an OCIO processor, since each adds its own 
2D or 3D texture.
"""


class ImagePlane(QtOpenGL.QGLWidget):
    """
    Qt-wrapped OpenGL window for drawing with PyOpenGL.
    """

    def __init__(self, parent=None):
        super(ImagePlane, self).__init__(parent)

        # Clicking on/tabbing to widget restores focus
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        # Set to True after initializeGL is called. Don't allow grabbing
        # OpenGL context until that point.
        self._gl_ready = False

        # Color management
        self._ocio_input_cs = None
        self._ocio_display = None
        self._ocio_view = None
        self._ocio_exposure = 0.0
        self._ocio_gamma = 1.0
        self._ocio_channel_hot = [1, 1, 1, 1]
        self._ocio_proc_cache_id = None
        self._ocio_shader_cache_id = None
        self._ocio_shader_desc = None
        self._ocio_tex_start_index = 1  # Start after image_tex
        self._ocio_tex_ids = []
        self._ocio_uniform_ids = {}

        # MVP matrix components
        self._model_view_mat = imath.M44f()
        self._proj_mat = imath.M44f()

        # Image texture
        self._image_tex = None
        self._image_center = imath.V2f(0.0, 0.0)
        self._image_pos = imath.V2f(0.0, 0.0)
        self._image_size = imath.V2f(1.0, 1.0)

        # Image plane VAO
        self._plane_vao = None
        self._plane_position_vbo = None
        self._plane_tex_coord_vbo = None
        self._plane_index_vbo = None

        # GLSL shader program
        self._vert_shader = None
        self._frag_shader = None
        self._shader_program = None

        # Setup keyboard shortcuts
        self._install_shortcuts()

    def initializeGL(self):
        """
        Set up OpenGL resources and state (called once).
        """
        self._gl_ready = True

        # Init image texture
        self._image_tex = GL.glGenTextures(1)
        GL.glActiveTexture(GL.GL_TEXTURE0)
        GL.glBindTexture(GL.GL_TEXTURE_2D, self._image_tex)
        GL.glTexImage2D(
            GL.GL_TEXTURE_2D,
            0,
            GL.GL_RGBA32F,
            self._image_size.x,
            self._image_size.y,
            0,
            GL.GL_RGBA,
            GL.GL_FLOAT,
            ctypes.c_void_p(0)
        )

        GL.glTexParameteri(
            GL.GL_TEXTURE_2D,
            GL.GL_TEXTURE_WRAP_S,
            GL.GL_CLAMP_TO_EDGE
        )
        GL.glTexParameteri(
            GL.GL_TEXTURE_2D,
            GL.GL_TEXTURE_WRAP_T,
            GL.GL_CLAMP_TO_EDGE
        )
        self._set_ocio_tex_params(GL.GL_TEXTURE_2D, ocio.INTERP_LINEAR)

        # Init image plane
        plane_position_data = np.array(
            [
                -0.5,  0.5, 0.0,  # top-left
                 0.5,  0.5, 0.0,  # top-right
                 0.5, -0.5, 0.0,  # bottom-right
                -0.5, -0.5, 0.0   # bottom-left
            ],
            dtype=np.float32
        )

        plane_tex_coord_data = np.array(
            [
                0.0, 1.0,  # top-left
                1.0, 1.0,  # top-right
                1.0, 0.0,  # bottom-right
                0.0, 0.0   # bottom-left
            ],
            dtype=np.float32
        )

        plane_index_data = np.array(
            [
                0, 1, 2,  # triangle: top-left
                0, 2, 3   # triangle: bottom-right
            ],
            dtype=np.uint32
        )

        self._plane_vao = GL.glGenVertexArrays(1)
        GL.glBindVertexArray(self._plane_vao)

        (self._plane_position_vbo,
         self._plane_tex_coord_vbo,
         self._plane_index_vbo) = GL.glGenBuffers(3)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self._plane_position_vbo)
        GL.glBufferData(
            GL.GL_ARRAY_BUFFER,
            plane_position_data.nbytes,
            plane_position_data,
            GL.GL_STATIC_DRAW
        )
        GL.glVertexAttribPointer(
            0,
            3,
            GL.GL_FLOAT,
            GL.GL_FALSE,
            0,
            ctypes.c_void_p(0)
        )
        GL.glEnableVertexAttribArray(0)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self._plane_tex_coord_vbo)
        GL.glBufferData(
            GL.GL_ARRAY_BUFFER,
            plane_tex_coord_data.nbytes,
            plane_tex_coord_data,
            GL.GL_STATIC_DRAW
        )
        GL.glVertexAttribPointer(
            1,
            2,
            GL.GL_FLOAT,
            GL.GL_FALSE,
            0,
            ctypes.c_void_p(0)
        )
        GL.glEnableVertexAttribArray(1)

        GL.glBindBuffer(GL.GL_ELEMENT_ARRAY_BUFFER, self._plane_index_vbo)
        GL.glBufferData(
            GL.GL_ELEMENT_ARRAY_BUFFER,
            plane_index_data.nbytes,
            plane_index_data,
            GL.GL_STATIC_DRAW
        )

        self._build_program()

    def resizeGL(self, w, h):
        """
        Called whenever the widget is resized.

        :param w: Window width
        :param h: Window height
        """
        GL.glViewport(0, 0, w, h)

        # Center image plane
        l = -w / 2.0
        r =  w / 2.0
        b = -h / 2.0
        t =  h / 2.0
        n = -1.0
        f =  1.0

        # Orthographic projection:
        #   https://en.wikipedia.org/wiki/Orthographic_projection#Geometry
        self._proj_mat[0][0] =  2.0 / (r-l)
        self._proj_mat[1][1] =  2.0 / (t-b)
        self._proj_mat[2][2] = -2.0 / (f-n)
        self._proj_mat[0][3] = -(r+l) / (r-l)
        self._proj_mat[1][3] = -(t+b) / (t-b)
        self._proj_mat[2][3] = -(f+n) / (f-n)

        self._update_model_view_mat()

    def paintGL(self):
        """
        Called whenever a repaint is needed. Calling ``update()`` will
        schedule a repaint.
        """
        GL.glClearColor(0.0, 0.0, 0.0, 1.0)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT)

        if self._shader_program is not None:
            GL.glUseProgram(self._shader_program)

            self._use_ocio_tex()
            self._use_ocio_uniforms()

            # Set uniforms
            mvp_mat = self._proj_mat * self._model_view_mat
            mvp_mat_loc = GL.glGetUniformLocation(
                self._shader_program,
                "mvpMat"
            )
            GL.glUniformMatrix4fv(
                mvp_mat_loc,
                1,
                GL.GL_FALSE,
                self._m44f_to_ndarray(mvp_mat)
            )

            image_tex_loc = GL.glGetUniformLocation(
                self._shader_program,
                "imageTex"
            )
            GL.glUniform1i(image_tex_loc, 0)

            # Bind texture, VAO, and draw
            GL.glActiveTexture(GL.GL_TEXTURE0 + 0)
            GL.glBindTexture(GL.GL_TEXTURE_2D, self._image_tex)

            GL.glBindVertexArray(self._plane_vao)

            GL.glDrawElements(
                GL.GL_TRIANGLES,
                6,
                GL.GL_UNSIGNED_INT,
                ctypes.c_void_p(0)
            )

            GL.glBindVertexArray(0)

    def load_image(self, image_path):
        """
        Load an image into the image plane texture.

        :param str image_path: Image file path
        :return: Input color space name
        :rtype: str
        """
        config = ocio.GetCurrentConfig()

        # Get input color space (file rule)
        cs_name, rule_idx = config.getColorSpaceFromFilepath(image_path)
        if not cs_name:
            # Use previous or config default
            if self._ocio_input_cs:
                cs_name = self._ocio_input_cs
            else:
                cs_name = ocio.ROLE_DEFAULT
        self._ocio_input_cs = cs_name

        # Get default view for input color space (viewing rule)
        self._ocio_view = config.getDefaultView(
            self._ocio_display,
            self._ocio_input_cs
        )

        buf = oiio.ImageBuf(image_path)
        spec = buf.spec()

        # Convert to RGBA, filling missing color channels with 0.0, and a
        # missing alpha with 1.0.
        if spec.nchannels < 4:
            buf = oiio.ImageBufAlgo.channels(
                buf,
                tuple(
                    list(range(spec.nchannels))
                    + ([0.0] * (4 - spec.nchannels - 1))
                    + [1.0]
                ),
                newchannelnames=("R", "G", "B", "A")
            )
        elif spec.nchannels > 4:
            buf = oiio.ImageBufAlgo.channels(
                buf,
                (0, 1, 2, 3),
                newchannelnames=("R", "G", "B", "A")
            )

        # Get pixels as 32-bit float NumPy array
        data = buf.get_pixels(oiio.FLOAT)

        # Stash image size for pan/zoom calculations
        self._image_pos.x = spec.x
        self._image_pos.y = spec.y
        self._image_size.x = spec.width
        self._image_size.y = spec.height

        # Load image data into texture
        self.makeCurrent()

        GL.glBindTexture(GL.GL_TEXTURE_2D, self._image_tex)
        GL.glTexImage2D(
            GL.GL_TEXTURE_2D,
            0,
            GL.GL_RGBA32F,
            spec.width,
            spec.height,
            0,
            GL.GL_RGBA,
            GL.GL_FLOAT,
            data.ravel()
        )

        self._update_model_view_mat(update=False)

        self.update_ocio_proc(
            input_cs=self._ocio_input_cs,
            view=self._ocio_view
        )

    def input_colorspace(self):
        """
        :return: Current input color space name, which could have been
            changed by the last loaded image defaults.
        :rtype: str
        """
        return self._ocio_input_cs

    def display(self):
        """
        :return: Current OCIO display
        :rtype: str
        """
        return self._ocio_display

    def view(self):
        """
        :return: Current OCIO view, which could have been changed by
            the last loaded image defaults.
        :rtype: str
        """
        return self._ocio_view

    def update_ocio_proc(
        self,
        input_cs=None,
        display=None,
        view=None,
        channel=None
    ):
        """
        Update one or more aspects of the OCIO GPU renderer. Parameters
        are cached so not providing a parameter maintains the existing
        state. This will trigger a GL update IF the underlying OCIO ops
        in the processor have changed.

        :param str input_cs: Override the input color space name (how
            the image texture should be interpreted by OCIO). If this
            is wrong, the rendered results will look wrong in most
            cases.
        :param str display: OCIO display representing the current
            display device colors are being viewed on.
        :param str view: OCIO view defining the output transform for
            the given display.
        :param int channel: ImagePlaneChannels value to toggle channel
            isolation.
        """
        if input_cs:
            self._ocio_input_cs = input_cs
        if display:
            self._ocio_display = display
        if view:
            self._ocio_view = view
        if channel is not None:
            self._update_ocio_channel_hot(channel)

        # Can a processor be made?
        if not all((self._ocio_input_cs, self._ocio_display, self._ocio_view)):
            return

        config = ocio.GetCurrentConfig()

        # Build transforms needed for viewing pipeline
        exposure_tr = ocio.ExposureContrastTransform(
            exposure=self._ocio_exposure,
            dynamicExposure=True
        )
        channel_view_tr = ocio.MatrixTransform.View(
            channelHot=self._ocio_channel_hot,
            lumaCoef=config.getDefaultLumaCoefs()
        )
        display_view_tr = ocio.DisplayViewTransform(
            src=self._ocio_input_cs,
            display=self._ocio_display,
            view=self._ocio_view
        )
        gamma_tr = ocio.ExposureContrastTransform(
            gamma=self._ocio_gamma,
            pivot=1.0,
            dynamicGamma=True
        )

        # Mimic OCIO v1 DisplayTransform for consistency with DCCs
        viewing_pipeline = ocio.LegacyViewingPipeline()
        viewing_pipeline.setLinearCC(exposure_tr)
        viewing_pipeline.setChannelView(channel_view_tr)
        viewing_pipeline.setDisplayViewTransform(display_view_tr)
        viewing_pipeline.setDisplayCC(gamma_tr)

        # Create processor
        proc = viewing_pipeline.getProcessor(config)

        if proc.getCacheID() != self._ocio_proc_cache_id:
            self._ocio_shader_desc = ocio.GpuShaderDesc.CreateShaderDesc(
                language=ocio.GPU_LANGUAGE_GLSL_4_0
            )
            self._ocio_proc_cache_id = proc.getCacheID()
            ocio_gpu_proc = proc.getDefaultGPUProcessor()
            ocio_gpu_proc.extractGpuShaderInfo(self._ocio_shader_desc)

            self._allocate_ocio_tex()
            self._build_program()

            # Set initial dynamic property state
            self._update_ocio_dyn_prop(
                ocio.DYNAMIC_PROPERTY_EXPOSURE,
                self._ocio_exposure
            )
            self._update_ocio_dyn_prop(
                ocio.DYNAMIC_PROPERTY_GAMMA,
                self._ocio_gamma
            )

            self.update()

    def exposure(self):
        """
        :return: Last set exposure dynamic property value
        :rtype: float
        """
        return self._ocio_exposure

    def update_exposure(self, value):
        """
        Update OCIO GPU renderer exposure. This is a dynamic property,
        implemented as a GLSL uniform, so can be updated without
        modifying the OCIO shader program or its dependencies.

        :param float value: Exposure value in stops
        """
        self._ocio_exposure = value
        self._update_ocio_dyn_prop(ocio.DYNAMIC_PROPERTY_EXPOSURE, value)
        self.update()

    def gamma(self):
        """
        :return: Last set gamma dynamic property value
        :rtype: float
        """
        return self._ocio_gamma

    def update_gamma(self, value):
        """
        Update OCIO GPU renderer gamma. This is a dynamic property,
        implemented as a GLSL uniform, so can be updated without
        modifying the OCIO shader program or its dependencies.

        .. note::
            Value is floor clamped at 0.001 to prevent zero division
            errors.

        :param float value: Gamma value used like: pow(rgb, 1/gamma)
        """
        # Translate gamma to exponent, enforcing floor
        value = 1.0 / max(0.001, value)

        self._ocio_gamma = value
        self._update_ocio_dyn_prop(ocio.DYNAMIC_PROPERTY_GAMMA, value)
        self.update()

    def _install_shortcuts(self):
        """
        Setup supported keyboard shortcuts.
        """
        # R,G,B,A = view channel
        # C = view color
        for i, key in enumerate(("R", "G", "B", "A", "C")):
            channel_shortcut = QtWidgets.QShortcut(
                QtGui.QKeySequence(key),
                self
            )
            channel_shortcut.activated.connect(
                partial(self.update_ocio_proc, channel=i)
            )

    def _compile_shader(self, glsl_src, shader_type):
        """
        Compile GLSL shader and return its object ID.

        :param str glsl_src: Shader source code
        :param GLenum shader_type: Type of shader to be created, which is a
            enum adhering to the formatting ``GL_*_SHADER``.
        :return: Shader object ID, or None if shader compilation fails
        :rtype: GLuint or None
        """
        shader = GL.glCreateShader(shader_type)
        GL.glShaderSource(shader, glsl_src)
        GL.glCompileShader(shader)

        compile_status = GL.glGetShaderiv(shader, GL.GL_COMPILE_STATUS)
        if not compile_status:
            compile_log = GL.glGetShaderInfoLog(shader)
            logger.error(
                "Shader program compile error: {log}".format(log=compile_log)
            )
            return None

        return shader

    def _build_program(self, force=False):
        """
        This builds the initial shader program, and rebuilds its
        fragment shader whenever the OCIO GPU renderer changes.

        :param bool force: Whether to force a rebuild even if the OCIO
            shader cache ID has not changed.
        """
        if not self._gl_ready:
            return

        self.makeCurrent()

        # If new shader cache ID matches previous cache ID existing program
        # can be reused.
        shader_cache_id = self._ocio_shader_cache_id
        if self._ocio_shader_desc and not force:
            shader_cache_id = self._ocio_shader_desc.getCacheID()
            if self._ocio_shader_cache_id == shader_cache_id:
                return

        # Init shader program
        if not self._shader_program:
            self._shader_program = GL.glCreateProgram()

        # Vert shader only needs to be built once
        if not self._vert_shader:
            self._vert_shader = self._compile_shader(
                GLSL_VERT_SRC,
                GL.GL_VERTEX_SHADER
            )
            if not self._vert_shader:
                return

            GL.glAttachShader(self._shader_program, self._vert_shader)

        # Frag shader needs recompile each build (for OCIO changes)
        if self._frag_shader:
            GL.glDetachShader(self._shader_program, self._frag_shader)
            GL.glDeleteShader(self._frag_shader)

        frag_src = GLSL_FRAG_SRC
        if self._ocio_shader_desc:
            # Inject OCIO shader block
            frag_src = GLSL_FRAG_OCIO_SRC_FMT.format(
                ocio_src=self._ocio_shader_desc.getShaderText()
            )
        self._frag_shader = self._compile_shader(
            frag_src,
            GL.GL_FRAGMENT_SHADER
        )
        if not self._frag_shader:
            return

        GL.glAttachShader(self._shader_program, self._frag_shader)

        # Link program
        GL.glBindAttribLocation(self._shader_program, 0, "in_position")
        GL.glBindAttribLocation(self._shader_program, 1, "in_texCoord")

        GL.glLinkProgram(self._shader_program)
        link_status = GL.glGetProgramiv(self._shader_program, GL.GL_LINK_STATUS)
        if not link_status:
            link_log = GL.glGetProgramInfoLog(self._shader_program)
            logger.error(
                "Shader program link error: {log}".format(log=link_log)
            )
            return

        # Store cache ID to detect reuse
        self._ocio_shader_cache_id = shader_cache_id

    def _update_model_view_mat(self, update=True):
        """
        Re-calculate the model view matrix, which needs to be updated
        prior to rendering if the image or window size have changed.

        :param bool update: Optionally redraw the window
        """
        # Fit image to window
        size = self._widget_size_to_v2f(self)
        size_ratio = size / self._image_size

        if size_ratio.x < size_ratio.y:
            zoom = size_ratio.x
        else:
            zoom = size_ratio.y

        self._model_view_mat.makeIdentity()

        # Flip Y to account for different OIIO/OpenGL image origin
        self._model_view_mat.scale(imath.V3f(1.0, -1.0, 1.0))

        self._model_view_mat.scale(imath.V3f(zoom, zoom, 1.0))
        self._model_view_mat.translate(
            self._v2f_to_v3f(self._image_center / size * 2.0, 0.0)
        )
        self._model_view_mat.scale(self._v2f_to_v3f(self._image_size, 1.0))

        if update:
            self.update()

    def _set_ocio_tex_params(self, tex_type, interpolation):
        """
        Set texture parameters for an OCIO LUT texture based on its
        type and interpolation.

        :param tex_type: OpenGL texture type (GL_TEXTURE_1/2/3D)
        :param ocio.Interpolation interpolation: Interpolation enum
            value.
        """
        if interpolation == ocio.INTERP_NEAREST:
            GL.glTexParameteri(
                tex_type,
                GL.GL_TEXTURE_MIN_FILTER,
                GL.GL_NEAREST
            )
            GL.glTexParameteri(
                tex_type,
                GL.GL_TEXTURE_MAG_FILTER,
                GL.GL_NEAREST
            )
        else:
            GL.glTexParameteri(
                tex_type,
                GL.GL_TEXTURE_MIN_FILTER,
                GL.GL_LINEAR
            )
            GL.glTexParameteri(
                tex_type,
                GL.GL_TEXTURE_MAG_FILTER,
                GL.GL_LINEAR
            )

    def _allocate_ocio_tex(self):
        """
        Iterate and allocate 1/2/3D textures needed by the current
        OCIO GPU processor. 3D LUTs become 3D textures and 1D LUTs
        become 1D or 2D textures depending on their size. Since
        textures have a hardware enforced width limitation, large LUTs
        are wrapped onto multiple rows.

        .. note::
            Each time this runs, the previous set of textures are
            deleted from GPU memory first.
        """
        if not self._ocio_shader_desc:
            return

        self.makeCurrent()

        # Delete previous textures
        self._del_ocio_tex()
        self._del_ocio_uniforms()

        tex_index = self._ocio_tex_start_index

        # Process 3D textures
        for tex_info in self._ocio_shader_desc.get3DTextures():
            tex_data = tex_info.getValues()

            tex = GL.glGenTextures(1)
            GL.glActiveTexture(GL.GL_TEXTURE0 + tex_index)
            GL.glBindTexture(GL.GL_TEXTURE_3D, tex)
            self._set_ocio_tex_params(GL.GL_TEXTURE_3D, tex_info.interpolation)
            GL.glTexImage3D(
                GL.GL_TEXTURE_3D,
                0,
                GL.GL_RGB32F,
                tex_info.edgeLen,
                tex_info.edgeLen,
                tex_info.edgeLen,
                0,
                GL.GL_RGB,
                GL.GL_FLOAT,
                tex_data
            )

            self._ocio_tex_ids.append((
                tex,
                tex_info.textureName,
                tex_info.samplerName,
                GL.GL_TEXTURE_3D,
                tex_index
            ))
            tex_index += 1

        # Process 2D textures
        for tex_info in self._ocio_shader_desc.getTextures():
            tex_data = tex_info.getValues()

            internal_fmt = GL.GL_RGB32F
            fmt = GL.GL_RGB
            if tex_info.channel == self._ocio_shader_desc.TEXTURE_RED_CHANNEL:
                internal_fmt = GL.GL_R32F
                fmt = GL.GL_RED

            tex = GL.glGenTextures(1)
            GL.glActiveTexture(GL.GL_TEXTURE0 + tex_index)

            if tex_info.dimensions == self._ocio_shader_desc.TEXTURE_2D:
                tex_type = GL.GL_TEXTURE_2D
                GL.glBindTexture(tex_type, tex)
                self._set_ocio_tex_params(tex_type, tex_info.interpolation)
                GL.glTexImage2D(
                    tex_type,
                    0,
                    internal_fmt,
                    tex_info.width,
                    tex_info.height,
                    0,
                    fmt,
                    GL.GL_FLOAT,
                    tex_data
                )
            else:
                tex_type = GL.GL_TEXTURE_1D
                GL.glBindTexture(tex_type, tex)
                self._set_ocio_tex_params(tex_type, tex_info.interpolation)
                GL.glTexImage1D(
                    tex_type,
                    0,
                    internal_fmt,
                    tex_info.width,
                    0,
                    fmt,
                    GL.GL_FLOAT,
                    tex_data
                )

            self._ocio_tex_ids.append((
                tex,
                tex_info.textureName,
                tex_info.samplerName,
                tex_type,
                tex_index
            ))
            tex_index += 1

    def _del_ocio_tex(self):
        """
        Delete all OCIO textures from the GPU.
        """
        for tex, tex_name, sampler_name, tex_type, tex_index \
                in self._ocio_tex_ids:
            GL.glDeleteTextures([tex])
        del self._ocio_tex_ids[:]

    def _use_ocio_tex(self):
        """
        Bind all OCIO textures to the shader program.
        """
        for tex, tex_name, sampler_name, tex_type, tex_index \
                in self._ocio_tex_ids:
            GL.glActiveTexture(GL.GL_TEXTURE0 + tex_index)
            GL.glBindTexture(tex_type, tex)
            GL.glUniform1i(
                GL.glGetUniformLocation(self._shader_program, sampler_name),
                tex_index
            )

    def _del_ocio_uniforms(self):
        """
        Forget about the dynamic property uniforms needed for the
        previous OCIO shader build.
        """
        self._ocio_uniform_ids.clear()

    def _use_ocio_uniforms(self):
        """
        Bind and/or update dynamic property uniforms needed for the
        current OCIO shader build.
        """
        if not self._ocio_shader_desc or not self._shader_program:
            return

        for name, uniform_data in self._ocio_shader_desc.getUniforms():
            if name not in self._ocio_uniform_ids:
                uid = GL.glGetUniformLocation(self._shader_program, name)
                self._ocio_uniform_ids[name] = uid
            else:
                uid = self._ocio_uniform_ids[name]

            if uniform_data.type == ocio.UNIFORM_DOUBLE:
                GL.glUniform1f(uid, uniform_data.getDouble())

    def _update_ocio_dyn_prop(self, prop_type, value):
        """
        Update a specific OCIO dynamic property, which will be passed
        to the shader program as a uniform.

        :param ocio.DynamicPropertryType prop_type: Property type to
            update. Only one dynamic property per type is supported
            per processor, so only the first will be updated if there
            are multiple.
        :param any value: An appropriate value for the specific
            property type.
        """
        if not self._ocio_shader_desc:
            return

        if self._ocio_shader_desc.hasDynamicProperty(prop_type):
            dyn_prop = self._ocio_shader_desc.getDynamicProperty(prop_type)
            dyn_prop.setDouble(value)

    def _update_ocio_channel_hot(self, channel):
        """
        Update the OCIO GPU renderers channel view to either isolate a
        specific channel or show them all.

        :param int channel: ImagePlaneChannels value to toggle channel
            isolation.
        """
        # If index is in range, and we are viewing all channels, or a channel
        # other than index, isolate channel at index.
        if (channel < 4 and
                (all(self._ocio_channel_hot) or
                 not self._ocio_channel_hot[channel])):
            for i in range(4):
                self._ocio_channel_hot[i] = 1 if i == channel else 0

        # Otherwise show all channels
        else:
            for i in range(4):
                self._ocio_channel_hot[i] = 1

    def _m44f_to_ndarray(self, m44f):
        """
        Convert Imath.M44f matrix to a flat NumPy float32 array, so that it
        can be passed to PyOpenGL functions.

        :param Imath.M44f m44f: 4x4 matrix
        :return: NumPy array
        :rtype: numpy.array(16)
        """
        return np.array(
            [
                m44f[0][0], m44f[0][1], m44f[0][2], m44f[0][3],
                m44f[1][0], m44f[1][1], m44f[1][2], m44f[1][3],
                m44f[2][0], m44f[2][1], m44f[2][2], m44f[2][3],
                m44f[3][0], m44f[3][1], m44f[3][2], m44f[3][3],
            ],
            dtype=np.float32
        )

    def _v2f_to_v3f(self, v2f, z):
        """
        Extend an Imath.V2f to a Imath.V3f by adding a Z dimension value.

        :param Imath.V2f v2f: 2D float vector to extend
        :param float z: Z value to extend vector with
        :return: 3D float vector
        :rtype: Imath.V3f
        """
        return imath.V3f(v2f.x, v2f.y, z)

    def _widget_size_to_v2f(self, widget):
        """
        Get QWidget dimensions as a Imath.V2f.

        :param QWidget widget: Widget to get dimensions of
        :return: 2D float vector
        :rtype: Imath.V2f
        """
        return imath.V2f(widget.width(), widget.height())


class ImageViewChannels(object):
    """
    Enum to describe all the toggleable channel view options in
    ``ImagePlane``.
    """

    R, G, B, A, ALL = list(range(5))


class ImageView(QtWidgets.QWidget):
    """
    Main image view widget, which can display an image with internal
    32-bit float precision.
    """

    def __init__(self, parent=None):
        super(ImageView, self).__init__(parent)

        # OpenGL rendering window
        self.image_plane = ImagePlane(self)
        self.image_plane.setSizePolicy(QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Expanding,
            QtWidgets.QSizePolicy.Expanding
        ))

        # Top menu bar
        self.file_menu = QtWidgets.QMenu("File")
        self.file_menu.addAction(
            "Load image...",
            self.load_image,
            QtCore.Qt.CTRL + QtCore.Qt.Key_L
        )
        self.file_menu.addAction("Exit", self.close)

        self.color_menu = QtWidgets.QMenu("Color")
        self.input_cs_menu = self.color_menu.addMenu("Input color space")
        self.input_cs_action_group = QtWidgets.QActionGroup(self)
        self.input_cs_action_group.setExclusive(True)
        self.display_menu = self.color_menu.addMenu("Display")
        self.display_action_group = QtWidgets.QActionGroup(self)
        self.display_action_group.setExclusive(True)

        self.menu_bar = QtWidgets.QMenuBar()
        self.menu_bar.addMenu(self.file_menu)
        self.menu_bar.addMenu(self.color_menu)

        self.view_label = QtWidgets.QLabel("View")
        self.view_box = QtWidgets.QComboBox()

        self.exposure_label = QtWidgets.QLabel("Exposure")
        self.exposure_spinbox = QtWidgets.QDoubleSpinBox()
        self.exposure_spinbox.setRange(-6.0, 6.0)
        self.exposure_spinbox.setValue(self.image_plane.exposure())

        self.gamma_label = QtWidgets.QLabel("Gamma")
        self.gamma_spinbox = QtWidgets.QDoubleSpinBox()
        self.gamma_spinbox.setRange(0.0, 4.0)
        self.gamma_spinbox.setValue(self.image_plane.gamma())

        color_management_layout = QtWidgets.QHBoxLayout()
        color_management_layout.setContentsMargins(7, 5, 7, 5)
        color_management_layout.setSpacing(5)
        color_management_layout.addWidget(self.view_label)
        color_management_layout.addWidget(self.view_box)
        color_management_layout.addStretch()
        color_management_layout.addWidget(self.exposure_label)
        color_management_layout.addWidget(self.exposure_spinbox)
        color_management_layout.addWidget(self.gamma_label)
        color_management_layout.addWidget(self.gamma_spinbox)

        # Main layout
        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self.menu_bar)
        layout.addLayout(color_management_layout)
        layout.addWidget(self.image_plane)
        self.setLayout(layout)

        # Connect signals/slots
        self.input_cs_action_group.triggered.connect(
            self._on_input_colorspace_changed
        )
        self.display_action_group.triggered.connect(
            self._on_display_changed
        )
        self.view_box.currentIndexChanged[str].connect(
            self._on_view_changed
        )
        self.exposure_spinbox.valueChanged.connect(
            self._on_exposure_changed
        )
        self.gamma_spinbox.valueChanged.connect(
            self._on_gamma_changed
        )

        # Init GUI
        self._init_ocio()
        self.resize(QtCore.QSize(1280, 720))

    def load_image(self, image_path=None):
        """
        Load an image into the viewer.

        If no ``image_path`` is provided, a file dialog will allow the
        user to choose one.

        :param str image_path: Absolute path to image file
        """
        if image_path is None:
            # Prompt user to choose an image
            image_path, sel_filter = QtWidgets.QFileDialog.getOpenFileName(
                self,
                "Load image"
            )
            if not image_path:
                return

        self.image_plane.load_image(image_path)

        # Input color space or the current view could be changed by file and
        # viewing rules on image load. Update the GUI without triggering a
        # re-render.
        with self._ocio_signals_blocked():
            self.set_input_color_space(self.image_plane.input_colorspace())
            self.set_display_view(
                self.image_plane.display(),
                self.image_plane.view()
            )

    def view_channel(self, channel):
        """
        Isolate a specific channel by its index. Specifying an out
        of range index will restore the combined channel view.

        :param int channel: ImageViewChannels channel view to toggle.
            ALL always shows all channels.
        """
        self.image_plane.update_ocio_proc(channel=channel)

    def input_color_space(self):
        """
        :return: Input color space name
        :rtype: str
        """
        return self.input_cs_action_group.checkedAction().text()

    def set_input_color_space(self, cs_name):
        """
        Override current input color space. This controls how an input
        image should be interpreted by OCIO. Each loaded image utilizes
        OCIO config file rules to determine this automatically, so this
        override only guarantees persistence for the current image.

        :param str cs_name: OCIO color space name
        """
        for action in self.input_cs_action_group.actions():
            if action.text() == cs_name:
                action.setChecked(True)
                # Force group triggered signal
                if not self.input_cs_action_group.signalsBlocked():
                    self.input_cs_action_group.triggered.emit(action)
                break

    def display(self):
        """
        :return: OCIO display
        :rtype: str
        """
        return self.display_action_group.checkedAction().text()

    def view(self):
        """
        :return: OCIO view
        :rtype: str
        """
        return self.view_box.currentText()

    def set_display_view(self, display, view):
        """
        Set the OCIO display and view. The display represents the
        current display device colors are being viewed on. The view
        defines the output transform for the given display.

        :param str display: OCIO display
        :param str view: OCIO view
        """
        for action in self.display_action_group.actions():
            if action.text() == display:
                action.setChecked(True)
                # Force group triggered signal
                if not self.display_action_group.signalsBlocked():
                    self.display_action_group.triggered.emit(action)
                break

        view_idx = self.view_box.findText(view)
        if view_idx != -1:
            self.view_box.setCurrentIndex(view_idx)

    def exposure(self):
        """
        :return: Exposure value
        :rtype: float
        """
        return self.exposure_spinbox.value()

    def set_exposure(self, value):
        """
        Update viewer exposure, applied in scene_linear space prior to
        the output transform.

        :param float value: Exposure value in stops
        """
        self.exposure_spinbox.setValue(value)

    def gamma(self):
        """
        :return: Gamma value
        :rtype: float
        """
        return self.gamma_spinbox.value()

    def set_gamma(self, value):
        """
        Update viewer gamma, applied after the OCIO output transform.

        :param float value: Gamma value used like: pow(rgb, 1/gamma)
        """
        self.gamma_spinbox.setValue(value)

    @contextmanager
    def _ocio_signals_blocked(self):
        """
        This context manager can be used to prevent automatic OCIO
        processor updates while changing interconnected OCIO
        parameters.
        """
        self.input_cs_action_group.blockSignals(True)
        self.display_action_group.blockSignals(True)
        self.view_box.blockSignals(True)
        self.exposure_spinbox.blockSignals(True)
        self.gamma_spinbox.blockSignals(True)

        yield

        self.input_cs_action_group.blockSignals(False)
        self.display_action_group.blockSignals(False)
        self.view_box.blockSignals(False)
        self.exposure_spinbox.blockSignals(False)
        self.gamma_spinbox.blockSignals(False)

    def _init_ocio(self):
        """
        Runs exactly once to initialize all OCIO parameters and
        initiate creation of the initial OCIO processor.
        """
        # TODO: implement ColorSpaceMenuHelper

        with self._ocio_signals_blocked():

            # Assumes OCIO env var is set
            config = ocio.GetCurrentConfig()

            # Populate input color spaces
            all_cs_names = [cs.getName() for cs in config.getColorSpaces()]
            all_cs_names.sort()

            for action in self.input_cs_action_group.actions():
                self.input_cs_action_group.removeAction(action)
            self.input_cs_menu.clear()

            for i, cs_name in enumerate(all_cs_names):
                action = self.input_cs_menu.addAction(cs_name)
                action.setCheckable(True)
                if i == 0:
                    action.setChecked(True)
                self.input_cs_action_group.addAction(action)

            cs_actions = self.input_cs_action_group.actions()
            default_cs = config.getColorSpace(ocio.ROLE_DEFAULT)
            if default_cs:
                default_cs_name = default_cs.getName()
                if default_cs_name in all_cs_names:
                    cs_idx = all_cs_names.index(default_cs_name)
                    cs_actions[cs_idx].setChecked(True)

            # Populate displays
            for action in self.display_action_group.actions():
                self.display_action_group.removeAction(action)
            self.display_menu.clear()

            displays = list(config.getDisplays())

            for i, display in enumerate(displays):
                action = self.display_menu.addAction(display)
                action.setCheckable(True)
                if i == 0:
                    action.setChecked(True)
                self.display_action_group.addAction(action)

            default_display = config.getDefaultDisplay()

            # Populate views. They will be re-populated whenever the display
            # changes.
            self.view_box.clear()
            self.view_box.addItems(config.getViews(default_display))

            default_view = config.getDefaultView(default_display)

            # Set default display/view
            self.set_display_view(default_display, default_view)

            # Build OCIO processors
            self.image_plane.update_ocio_proc(
                input_cs=self.input_color_space(),
                display=self.display(),
                view=self.view()
            )

    @QtCore.Slot(QtWidgets.QAction)
    def _on_input_colorspace_changed(self, action):
        self.image_plane.update_ocio_proc(input_cs=action.text())

    @QtCore.Slot(QtWidgets.QAction)
    def _on_display_changed(self, action):
        with self._ocio_signals_blocked():

            config = ocio.GetCurrentConfig()
            display = action.text()

            # Re-populate views
            current_view = self.view()
            views = list(config.getViews(display))

            self.view_box.clear()
            self.view_box.addItems(views)

            if current_view in views:
                # Preserve current view if possible
                view_idx = views.index(current_view)
            else:
                # Fallback on default if not
                view_idx = self.view_box.findText(
                    config.getDefaultView(display)
                )

            if view_idx != -1:
                self.view_box.setCurrentIndex(view_idx)

            self.image_plane.update_ocio_proc(display=display, view=self.view())

    @QtCore.Slot(str)
    def _on_view_changed(self, view):
        self.image_plane.update_ocio_proc(view=view)

    @QtCore.Slot(float)
    def _on_exposure_changed(self, value):
        self.image_plane.update_exposure(value)

    @QtCore.Slot(float)
    def _on_gamma_changed(self, value):
        self.image_plane.update_gamma(value)


def main():
    # OpenGL core profile needed on macOS to access programmatic pipeline
    gl_format = QtOpenGL.QGLFormat()
    gl_format.setProfile(QtOpenGL.QGLFormat.CoreProfile)
    gl_format.setSampleBuffers(True)
    gl_format.setSwapInterval(1)
    gl_format.setVersion(4, 0)
    QtOpenGL.QGLFormat.setDefaultFormat(gl_format)

    app = QtWidgets.QApplication(sys.argv)
    viewer = ImageView()
    viewer.show()
    app.exec_()


if __name__ == "__main__":
    main()
