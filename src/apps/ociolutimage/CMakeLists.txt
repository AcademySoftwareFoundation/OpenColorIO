# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
	main.cpp
)

add_executable(ociolutimage ${SOURCES})

set_target_properties(ociolutimage PROPERTIES
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociolutimage
	PRIVATE
		apputils
		OpenColorIO
		OpenImageIO::OpenImageIO
		utils::strings
)

install(TARGETS ociolutimage
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
