# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# TODO: Much of the OpenGL code in this module is adapted from the
#       oglapphelpers library bundled with OCIO. We should fully
#       reimplement that in Python for direct use in applications.

import ctypes
import logging
import math
from functools import partial
from pathlib import Path
from typing import Any, Optional
import sys

import numpy as np
from OpenGL import GL
try:
    import OpenImageIO as oiio
except:
    import imageio as iio
import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets, QtOpenGLWidgets

from ..log_handlers import message_queue
from ..ref_space_manager import ReferenceSpaceManager


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


class ImagePlane(QtOpenGLWidgets.QOpenGLWidget):
    """
    Qt-wrapped OpenGL window for drawing with PyOpenGL.
    """

    image_loaded = QtCore.Signal(Path, int, int)
    sample_changed = QtCore.Signal(int, int, float, float, float, float, float, float)
    scale_changed = QtCore.Signal(float)
    tf_subscription_requested = QtCore.Signal(int)

    def __init__(self, parent: Optional[QtWidgets.QWidget] = None):
        super().__init__(parent)

        # Clicking on/tabbing to widget restores focus
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setMouseTracking(True)

        # Set to True after initializeGL is called. Don't allow grabbing
        # OpenGL context until that point.
        self._gl_ready = False

        # Color management
        self._ocio_input_color_space = None
        self._ocio_tf = None
        self._ocio_exposure = 0.0
        self._ocio_gamma = 1.0
        self._ocio_channel_hot = [1, 1, 1, 1]
        self._ocio_tf_proc = None
        self._ocio_tf_proc_cpu = None
        self._ocio_proc_cache_id = None
        self._ocio_shader_cache_id = None
        self._ocio_shader_desc = None
        self._ocio_tex_start_index = 1  # Start after image_tex
        self._ocio_tex_ids = []
        self._ocio_uniform_ids = {}

        # MVP matrix components
        self._model_view_mat = np.eye(4)
        self._proj_mat = np.eye(4)

        # Keyboard shortcuts
        self._shortcuts = []

        # Mouse info
        self._mouse_pressed = False
        self._mouse_last_pos = QtCore.QPointF()

        # Image texture
        self._image_array = None
        self._image_tex = None
        self._image_pos = np.array([0.0, 0.0])
        self._image_size = np.array([1.0, 1.0])
        self._image_scale = 1.0

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

    def initializeGL(self) -> None:
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
            self._image_size[0],
            self._image_size[1],
            0,
            GL.GL_RGBA,
            GL.GL_FLOAT,
            ctypes.c_void_p(0),
        )

        GL.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_S, GL.GL_CLAMP_TO_EDGE)
        GL.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_T, GL.GL_CLAMP_TO_EDGE)
        self._set_ocio_tex_params(GL.GL_TEXTURE_2D, ocio.INTERP_LINEAR)

        # Init image plane
        # fmt: off
        plane_position_data = np.array(
            [
                -0.5,
                 0.5,
                 0.0,  # top-left
                 0.5,
                 0.5,
                 0.0,  # top-right
                 0.5,
                -0.5,
                 0.0,  # bottom-right
                -0.5,
                -0.5,
                 0.0,  # bottom-left
            ],
            dtype=np.float32,
        )
        # fmt: on

        plane_tex_coord_data = np.array(
            [
                0.0,
                1.0,  # top-left
                1.0,
                1.0,  # top-right
                1.0,
                0.0,  # bottom-right
                0.0,
                0.0,  # bottom-left
            ],
            dtype=np.float32,
        )

        plane_index_data = np.array(
            [0, 1, 2, 0, 2, 3],  # triangles: top-left, bottom-right
            dtype=np.uint32,
        )

        self._plane_vao = GL.glGenVertexArrays(1)
        GL.glBindVertexArray(self._plane_vao)

        (
            self._plane_position_vbo,
            self._plane_tex_coord_vbo,
            self._plane_index_vbo,
        ) = GL.glGenBuffers(3)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self._plane_position_vbo)
        GL.glBufferData(
            GL.GL_ARRAY_BUFFER,
            plane_position_data.nbytes,
            plane_position_data,
            GL.GL_STATIC_DRAW,
        )
        GL.glVertexAttribPointer(0, 3, GL.GL_FLOAT, GL.GL_FALSE, 0, ctypes.c_void_p(0))
        GL.glEnableVertexAttribArray(0)

        GL.glBindBuffer(GL.GL_ARRAY_BUFFER, self._plane_tex_coord_vbo)
        GL.glBufferData(
            GL.GL_ARRAY_BUFFER,
            plane_tex_coord_data.nbytes,
            plane_tex_coord_data,
            GL.GL_STATIC_DRAW,
        )
        GL.glVertexAttribPointer(1, 2, GL.GL_FLOAT, GL.GL_FALSE, 0, ctypes.c_void_p(0))
        GL.glEnableVertexAttribArray(1)

        GL.glBindBuffer(GL.GL_ELEMENT_ARRAY_BUFFER, self._plane_index_vbo)
        GL.glBufferData(
            GL.GL_ELEMENT_ARRAY_BUFFER,
            plane_index_data.nbytes,
            plane_index_data,
            GL.GL_STATIC_DRAW,
        )

        self._build_program()

    def _orthographicProjMatrix(self, near, far, left, right, top, bottom):
        rightPlusLeft  = right + left
        rightMinusLeft = right - left

        topPlusBottom  = top + bottom
        topMinusBottom = top - bottom

        farPlusNear  = far + near
        farMinusNear = far - near

        tx = -rightPlusLeft / rightMinusLeft
        ty = -topPlusBottom / topMinusBottom
        tz = -farPlusNear / farMinusNear

        A = 2 / rightMinusLeft
        B = 2 / topMinusBottom
        C = -2 / farMinusNear

        return np.array([
            [A, 0, 0, tx],
            [0, B, 0, ty],
            [0, 0, C, tz],
            [0, 0, 0, 1 ]
        ])

    def resizeGL(self, w: int, h: int) -> None:
        """
        Called whenever the widget is resized.

        :param w: Window width
        :param h: Window height
        """
        GL.glViewport(0, 0, w, h)

        # Center image plane
        # fmt: on
        self._proj_mat = self._orthographicProjMatrix(
                -1.0,  # Near
                 1.0,  # Far
            -w / 2.0,  # Left
             w / 2.0,  # Right
             h / 2.0,  # Top
            -h / 2.0,  # Bottom
        )

        self._update_model_view_mat()

    def paintGL(self) -> None:
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
            mvp_mat = self._proj_mat @ self._model_view_mat
            mvp_mat_loc = GL.glGetUniformLocation(self._shader_program, "mvpMat")
            GL.glUniformMatrix4fv(
                mvp_mat_loc, 1, GL.GL_FALSE, mvp_mat.T
            )

            image_tex_loc = GL.glGetUniformLocation(self._shader_program, "imageTex")
            GL.glUniform1i(image_tex_loc, 0)

            # Bind texture, VAO, and draw
            GL.glActiveTexture(GL.GL_TEXTURE0 + 0)
            GL.glBindTexture(GL.GL_TEXTURE_2D, self._image_tex)

            GL.glBindVertexArray(self._plane_vao)

            GL.glDrawElements(
                GL.GL_TRIANGLES, 6, GL.GL_UNSIGNED_INT, ctypes.c_void_p(0)
            )

            GL.glBindVertexArray(0)

    def load_oiio(self, image_path: Path) -> np.ndarray:
        image_buf = oiio.ImageBuf(image_path.as_posix())
        spec = image_buf.spec()

        # Convert to RGBA, filling missing color channels with 0.0, and a
        # missing alpha with 1.0.
        if spec.nchannels < 4:
            image_buf = oiio.ImageBufAlgo.channels(
                image_buf,
                tuple(
                    list(range(spec.nchannels))
                    + ([0.0] * (4 - spec.nchannels - 1))
                    + [1.0]
                ),
                newchannelnames=("R", "G", "B", "A"),
            )
        elif spec.nchannels > 4:
            image_buf = oiio.ImageBufAlgo.channels(
                image_buf, (0, 1, 2, 3), newchannelnames=("R", "G", "B", "A")
            )

        # Get pixels as 32-bit float NumPy array
        return image_buf.get_pixels(oiio.FLOAT)

    def load_iio(self, image_path: Path) -> np.ndarray:
        data = iio.imread(image_path.as_posix())

        # Convert to 32-bit float
        if not np.issubdtype(data.dtype, np.floating):
            data = data.astype(np.float32) / np.iinfo(data.dtype).max
        if data.dtype != np.float32:
            data = data.astype(np.float32)

        # Convert to RGBA, filling missing color channels with 0.0, and a
        # missing alpha with 1.0.
        nchannels = 1
        if len(data.shape) == 3:
            nchannels = data.shape[-1]

        while nchannels < 3:
            data = np.dstack((data, np.zeros(data.shape[:2])))
            nchannels += 1
        if nchannels < 4:
            data = np.dstack((data, np.ones(data.shape[:2])))
        if nchannels > 4:
            data = data[..., :4]

        return data

    def load_image(self, image_path: Path) -> None:
        """
        Load an image into the image plane texture.

        :param image_path: Image file path
        """
        config = ocio.GetCurrentConfig()

        # Get input color space (file rule)
        color_space_name, rule_idx = config.getColorSpaceFromFilepath(
            image_path.as_posix()
        )
        if not color_space_name:
            # Use previous or config default
            if self._ocio_input_color_space:
                color_space_name = self._ocio_input_color_space
            else:
                color_space_name = ocio.ROLE_DEFAULT
        self._ocio_input_color_space = color_space_name

        if "OpenImageIO" in sys.modules:
            self._image_array = self.load_oiio(image_path)
        else:
            self._image_array = self.load_iio(image_path)

        width = self._image_array.shape[1]
        height = self._image_array.shape[0]

        # Stash image size for pan/zoom calculations

        self._image_pos = np.array([0, 1], dtype=np.float64)
        self._image_size = np.array([width, height], dtype=np.float64)

        # Load image data into texture
        self.makeCurrent()

        GL.glBindTexture(GL.GL_TEXTURE_2D, self._image_tex)
        GL.glTexImage2D(
            GL.GL_TEXTURE_2D,
            0,
            GL.GL_RGBA32F,
            width,
            height,
            0,
            GL.GL_RGBA,
            GL.GL_FLOAT,
            self._image_array.ravel(),
        )

        self.image_loaded.emit(
            image_path, int(self._image_size[0]), int(self._image_size[1])
        )

        self.update_ocio_proc(input_color_space=self._ocio_input_color_space)
        self.fit()

    def input_color_space(self) -> str:
        """
        :return: Current input OCIO color space name
        """
        return self._ocio_input_color_space

    def transform(self) -> Optional[ocio.Transform]:
        """
        :return: Current OCIO transform
        """
        return self._ocio_tf

    def clear_transform(self) -> None:
        """
        Clear current OCIO transform, passing through the input image.
        """
        self._ocio_tf = None

        self.update_ocio_proc(force_update=True)

    def reset_ocio_proc(self, update: bool = False) -> None:
        """
        Reset the OCIO GPU renderer to a passthrough state.

        :param update: Whether to redraw viewport
        """
        self._ocio_input_color_space = None
        self._ocio_tf = None
        self._ocio_exposure = 0.0
        self._ocio_gamma = 1.0
        self._ocio_channel_hot = [1, 1, 1, 1]

        if update:
            self.update_ocio_proc(force_update=True)

    def update_ocio_proc(
        self,
        input_color_space: Optional[str] = None,
        transform: Optional[ocio.Transform] = None,
        channel: Optional[int] = None,
        force_update: bool = False,
    ):
        """
        Update one or more aspects of the OCIO GPU renderer. Parameters
        are cached, so not providing a parameter maintains the existing
        state. This will trigger a GL update IF the underlying OCIO ops
        in the processor have changed.

        :param input_color_space: Input OCIO color space name
        :param transform: Optional main OCIO transform, to be applied
            from the current config's scene reference space.
        :param channel: ImagePlaneChannels value to toggle channel
            isolation.
        :param force_update: Set to True to update the viewport even
            when the processor has not been updated.
        """
        # Update processor parameters
        if input_color_space is not None:
            self._ocio_input_color_space = input_color_space
        if transform is not None:
            self._ocio_tf = transform
        if channel is not None:
            self._update_ocio_channel_hot(channel)

        config = ocio.GetCurrentConfig()
        has_scene_linear = config.hasRole(ocio.ROLE_SCENE_LINEAR)
        scene_ref_name = ReferenceSpaceManager.scene_reference_space().getName()

        # Build simplified viewing pipeline:
        # - GPU: For viewport rendering
        # - CPU: For pixel sampling, sans viewport adjustments
        gpu_viewing_pipeline = ocio.GroupTransform()
        cpu_viewing_pipeline = ocio.GroupTransform()

        # Convert to scene linear space if input space is known
        if has_scene_linear and self._ocio_input_color_space:
            to_scene_linear = ocio.ColorSpaceTransform(
                src=self._ocio_input_color_space, dst=ocio.ROLE_SCENE_LINEAR
            )
            gpu_viewing_pipeline.appendTransform(to_scene_linear)
            cpu_viewing_pipeline.appendTransform(to_scene_linear)

        # Dynamic exposure adjustment
        gpu_viewing_pipeline.appendTransform(
            ocio.ExposureContrastTransform(
                exposure=self._ocio_exposure, dynamicExposure=True
            )
        )

        # Convert to the scene reference space, which is the expected input space for
        # all provided transforms. If the input color space is not known, the transform
        # will be applied to unmodified input pixels.
        if has_scene_linear and self._ocio_input_color_space:
            to_scene_ref = ocio.ColorSpaceTransform(
                src=ocio.ROLE_SCENE_LINEAR, dst=scene_ref_name
            )
            gpu_viewing_pipeline.appendTransform(to_scene_ref)
            cpu_viewing_pipeline.appendTransform(to_scene_ref)
        elif self._ocio_input_color_space:
            to_scene_ref = ocio.ColorSpaceTransform(
                src=self._ocio_input_color_space, dst=scene_ref_name
            )
            gpu_viewing_pipeline.appendTransform(to_scene_ref)
            cpu_viewing_pipeline.appendTransform(to_scene_ref)

        # Main transform
        if self._ocio_tf is not None:
            gpu_viewing_pipeline.appendTransform(self._ocio_tf)
            cpu_viewing_pipeline.appendTransform(self._ocio_tf)

        # Or restore input color space, if known
        elif self._ocio_input_color_space:
            from_scene_ref = ocio.ColorSpaceTransform(
                src=scene_ref_name, dst=self._ocio_input_color_space
            )
            gpu_viewing_pipeline.appendTransform(from_scene_ref)
            cpu_viewing_pipeline.appendTransform(from_scene_ref)

        # Channel view
        gpu_viewing_pipeline.appendTransform(
            ocio.MatrixTransform.View(
                channelHot=self._ocio_channel_hot,
                lumaCoef=config.getDefaultLumaCoefs(),
            )
        )

        # Dynamic gamma adjustment
        gpu_viewing_pipeline.appendTransform(
            ocio.ExposureContrastTransform(
                gamma=self._ocio_gamma, pivot=1.0, dynamicGamma=True
            )
        )

        # Create GPU processor
        gpu_proc = config.getProcessor(gpu_viewing_pipeline, ocio.TRANSFORM_DIR_FORWARD)

        if gpu_proc.getCacheID() != self._ocio_proc_cache_id:
            # Update CPU processor
            cpu_proc = config.getProcessor(
                cpu_viewing_pipeline, ocio.TRANSFORM_DIR_FORWARD
            )
            self._ocio_tf_proc = cpu_proc
            self._ocio_tf_proc_cpu = cpu_proc.getDefaultCPUProcessor()

            # Update GPU processor shaders and textures
            self._ocio_shader_desc = ocio.GpuShaderDesc.CreateShaderDesc(
                language=ocio.GPU_LANGUAGE_GLSL_4_0
            )
            self._ocio_proc_cache_id = gpu_proc.getCacheID()
            ocio_gpu_proc = gpu_proc.getDefaultGPUProcessor()
            ocio_gpu_proc.extractGpuShaderInfo(self._ocio_shader_desc)

            self._allocate_ocio_tex()
            self._build_program()

            # Set initial dynamic property state
            self._update_ocio_dyn_prop(
                ocio.DYNAMIC_PROPERTY_EXPOSURE, self._ocio_exposure
            )
            self._update_ocio_dyn_prop(ocio.DYNAMIC_PROPERTY_GAMMA, self._ocio_gamma)

            self.update()

            # Log processor change after render
            message_queue.put_nowait(cpu_proc)

        elif force_update:
            self.update()

            # The transform and processor has not changed, but other app components
            # which view it may have dropped tje reference. Log processor to update
            # them as needed.
            if self._ocio_tf_proc is not None:
                message_queue.put_nowait(self._ocio_tf_proc)

    def exposure(self) -> float:
        """
        :return: Last set exposure dynamic property value
        """
        return self._ocio_exposure

    def update_exposure(self, value: float) -> None:
        """
        Update OCIO GPU renderer exposure. This is a dynamic property,
        implemented as a GLSL uniform, so can be updated without
        modifying the OCIO shader program or its dependencies.

        :param value: Exposure value in stops
        """
        self._ocio_exposure = value
        self._update_ocio_dyn_prop(ocio.DYNAMIC_PROPERTY_EXPOSURE, value)
        self.update()

    def gamma(self) -> float:
        """
        :return: Last set gamma dynamic property value
        """
        return self._ocio_gamma

    def update_gamma(self, value: float) -> None:
        """
        Update OCIO GPU renderer gamma. This is a dynamic property,
        implemented as a GLSL uniform, so can be updated without
        modifying the OCIO shader program or its dependencies.

        .. note::
            Value is floor clamped at 0.001 to prevent zero division
            errors.

        :param value: Gamma value used like: pow(rgb, 1/gamma)
        """
        # Translate gamma to exponent, enforcing floor
        value = 1.0 / max(0.001, value)

        self._ocio_gamma = value
        self._update_ocio_dyn_prop(ocio.DYNAMIC_PROPERTY_GAMMA, value)
        self.update()

    def enterEvent(self, event: QtCore.QEvent) -> None:
        for shortcut in self._shortcuts:
            shortcut.setEnabled(True)

    def leaveEvent(self, event: QtCore.QEvent) -> None:
        for shortcut in self._shortcuts:
            shortcut.setEnabled(False)

    def mousePressEvent(self, event: QtGui.QMouseEvent) -> None:
        self._mouse_pressed = True
        self._mouse_last_pos = event.pos()

    def mouseMoveEvent(self, event: QtGui.QMouseEvent) -> None:
        pos = event.pos()

        if self._mouse_pressed:
            offset = np.array([*(pos - self._mouse_last_pos).toTuple()])
            self._mouse_last_pos = pos

            self.pan(offset, update=True)
        else:
            widget_w = self.width()
            widget_h = self.height()

            # Trace mouse position through the inverse MVP matrix to update sampled
            # pixel.
            screen_pos = np.array([
                pos.x() / widget_w * 2.0 - 1.0,
                (widget_h - pos.y() - 1) / widget_h * 2.0 - 1.0,
                0.0,
                1.0
            ])
            model_pos = np.linalg.inv(self._proj_mat @ self._model_view_mat) @ screen_pos
            pixel_pos = np.array([model_pos[0] + 0.5, model_pos[1] + 0.5]) * self._image_size

            # Broadcast sample position
            if (
                self._image_array is not None
                and 0 <= pixel_pos[0] <= self._image_size[0]
                and 0 <= pixel_pos[1] <= self._image_size[1]
            ):
                pixel_x = math.floor(pixel_pos[0])
                pixel_y = math.floor(pixel_pos[1])
                pixel_input = list(self._image_array[pixel_y, pixel_x])
                if len(pixel_input) < 3:
                    pixel_input += [0.0] * (3 - len(pixel_input))
                elif len(pixel_input) > 3:
                    pixel_input = pixel_input[:3]

                # Sample output pixel with CPU processor
                if self._ocio_tf_proc_cpu is not None:
                    pixel_output = self._ocio_tf_proc_cpu.applyRGB(pixel_input)
                else:
                    pixel_output = pixel_input.copy()

                self.sample_changed.emit(pixel_x, pixel_y, *pixel_input, *pixel_output)
            else:
                # Out of image bounds
                self.sample_changed.emit(-1, -1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

    def mouseReleaseEvent(self, event: QtGui.QMouseEvent) -> None:
        self._mouse_pressed = False

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        w, h = self.width(), self.height()

        # Fit image to frame
        if h > w:
            min_scale = w / self._image_size[0]
        else:
            min_scale = h / self._image_size[1]

        # Fill frame with 1 pixel with 0.5 pixel overscan
        max_scale = max(w, h) * 1.5

        delta = event.angleDelta().y() / 360.0
        scale = max(0.01, self._image_scale - delta)

        if scale > 1.0:
            if delta > 0.0:
                # Exponential zoom out
                scale = pow(scale, 1.0 / 1.01)
            else:
                # Exponential zoom in
                scale = pow(scale, 1.01)

        scale = min(max_scale, max(min_scale, scale))

        if scale < 1.0:
            # Half zoom in/out
            scale = (self._image_scale + scale) / 2.0

        self.zoom(event.pos(), scale, update=True, absolute=True)

    def pan(
        self, offset: np.ndarray, update: bool = True, absolute: bool = False
    ) -> None:
        """
        Pan the viewport by the specified offset in screen space.

        :param offset: Offset in pixels
        :param update: Whether to redraw the viewport
        :param absolute: When True, offset will be treated as an
            absolute position to translate the viewport from its
            origin.
        """
        if absolute:
            self._image_pos = offset / self._image_scale
        else:
            self._image_pos += offset / self._image_scale

        self._update_model_view_mat(update=update)

    def zoom(
        self,
        point: QtCore.QPoint,
        amount: float,
        update: bool = True,
        absolute: bool = False,
    ) -> None:
        """
        Zoom the viewport by the specified scale offset amount.

        :param point: Viewport position to center zoom on
        :param amount: Zoom scale amount
        :param update: Whether to redraw the viewport
        :param absolute: When True, amount will be treated as an
            absolute scale to set the viewport to.
        """
        offset = np.array([*(point - self.rect().center()).toTuple()])

        self.pan(-offset, update=False)

        if absolute:
            self._image_scale = amount
        else:
            self._image_scale += amount

        self._update_model_view_mat(update=False)

        self.pan(offset, update=update)

        if self._image_array is not None:
            self.scale_changed.emit(self._image_scale)

    def fit(self, update: bool = True) -> None:
        """
        Pan and zoom so the image fits within the viewport and is
        centered.

        :param update: Whether to redraw the viewport
        """
        w, h = self.width(), self.height()

        # Fit image to frame
        if h > w:
            scale = w / self._image_size[0]
        else:
            scale = h / self._image_size[1]

        self.zoom(QtCore.QPoint(), scale, update=False, absolute=True)
        self.pan(np.array([0.0, 0.0]), update=update, absolute=True)

    def _install_shortcuts(self) -> None:
        """
        Setup supported keyboard shortcuts.
        """
        # R,G,B,A = view channel
        # C = view color
        for i, key in enumerate(("R", "G", "B", "A", "C")):
            channel_shortcut = QtGui.QShortcut(QtGui.QKeySequence(key), self)
            channel_shortcut.activated.connect(
                partial(self.update_ocio_proc, channel=i)
            )
            self._shortcuts.append(channel_shortcut)

        # Number keys = Subscribe to transform @ slot
        for i in range(10):
            subscribe_shortcut = QtGui.QShortcut(QtGui.QKeySequence(str(i)), self)
            subscribe_shortcut.activated.connect(
                lambda slot=i: self.tf_subscription_requested.emit(slot)
            )
            self._shortcuts.append(subscribe_shortcut)

        # Ctrl + Number keys = Power of 2 scale: 1 = x1, 2 = x2, 3 = x4, ...
        for i in range(9):
            scale_shortcut = QtGui.QShortcut(
                QtGui.QKeySequence(f"Ctrl+{i + 1}"), self
            )
            scale_shortcut.activated.connect(
                lambda exponent=i: self.zoom(
                    self.rect().center(), float(2**exponent), absolute=True
                )
            )
            self._shortcuts.append(scale_shortcut)

        # F = fit image to viewport
        fit_shortcut = QtGui.QShortcut(QtGui.QKeySequence("F"), self)
        fit_shortcut.activated.connect(self.fit)
        self._shortcuts.append(fit_shortcut)

    def _compile_shader(
        self, glsl_src: str, shader_type: GL.GLenum
    ) -> Optional[GL.GLuint]:
        """
        Compile GLSL shader and return its object ID.

        :param glsl_src: Shader source code
        :param shader_type: Type of shader to be created, which is an
            enum adhering to the formatting ``GL_*_SHADER``.
        :return: Shader object ID, or None if shader compilation fails
        """
        shader = GL.glCreateShader(shader_type)
        GL.glShaderSource(shader, glsl_src)
        GL.glCompileShader(shader)

        compile_status = GL.glGetShaderiv(shader, GL.GL_COMPILE_STATUS)
        if not compile_status:
            compile_log = GL.glGetShaderInfoLog(shader)
            logger.error("Shader program compile error: {log}".format(log=compile_log))
            return None

        return shader

    def _build_program(self, force: bool = False) -> None:
        """
        This builds the initial shader program, and rebuilds its
        fragment shader whenever the OCIO GPU renderer changes.

        :param force: Whether to force a rebuild even if the OCIO
            shader cache ID has not changed.
        """
        if not self._gl_ready:
            return

        self.makeCurrent()

        # If new shader cache ID matches previous cache ID, existing program
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
            self._vert_shader = self._compile_shader(GLSL_VERT_SRC, GL.GL_VERTEX_SHADER)
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
        self._frag_shader = self._compile_shader(frag_src, GL.GL_FRAGMENT_SHADER)
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
            logger.error("Shader program link error: {log}".format(log=link_log))
            return

        # Store cache ID to detect reuse
        self._ocio_shader_cache_id = shader_cache_id

    def _update_model_view_mat(self, update: bool = True) -> None:
        """
        Re-calculate the model view matrix, which needs to be updated
        prior to rendering if the image or window size have changed.

        :param bool update: Optionally redraw the window
        """
        size = np.array([self.width(), self.height()])

        self._model_view_mat = np.eye(4)

        # Flip Y to account for different OIIO/OpenGL image origin
        self._model_view_mat *= [1.0, -1.0, 1.0, 1.0]

        self._model_view_mat *= [self._image_scale, self._image_scale, 1.0, 1.0]
        self._model_view_mat[:2, -1] += self._image_pos / size * 2.0

        self._model_view_mat *= self._image_size.tolist() + [1.0, 1.0]

        # Use nearest interpolation when scaling up to see pixels
        if self._image_scale > 1.0:
            self._set_ocio_tex_params(GL.GL_TEXTURE_2D, ocio.INTERP_NEAREST)
        else:
            self._set_ocio_tex_params(GL.GL_TEXTURE_2D, ocio.INTERP_LINEAR)

        if update:
            self.update()

    def _set_ocio_tex_params(
        self, tex_type: GL.GLenum, interpolation: ocio.Interpolation
    ) -> None:
        """
        Set texture parameters for an OCIO LUT texture based on its
        type and interpolation.

        :param tex_type: OpenGL texture type (GL_TEXTURE_1/2/3D)
        :param interpolation: Interpolation enum value
        """
        if interpolation == ocio.INTERP_NEAREST:
            GL.glTexParameteri(tex_type, GL.GL_TEXTURE_MIN_FILTER, GL.GL_NEAREST)
            GL.glTexParameteri(tex_type, GL.GL_TEXTURE_MAG_FILTER, GL.GL_NEAREST)
        else:
            GL.glTexParameteri(tex_type, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR)
            GL.glTexParameteri(tex_type, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR)

    def _allocate_ocio_tex(self) -> None:
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
                tex_data,
            )

            self._ocio_tex_ids.append(
                (
                    tex,
                    tex_info.textureName,
                    tex_info.samplerName,
                    GL.GL_TEXTURE_3D,
                    tex_index,
                )
            )
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

            if tex_info.height > 1:
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
                    tex_data,
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
                    tex_data,
                )

            self._ocio_tex_ids.append(
                (tex, tex_info.textureName, tex_info.samplerName, tex_type, tex_index)
            )
            tex_index += 1

    def _del_ocio_tex(self) -> None:
        """
        Delete all OCIO textures from the GPU.
        """
        for tex, tex_name, sampler_name, tex_type, tex_index in self._ocio_tex_ids:
            GL.glDeleteTextures([tex])
        del self._ocio_tex_ids[:]

    def _use_ocio_tex(self) -> None:
        """
        Bind all OCIO textures to the shader program.
        """
        for tex, tex_name, sampler_name, tex_type, tex_index in self._ocio_tex_ids:
            GL.glActiveTexture(GL.GL_TEXTURE0 + tex_index)
            GL.glBindTexture(tex_type, tex)
            GL.glUniform1i(
                GL.glGetUniformLocation(self._shader_program, sampler_name), tex_index
            )

    def _del_ocio_uniforms(self) -> None:
        """
        Forget about the dynamic property uniforms needed for the
        previous OCIO shader build.
        """
        self._ocio_uniform_ids.clear()

    def _use_ocio_uniforms(self) -> None:
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

    def _update_ocio_dyn_prop(
        self, prop_type: ocio.DynamicPropertyType, value: Any
    ) -> None:
        """
        Update a specific OCIO dynamic property, which will be passed
        to the shader program as a uniform.

        :param prop_type: Property type to update. Only one dynamic
            property per type is supported per processor, so only the
            first will be updated if there are multiple.
        :param value: An appropriate value for the specific property
            type.
        """
        if not self._ocio_shader_desc:
            return

        if self._ocio_shader_desc.hasDynamicProperty(prop_type):
            dyn_prop = self._ocio_shader_desc.getDynamicProperty(prop_type)
            dyn_prop.setDouble(value)

    def _update_ocio_channel_hot(self, channel: int) -> None:
        """
        Update the OCIO GPU renderers channel view to either isolate a
        specific channel or show them all.

        :param channel: ImagePlaneChannels value to toggle channel
            isolation.
        """
        # If index is in range, and we are viewing all channels, or a channel
        # other than index, isolate channel at index.
        if channel < 4 and (
            all(self._ocio_channel_hot) or not self._ocio_channel_hot[channel]
        ):
            for i in range(4):
                self._ocio_channel_hot[i] = 1 if i == channel else 0

        # Otherwise show all channels
        else:
            for i in range(4):
                self._ocio_channel_hot[i] = 1
