# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(OCIO_BUILD_APPS)
	add_subdirectory(ociobakelut)
	add_subdirectory(ociocheck)
	add_subdirectory(ociochecklut)
	add_subdirectory(ociomakeclf)
	add_subdirectory(ociowrite)

	if(OpenImageIO_FOUND AND TARGET OpenImageIO::OpenImageIO)
		add_subdirectory(ociolutimage)
		add_subdirectory(ocioconvert)
		add_subdirectory(ociodisplay)
		add_subdirectory(ocioperf)
	else()
		message(WARNING "Could NOT find OpenImageIO. Skipping build for ociolutimage, ocioconvert, ociodisplay and ocioperf.")
	endif()
else()
	message(WARNING "Skipping build of all apps.")
endif()