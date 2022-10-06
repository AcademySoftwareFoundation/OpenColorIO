# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
	main.cpp
)

add_executable(ociolutimage ${SOURCES})

set_target_properties(ociolutimage PROPERTIES
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

if (USE_MSVC)
	# Temporary until fixed in OpenImageIO: Mute some warnings from OpenImageIO farmhash.h
	# C4267 (level 3)	    'var' : conversion from 'size_t' to 'type', possible loss of data
	# C4244	(level 3 & 4)	'conversion' conversion from 'type1' to 'type2', possible loss of data
	target_compile_options(ociolutimage PRIVATE /wd4267 /wd4244)
endif()

target_link_libraries(ociolutimage
	PRIVATE
		apputils
		imageioapphelpers
		OpenColorIO
		utils::strings
)

install(TARGETS ociolutimage
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
