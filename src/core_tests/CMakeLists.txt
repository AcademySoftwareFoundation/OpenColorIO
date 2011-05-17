###############################################################################
### CORE UNIT TESTS ###

add_definitions("-DOCIO_UNIT_TEST")

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${EXTERNAL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
    )

file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )

add_executable(ocio_core_tests ${core_test_src_files})
add_dependencies(ocio_core_tests tinyxml YAML_CPP_LOCAL LCMS)
set_target_properties(ocio_core_tests PROPERTIES
    COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
target_link_libraries(ocio_core_tests ${EXTERNAL_LIBRARIES})

###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

add_test(NAME ocio_core_tests
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh)
