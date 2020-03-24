# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
	argparse.cpp
	strutil.cpp
)

add_library(apputils STATIC ${SOURCES})
target_include_directories(apputils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
