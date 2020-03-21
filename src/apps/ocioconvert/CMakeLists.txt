# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

find_package(OpenGL REQUIRED)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()
find_package(GLUT REQUIRED)

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

target_include_directories(ocioconvert 
    SYSTEM
    PRIVATE
        ${GLEW_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(ocioconvert
    PRIVATE
        apputils
        ${GLEW_LIBRARIES}
        ${GLUT_LIBRARIES}
        ilmbase::ilmbase
        oglbuilder
        oiiohelpers
        OpenColorIO
        OpenImageIO
        ${OPENGL_LIBRARIES}
)
install(TARGETS ocioconvert
    RUNTIME DESTINATION bin
)

