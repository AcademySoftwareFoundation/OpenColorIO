
include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDES}
)

file(GLOB_RECURSE glsl_src_files "${CMAKE_SOURCE_DIR}/src/lang-glsl/*.cpp")

add_library(OCIOOpenGL STATIC ${glsl_src_files})

target_link_OCIO(OCIOOpenGL)
set_target_properties(OCIOOpenGL PROPERTIES COMPILE_FLAGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

set_target_properties(OCIOOpenGL
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOOpenGL"
)

install(TARGETS OCIOOpenGL EXPORT OpenColorIO 
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/share/lang-glsl/lib)

install(FILES ${CMAKE_SOURCE_DIR}/src/lang-glsl/glsl.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/lang-glsl/include)
