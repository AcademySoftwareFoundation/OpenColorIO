include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${OPENGL_INCLUDE_DIR}
    ${OIIO_INCLUDES}
    ${GLEW_INCLUDES}
	${GLUT_INCLUDE_DIR}
)
add_executable(ociodisplay main.cpp)
# set_target_properties(ociodisplay PROPERTIES INSTALL_RPATH ${OIIO_LIBRARIES} )
target_link_libraries(ociodisplay ${GLEW_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY} ${OIIO_LIBRARIES})
target_link_OCIO(ociodisplay)
install(TARGETS ociodisplay EXPORT OpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
