# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# Utility for replicating the behavior of CMake's configure_file command 
# for *.in files when CMake is unavailable. Variable values are extracted from 
# the root CMakeLists.txt file using regular expressions.
#
# -----------------------------------------------------------------------------

import logging
import os
import re


logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

MODULE = os.path.realpath(__file__)
HERE = os.path.dirname(MODULE)
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))


def _get_cmake_values(pattern, what):
    """Search root CMakeLists.txt module for the given pattern(s)."""
    values = None
    cmake_path = os.path.join(ROOT, "CMakeLists.txt")

    with open(cmake_path, "r") as cmake_file:
        for line in cmake_file:
            match = re.match(pattern, line)
            if match:
                values = match.groups()
                break

    if not values:
        raise RuntimeError("Failed to get '{}' value from '{}'!".format(
            what, 
            cmake_path
        ))

    return values


def get_project_name():
    values = _get_cmake_values(
        r"^\s*project\((\w+)\s*",
        "CMAKE_PROJECT_NAME"
    )
    return values[0]


def get_project_version():
    values = _get_cmake_values(
        r"^\s*VERSION (\d+)\.(\d+)\.(\d+)\s*",
        "OpenColorIO_VERSION/CMAKE_PROJECT_VERSION"
    )
    major = values[0]
    minor = values[1]
    patch = values[2]
    version = "{}.{}.{}".format(major, minor, patch)
    return version, major, minor, patch


def get_project_description():
    values = _get_cmake_values(
        r"^\s*DESCRIPTION \"(.*)\"\s*",
        "CMAKE_PROJECT_DESCRIPTION"
    )
    return values[0]


def get_ocio_version_release_type():
    values = _get_cmake_values(
        r"^\s*set\(OpenColorIO_VERSION_RELEASE_TYPE \"(.*)\"",
        "OpenColorIO_VERSION_RELEASE_TYPE"
    )
    return values[0]


def get_ocio_namespace(major=None, minor=None, release_type=None):
    """
    .. note::
        This function will need to be updated if the OCIO_NAMESPACE 
        format changes. There may also be multiple formats present in the 
        module depending on user CMake configuration. We only care about
        the first here, which _should_ be the default.
    """
    if not all((major, minor)):
        _, major, minor, _ = get_project_version()
    if not release_type:
        release_type = get_ocio_version_release_type()

    values = _get_cmake_values(
        r"^\s*set\(OCIO_NAMESPACE \"(.*)\"",
        "OCIO_NAMESPACE"
    )
    namespace_fmt = values[0]
    namespace = namespace_fmt\
        .replace('${OpenColorIO_VERSION_MAJOR}', major)\
        .replace('${OpenColorIO_VERSION_MINOR}', minor)\
        .replace('${OpenColorIO_VERSION_RELEASE_TYPE}', release_type)
    return namespace


def configure_file(src_filename, dst_filename):
    """
    Substitute common @<var>@ variables in the given file. Support for each 
    substitution must be added explicitly.
    """
    logger.info("Configuring file: {} -> {}".format(src_filename, dst_filename))

    # PROJECT_SOURCE_DIR
    # PROJECT_BINARY_DIR
    source_dir = binary_dir = ROOT

    # CMAKE_PROJECT_NAME
    name = get_project_name()

    # CMAKE_PROJECT_VERSION
    # OpenColorIO_VERSION
    # OpenColorIO_VERSION_MAJOR
    # OpenColorIO_VERSION_MINOR
    # OpenColorIO_VERSION_PATCH
    version, major, minor, patch = get_project_version()

    # CMAKE_PROJECT_DESCRIPTION
    desc = get_project_description()

    # OpenColorIO_VERSION_RELEASE_TYPE
    release_type = get_ocio_version_release_type()

    # OCIO_NAMESPACE
    namespace = get_ocio_namespace(
        major=major, 
        minor=minor,
        release_type=release_type
    )

    with open(src_filename, "r") as f:
        data = f.read()

    data = data\
        .replace("@PROJECT_SOURCE_DIR@", source_dir)\
        .replace("@PROJECT_BINARY_DIR@", binary_dir)\
        .replace("@CMAKE_PROJECT_NAME@", name)\
        .replace("@CMAKE_PROJECT_VERSION@", version)\
        .replace("@CMAKE_PROJECT_DESCRIPTION@", desc)\
        .replace("@OpenColorIO_VERSION@", version)\
        .replace("@OpenColorIO_VERSION_MAJOR@", major)\
        .replace("@OpenColorIO_VERSION_MINOR@", minor)\
        .replace("@OpenColorIO_VERSION_PATCH@", patch)\
        .replace("@OpenColorIO_VERSION_RELEASE_TYPE@", release_type)\
        .replace("@OCIO_NAMESPACE@", namespace)

    with open(dst_filename, "w") as f:
        f.write(data)
