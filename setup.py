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

        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cfg = "Debug" if self.debug else "Release"

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={}".format(extdir),
            "-DPython_EXECUTABLE={}".format(sys.executable),
            # Not used on MSVC, but no harm
            "-DCMAKE_BUILD_TYPE={}".format(cfg),
            "-DBUILD_SHARED_LIBS=OFF",
            "-DOCIO_BUILD_DOCS=ON",
            "-DOCIO_BUILD_APPS=OFF",
            "-DOCIO_BUILD_TESTS=OFF",
            "-DOCIO_BUILD_GPU_TESTS=OFF",
            # Install pybind11 via pip to avoid issue on Windows where
            # ExternalProject pybind11 build detect the wrong version
            "-DOCIO_INSTALL_EXT_PACKAGES=MISSING",
        ]
        build_args = []

        if self.compiler.compiler_type != "msvc":
            # Using Ninja-build since it a) is available as a wheel and b)
            # multithreads automatically. MSVC would require all variables be
            # exported for Ninja to pick it up, which is a little tricky to do.
            # Users can override the generator with CMAKE_GENERATOR in CMake
            # 3.15+.
            if not cmake_generator:
                cmake_args += ["-GNinja"]

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
                    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), extdir)
                ]
                build_args += ["--config", cfg]

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


# For historical reason, we use PyOpenColorIO as the import name
setup(
    version=get_version(),
    package_dir={
        'PyOpenColorIOTests': 'tests/python',
        'PyOpenColorIOTests.data': 'tests/data',
    },
    packages=['PyOpenColorIOTests', 'PyOpenColorIOTests.data'],
    ext_modules=[CMakeExtension("PyOpenColorIO")],
    cmdclass={"build_ext": CMakeBuild},
    include_package_data=True
)
