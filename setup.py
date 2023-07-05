# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Adapted from: https://github.com/pybind/cmake_example
#

import os
import re
import shutil
import subprocess
import sys
import tempfile

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# Extract the project version from CMake generated ABI header.
# NOTE: When droping support for Python2 we can use
# tempfile.TemporaryDirectory() context manager instead of try...finally.
def get_version():
    VERSION_REGEX = re.compile(
        r"^\s*#\s*define\s+OCIO_VERSION_FULL_STR\s+\"(.*)\"\s*$", re.MULTILINE)

    here = os.path.abspath(os.path.dirname(__file__))
    dirpath = tempfile.mkdtemp()

    try:
        stdout = subprocess.check_output(["cmake", here], cwd=dirpath)
        path = os.path.join(dirpath, "include", "OpenColorIO", "OpenColorABI.h")
        with open(path) as f:
            match = VERSION_REGEX.search(f.read())
            return match.group(1)
    except Exception as e:
        raise RuntimeError(
            "Unable to find OpenColorIO version: {}".format(str(e))
        )
    finally:
        shutil.rmtree(dirpath)


# Remove symlinks as Python wheels do not support them at the moment.
# Rename the remaining dylib to the expected install name (major.minor).
def patch_symlink(folder):
    if sys.platform.startswith("darwin"):
        VERSION_REGEX = re.compile(
            r"^libOpenColorIO.(?P<major>\d+).(?P<minor>\d+).\d+.dylib$")
        NAME_PATTERN = "libOpenColorIO.{major}.{minor}.dylib"
    elif sys.platform.startswith("linux"):
        VERSION_REGEX = re.compile(
            r"^libOpenColorIO.so.(?P<major>\d+).(?P<minor>\d+).\d+$")
        NAME_PATTERN = "libOpenColorIO.so.{major}.{minor}"
    else:
        return

    # First remove all symlinks in the folder
    for f in os.listdir(folder):
        filepath = os.path.join(folder, f)
        if os.path.islink(filepath):
            os.remove(filepath)

    # Then match the remaining OCIO dynamic lib and rename it
    for f in os.listdir(folder):
        filepath = os.path.join(folder, f)
        match = VERSION_REGEX.search(f)
        if match:
            res = match.groupdict()
            new_filename = NAME_PATTERN.format(
                major=res["major"], minor=res["minor"])
            new_filepath = os.path.join(folder, new_filename)
            os.rename(filepath, new_filepath)
            break


# Convert distutils Windows platform specifiers to CMake -A arguments
PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}

# A CMakeExtension needs a sourcedir instead of a file list.
# The name must be the _single_ output extension from the CMake build.
# If you need multiple extensions, see scikit-build.
class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        bindir = os.path.join(extdir, "bin")

        # required for auto-detection & inclusion of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
        cfg = "Debug" if debug else "Release"

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={}".format(extdir),
            "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY={}".format(bindir),
            "-DPython_EXECUTABLE={}".format(sys.executable),
            # Not used on MSVC, but no harm
            "-DCMAKE_BUILD_TYPE={}".format(cfg),
            "-DBUILD_SHARED_LIBS=ON",
            "-DOCIO_BUILD_DOCS=ON",
            "-DOCIO_BUILD_APPS=ON",
            "-DOCIO_BUILD_TESTS=OFF",
            "-DOCIO_BUILD_GPU_TESTS=OFF",
            # Make sure we build everything for the requested architecture(s)
            "-DOCIO_INSTALL_EXT_PACKAGES=ALL",
        ]
        build_args = []
        # Adding CMake arguments set as environment variable
        # (needed e.g. to build for ARM OSx on conda-forge)
        if "CMAKE_ARGS" in os.environ:
            cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

        if self.compiler.compiler_type != "msvc":
            # Using Ninja-build since it a) is available as a wheel and b)
            # multithreads automatically. MSVC would require all variables be
            # exported for Ninja to pick it up, which is a little tricky to do.
            # Users can override the generator with CMAKE_GENERATOR in CMake
            # 3.15+.
            if not cmake_generator:
                try:
                    import ninja  # noqa: F401

                    cmake_args += ["-GNinja"]
                except ImportError:
                    pass

        else:

            # Single config generators are handled "normally"
            single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})

            # CMake allows an arch-in-generator style for backward compatibility
            contains_arch = any(x in cmake_generator for x in {"ARM", "Win64"})

            # Specify the arch if using MSVC generator, but only if it doesn't
            # contain a backward-compatibility arch spec already in the
            # generator name.
            if not single_config and not contains_arch:
                cmake_args += ["-A", PLAT_TO_CMAKE[self.plat_name]]

            # Multi-config generators have a different way to specify configs
            if not single_config:
                cmake_args += [
                    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), extdir),
                    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), bindir),
                ]
                build_args += ["--config", cfg]

        if sys.platform.startswith("darwin"):
            # Cross-compile support for macOS - respect ARCHFLAGS if set
            archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
            if archs:
                cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]

        # When building the wheel, the install step is not executed so we need
        # to have the correct RPATH directly from the build tree output.
        cmake_args += ["-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON"]
        if sys.platform.startswith("linux"):
            cmake_args += ["-DCMAKE_INSTALL_RPATH={}".format("$ORIGIN;$ORIGIN/..")]
        if sys.platform.startswith("darwin"):
            cmake_args += ["-DCMAKE_INSTALL_RPATH={}".format("@loader_path;@loader_path/..")]

        # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
        # across all generators.
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            # self.parallel is a Python 3 only way to set parallel jobs by hand
            # using -j in the build_ext call, not supported by pip or PyPA-build.
            if hasattr(self, "parallel") and self.parallel:
                # CMake 3.12+ only.
                build_args += ["-j{}".format(self.parallel)]

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp
        )
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, cwd=self.build_temp
        )

        patch_symlink(extdir)


# For historical reason, we use PyOpenColorIO as the import name
setup(
    version=get_version(),
    package_dir={
        'PyOpenColorIO': 'src/bindings/python/package',
        'PyOpenColorIO.bin.pyocioamf': 'src/apps/pyocioamf',
        'PyOpenColorIO.bin.pyociodisplay': 'src/apps/pyociodisplay',
    },
    packages=[
        'PyOpenColorIO',
        'PyOpenColorIO.bin.pyocioamf',
        'PyOpenColorIO.bin.pyociodisplay',
    ],
    ext_modules=[CMakeExtension("PyOpenColorIO.PyOpenColorIO")],
    cmdclass={"build_ext": CMakeBuild},
    include_package_data=True,
    entry_points={
        'console_scripts': [
            # Native applications
            'ocioarchive=PyOpenColorIO.command_line:main',
            'ociobakelut=PyOpenColorIO.command_line:main',
            'ociocheck=PyOpenColorIO.command_line:main',
            'ociochecklut=PyOpenColorIO.command_line:main',
            'ocioconvert=PyOpenColorIO.command_line:main',
            'ociodisplay=PyOpenColorIO.command_line:main',
            'ociolutimage=PyOpenColorIO.command_line:main',
            'ociomakeclf=PyOpenColorIO.command_line:main',
            'ocioperf=PyOpenColorIO.command_line:main',
            'ociowrite=PyOpenColorIO.command_line:main',
            # Python applications
            'pyocioamf=PyOpenColorIO.bin.pyocioamf.pyocioamf:main',
            'pyociodisplay=PyOpenColorIO.bin.pyociodisplay.pyociodisplay:main',
        ]
    },
)
