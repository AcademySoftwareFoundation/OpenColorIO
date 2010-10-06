###############################################################################
### CORE UNIT TESTS ###

if(Boost_FOUND)
    add_definitions("-DOCIO_UNIT_TEST")
    
    include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${Boost_INCLUDE_DIR}
    )
    
    file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )
    
    link_directories( ${Boost_LIBRARY_DIRS} )
    
    add_executable( ocio_core_tests ${core_test_src_files} )
    
    target_link_libraries(ocio_core_tests
    
# Linking to OpenColorIO is not appropriate,
# as we are going to recompile all the src files with OCIO_UNIT_TEST defined.
#        OpenColorIO

        ${Boost_LIBRARIES}
        )
    
    install(TARGETS ocio_core_tests DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    
    ADD_TEST(ocio_core_tests COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests --log_level=test_suite)
    
endif()
