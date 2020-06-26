# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. GPU disabled for ociochecklut.")
else()
    add_compile_definitions(OCIO_GPU_ENABLED)
    set(OCIO_GL_LIB oglapphelpers)
endif()

if(${OCIO_USE_HEADLESS})
    if(${OCIO_EGL_HEADLESS})
        add_compile_definitions(OCIO_HEADLESS_ENABLED)
    else()
        message(WARNING "EGL component missing. Can't render headlessly for ociochecklut.")
    endif()
endif()

set(SOURCES
    main.cpp
)

add_executable(ociochecklut ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociochecklut
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

set_target_properties(ociochecklut PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociochecklut 
    PRIVATE 
        ${OCIO_GL_LIB}
        apputils
        OpenColorIO
)

install(TARGETS ociochecklut
    RUNTIME DESTINATION bin
)

