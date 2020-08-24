# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

project(lcms2)

cmake_minimum_required(VERSION 3.10)

include_directories(include)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.c" "src/*.h")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	LIBRARY_OUTPUT_NAME "${PROJECT_NAME}"
	PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include/lcms2
)
