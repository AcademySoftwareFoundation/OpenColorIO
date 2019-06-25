find_package(OpenGL REQUIRED)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()
find_package(GLUT REQUIRED)

set(SOURCES
    main.cpp
)

add_executable(ocioperf ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ocioperf
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(ocioperf PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_include_directories(ocioperf 
    SYSTEM
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIR}
)
target_link_libraries(ocioperf
    PRIVATE 
        apputils
        oglbuilder
        oiiohelpers
        OpenColorIO
        OpenImageIO
        ilmbase
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${GLUT_LIBRARIES}
)
install(TARGETS ocioperf
    RUNTIME DESTINATION bin
)

