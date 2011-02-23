###############################################################################
### CORE UNIT TESTS ###

add_definitions("-DOCIO_UNIT_TEST")
    
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${YAML_CPP_INCLUDE_DIRS}
    )

set(EXTERNAL_INCLUDE_DIRS)
set(EXTERNAL_COMPILE_FLAGS "")
set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARIES)

if(Boost_FOUND)
    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
    set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Boost_LIBRARIES})
endif()

if(TRUELIGHT_FOUND)
  set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Truelight_INCLUDE_DIR})
  set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} ${Truelight_COMPILE_FLAGS}")
  set(EXTERNAL_LINK_FLAGS "${EXTERNAL_LINK_FLAGS} ${Truelight_LINK_FLAGS}")
  set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Truelight_LIBRARIES})
endif()

include_directories(${EXTERNAL_INCLUDE_DIRS})

file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )

add_executable(ocio_core_tests
    ${core_test_src_files})
    
add_dependencies(ocio_core_tests
    YAML_CPP_LOCAL)

set_target_properties(ocio_core_tests PROPERTIES
    COMPILE_FLAGS "-fPIC -fvisibility-inlines-hidden -fvisibility=hidden ${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")

# Linking to OpenColorIO is not appropriate, as we are going to recompile all the
# src files with OCIO_UNIT_TEST defined.
target_link_libraries(ocio_core_tests
    ${YAML_CPP_STATIC_LIBRARIES}
    ${EXTERNAL_LIBRARIES})

###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

add_test(NAME ocio_core_tests
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh)
