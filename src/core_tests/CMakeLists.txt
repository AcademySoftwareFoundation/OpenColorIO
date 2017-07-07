###############################################################################
### CORE UNIT TESTS ###

add_definitions("-DOCIO_UNIT_TEST")
add_definitions("-DOCIO_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
    ${EXTERNAL_INCLUDE_DIRS}
    )

file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )

add_executable(ocio_core_tests ${core_test_src_files})

if(USE_EXTERNAL_TINYXML)
    target_link_libraries(ocio_core_tests ${TINYXML_LIBRARIES})
else(USE_EXTERNAL_TINYXML)
    add_dependencies(ocio_core_tests tinyxml)
endif(USE_EXTERNAL_TINYXML)

if(USE_EXTERNAL_YAML)
    target_link_libraries(ocio_core_tests ${YAML_CPP_LIBRARIES})
else(USE_EXTERNAL_YAML)
    add_dependencies(ocio_core_tests  YAML_CPP_LOCAL)
endif(USE_EXTERNAL_YAML)

set_target_properties(ocio_core_tests PROPERTIES
    COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
if(WIN32)
    get_target_property(OCIO_STATIC_DEFS ocio_core_tests COMPILE_DEFINITIONS)
    list(APPEND OCIO_STATIC_DEFS OCIO_STATIC=1)
    set_target_properties(
        ocio_core_tests
        PROPERTIES
        COMPILE_DEFINITIONS ${OCIO_STATIC_DEFS}
    )
    target_link_libraries(ocio_core_tests
            debug ${EXTERNAL_DEBUG_LIBRARIES}
            optimized ${EXTERNAL_OPTIMIZED_LIBRARIES}
            general ${EXTERNAL_GENERAL_LIBRARIES})
else()
    target_link_libraries(ocio_core_tests ${EXTERNAL_GENERAL_LIBRARIES})
endif()

###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})



if(WIN32)
    message(STATUS "Create ocio_core_tests.bat.in from ocio_core_tests.bat")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.bat.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat @ONLY)

    add_custom_target(MainTests
                      COMMAND "${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
                      DEPENDS ocio_core_tests
                      COMMENT "Setting Up and Running OCIO UNIT tests")
else()
    message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

    add_custom_target(MainTests
                      COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh
                      DEPENDS ocio_core_tests
                      COMMENT "Setting Up and Running OCIO UNIT tests")
endif()

add_test(ocio_core_tests "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target MainTests)
