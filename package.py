name = "OpenColorIO"
version = "2.0.0.3"
description = """
OpenColorIO -- it's open, it's color, and it's io af
"""
authors = ["ASWF"]

tools = [
    "ociobakelut", "ociocheck", "ociochecklut", "ocioconvert", "ociodisplay",
    "ociolutimage", "ociomakeclf", "ocioperf", "ociowrite"
]

# @early()
# def uuid():
#     import uuid
#     return str(uuid.uuid5(uuid.NAMESPACE_DNS, name))
uuid = '5f2447f3-ac4e-49ae-8690-eb9ca68e7475'


@early()
def variants():
    from rez.package_py_utils import expand_requires
    requires = ["platform-**", "arch-**", "os-**"]
    pys = [
        #'python-3.7.**',
        'python-3.8.**',
        #'python-2.7.**',
    ]
    ephemerals = [
        #'.shared-1',
        '.shared-0',
    ]
    return [
        expand_requires(*requires + [e, py]) for e in ephemerals for py in pys
    ]


hashed_variants = True

requires = []

private_build_requires = [
    "gcc-4+",
    "ninja",
    "cmake-3.12+",
    #"GLEW-2.1.0",
    #"GLFW-3.3.2",
    #"freeglut-3",

    ## docs (optional)
    'sphinx_press_theme',
    'testresources',
    'recommonmark',
    'breathe',
    'doxygen',
    'sphinx_tabs',

    ## third-party (optional)

    #"pybind11-2.5.0",
    #"lcms2-2",
    #'yamlcpp-0.6.3+<1',
    #'pystring-1.1.3+',
    #'libexpat-2.2.5+',
    #"OpenImageIO-2.1.12.0",  # shared
]


def pre_build_commands():
    # options
    shared = intersects(ephemerals.get('shared', '0'), '1')
    build_python = 'python' in resolve
    build_nuke = 'nuke' in resolve
    build_apps = 'OpenImageIO' in resolve

    env.LDFLAGS = '-Wl,-rpath,{build.install_path}/lib $LDFLAGS'
    env.CXXFLAGS = '$CXXFLAGS -Wno-deprecated-declarations -Wno-unused-variable -fPIC'

    env.OpenImageIO_ROOT = '$REZ_OPENIMAGEIO_ROOT'
    env.Python_ROOT = resolve.python.root if 'python' in resolve else ''
    env.pystring_ROOT = resolve.pystring.root if 'pystring' in resolve else ''
    env.Pybind11_ROOT = resolve.pybind11.root if 'pybind11' in resolve else ''
    env.Half_ROOT = resolve.openexr.root if 'openexr' in resolve else resolve.half.root if 'half' in resolve else ''
    env.Expat_ROOT = resolve.libexpat.root if 'libexpat' in resolve else ''

    env.REZ_BUILD_CMAKE_ARGS = ' '.join([
        '-DBUILD_SHARED_LIBS={shared}',
        '-DOCIO_BUILD_APPS=OFF',
        #'-DOCIO_BUILD_OPENFX=OFF',
        '-DOCIO_BUILD_NUKE={build_nuke}',
        '-DOCIO_BUILD_DOCS=OFF',
        '-DOCIO_BUILD_TESTS=OFF',
        '-DOCIO_BUILD_GPU_TESTS=OFF',
        '-DOCIO_BUILD_PYTHON={build_python}',
        '-DOCIO_BUILD_JAVA=OFF',
        '-DOCIO_USE_HEADLESS=ON',
        #'-DOCIO_NAMESPACE=OpenColorIO',
        #'-DOCIO_LIBNAME_SUFFIX=""',
        '-DCMAKE_INSTALL_PREFIX={build.install_path}',
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_CXX_STANDARD=14',
        '-DOCIO_INSTALL_EXT_PACKAGES=MISSING',
        '-DOCIO_WARNING_AS_ERROR=OFF',
        '-DPython_EXECUTABLE={resolve.python.root}/bin/python',
    ])


build_command_local = """
cmake -G Ninja -d {root} $REZ_BUILD_CMAKE_ARGS
ninja {install}
"""

build_command = """
git clone https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cmake OpenColorIO $REZ_BUILD_CMAKE_ARGS
make {install} -j$REZ_BUILD_THREAD_COUNT -Wno-unused-variable
"""

#build_command = build_command_local


def commands():
    env.PATH.prepend("{root}/bin")

    if 'python' in resolve:
        python_version = "{resolve.python.version.major}.{resolve.python.version.minor}"
        env.PYTHONPATH.append(
            "{root}/lib/python{python_version}/site-packages")

    if "nuke" in resolve:
        nuke_version = "{resolve.nuke.version.major}.{resolve.nuke.version.minor}"
        env.NUKE_PATH.prepend(
            '{root}/share/nuke:{root}/lib/nuke/{nuke_version}')

    if building:
        env.CFLAGS.set("-I{root}/include -I{root}/include/OpenColorIO $CFLAGS")
        env.CPPFLAGS.set(
            "-I{root}/include -I{root}/include/OpenColorIO $CPPFLAGS")
        env.CXXFLAGS.set(
            "-I{root}/include -I{root}/include/OpenColorIO $CXXFLAGS")
        env.LDFLAGS.set("-L{root}/lib -Wl,{root}/lib $LDFLAGS")

        env.CMAKE_INCLUDE_PATH.prepend("{root}/include")
        env.CMAKE_LIBRARY_PATH.prepend("{root}/lib")

        env.OpenColorIO_ROOT = '{root}'
        env.OpenColorIO_INCLUDE_DIRS = '{root}/include'
