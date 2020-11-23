# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# Utility for building OCIO_NAMESPACE as defined in the root CMakeLists.txt 
# without needing CMake configuration.
#
# -----------------------------------------------------------------------------

import os
import re


MODULE = os.path.realpath(__file__)
HERE = os.path.dirname(MODULE)
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))

# Variables defined in root CMakeLists.txt
# OCIO version, defined in 'project' call
SET_VERSION_RE = re.compile(
    r'^\s*VERSION (?P<major>\d+).(?P<minor>\d+).\d+'
)
# OCIO release typr, defined in 'set' call
SET_RELEASE_TYPE_RE = re.compile(
    r'^\s*set\(OpenColorIO_VERSION_RELEASE_TYPE "(?P<type>.*)"\)'
)
# Default OCIO namespace format, defined in 'set' call
SET_NAMESPACE_FMT_RE = re.compile(
    r'^\s*set\(OCIO_NAMESPACE "(?P<format>.*)" CACHE'
)


def get_ocio_namespace():
    """
    Search the root CMakeLists.txt module for the OCIO_NAMESPACE 
    definition and reconstruct it.

    .. note::
        This function will need to be changed if the OCIO_NAMESPACE 
        format or its dependent variable definitions change.

    At the time of writing, this is the OCIO_NAMESPACE format being 
    searched for and evaluated::

        OpenColorIO_v${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}${OpenColorIO_VERSION_RELEASE_TYPE}

    """
    version_major = None
    version_minor = None
    release_type = None
    namespace_fmt = None

    cmake_path = os.path.join(ROOT, 'CMakeLists.txt')
    with open(cmake_path, 'r') as cmake_file:
        for line in cmake_file:

            version_match = SET_VERSION_RE.match(line)
            if version_match:
                version_major = version_match.group('major')
                version_minor = version_match.group('minor')
                continue

            release_type_match = SET_RELEASE_TYPE_RE.match(line)
            if release_type_match:
                release_type = release_type_match.group('type')
                continue

            # NOTE: There may be multiple OCIO_NAMESPACE definitions or formats
            #       depending on user CMake configuration. We only care about 
            #       the first, which _should_ be the default.
            namespace_fmt_match = SET_NAMESPACE_FMT_RE.match(line)
            if namespace_fmt_match:
                namespace_fmt = namespace_fmt_match.group('format')
                break

    if not all((version_major, version_minor, release_type, namespace_fmt)):
        raise RuntimeError(
            "Failed to get OCIO_NAMESPACE from '{cmake}'!\n"
            "       major version: {major}\n"
            "       minor version: {minor}\n"
            "        release type: {type}\n"
            "    namespace format: {format}\n".format(
                cmake=cmake_path,
                major=version_major,
                minor=version_minor,
                type=release_type,
                format=namespace_fmt
            )
        )

    namespace = namespace_fmt\
        .replace('${OpenColorIO_VERSION_MAJOR}', version_major)\
        .replace('${OpenColorIO_VERSION_MINOR}', version_minor)\
        .replace('${OpenColorIO_VERSION_RELEASE_TYPE}', release_type)

    return namespace
