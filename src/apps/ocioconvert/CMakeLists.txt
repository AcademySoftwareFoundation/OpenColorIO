if (OIIO_FOUND)
    include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${OIIO_INCLUDES}
    )
    add_executable(ocioconvert main.cpp)
    
    target_link_libraries(ocioconvert ${OIIO_LIBRARIES} OpenColorIO dl)
    
    install(TARGETS ocioconvert DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()
