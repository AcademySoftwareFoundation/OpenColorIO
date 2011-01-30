###############################################################################
### CORE UNIT TESTS ###

add_definitions("-DOCIO_UNIT_TEST")
    
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${YAML_CPP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
    )
    
file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )
    
link_directories(
    ${Boost_LIBRARY_DIRS})
    
add_executable(ocio_core_tests
    ${core_test_src_files})
    
add_dependencies(ocio_core_tests
    YAML_CPP_LOCAL)
    
# Linking to OpenColorIO is not appropriate,
# as we are going to recompile all the src files with OCIO_UNIT_TEST defined.
target_link_libraries(ocio_core_tests
    ${YAML_CPP_STATIC_LIBRARIES}
    ${Boost_LIBRARIES})

###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

add_test(NAME ocio_core_tests
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh)
