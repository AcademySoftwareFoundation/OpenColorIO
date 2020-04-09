# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. GPU disabled for ocioconvert.")
else()
    add_compile_definitions(OCIO_GPU_ENABLED)
    set(OCIO_GL_LIB oglapphelpers)
endif()

set(SOURCES
    main.cpp
)

add_executable(ocioconvert ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ocioconvert
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(ocioconvert PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ocioconvert
    PRIVATE
        apputils
        ilmbase::ilmbase
        ${OCIO_GL_LIB}
        oiiohelpers
        OpenColorIO
        OpenImageIO
)

install(TARGETS ocioconvert
    RUNTIME DESTINATION bin
)

