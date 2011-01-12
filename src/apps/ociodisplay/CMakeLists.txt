if (OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND AND OIIO_FOUND)
    include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${OPENGL_INCLUDE_DIR}
        ${OIIO_INCLUDES}
        ${GLEW_INCLUDES}
    )
    add_executable(ociodisplay main.cpp)
    # set_target_properties(ociodisplay PROPERTIES INSTALL_RPATH ${OIIO_LIBRARIES} )
    target_link_libraries(ociodisplay ${GLEW_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY} ${OIIO_LIBRARIES} OpenColorIO)
    install(TARGETS ociodisplay DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()
