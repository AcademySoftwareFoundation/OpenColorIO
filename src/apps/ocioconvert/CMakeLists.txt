if (OIIO_FOUND)
    include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${CMAKE_SOURCE_DIR}/src/apps/share/
        ${OIIO_INCLUDES}
    )
    
    file(GLOB_RECURSE share_src_files "${CMAKE_SOURCE_DIR}/src/apps/share/*.cpp")
    
    add_executable(ocioconvert ${share_src_files} main.cpp)
    
    target_link_libraries(ocioconvert ${OIIO_LIBRARIES} OpenColorIO ${CMAKE_DL_LIBS})
    
    install(TARGETS ocioconvert DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
endif()
