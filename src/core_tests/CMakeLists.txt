###############################################################################
### CORE UNIT TESTS ###

add_definitions("-DOCIO_UNIT_TEST")
add_definitions("-DOCIO_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

set( CMAKE_UNIT_TEST_FILES_DIRECTORY ${CMAKE_BINARY_DIR}/unittestsfiles )
add_definitions("-DOCIO_UNIT_TEST_FILES_DIR=${CMAKE_UNIT_TEST_FILES_DIRECTORY}")

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
    ${CMAKE_SOURCE_DIR}/ext/sampleicc/src/include
    ${EXTERNAL_INCLUDE_DIRS}
	${CMAKE_BINARY_DIR}/ext/ilmbasehalf
    )

if(WIN32)
    # Hack to get around the "Debug" and "Release" sub directories 
    #  cmake tries to add with Windows VC+++ IDE

    # Set output path
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    # For multi-config builds (e.g. msvc)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR} )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR} )
        set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR} )
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()


## do not link with OpenColorIO since we reuse the sources files to build this app (avoid double link definition)
file( GLOB_RECURSE core_test_src_files 
	${CMAKE_SOURCE_DIR}/src/core/*.cpp
	${CMAKE_BINARY_DIR}/ext/ilmbasehalf/half.cpp)

add_executable(ocio_core_tests ${EXTERNAL_OBJECTS} ${core_test_src_files})

set_target_properties(ocio_core_tests PROPERTIES COMPILE_FLAGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

if(USE_EXTERNAL_TINYXML)
    target_link_libraries(ocio_core_tests ${TINYXML_LIBRARIES})
else(USE_EXTERNAL_TINYXML)
    add_dependencies(ocio_core_tests TINYXML_LIB)
endif(USE_EXTERNAL_TINYXML)

if(USE_EXTERNAL_YAML)
    target_link_libraries(ocio_core_tests ${YAML_CPP_LIBRARIES})
else(USE_EXTERNAL_YAML)
    add_dependencies(ocio_core_tests YAML_CPP_LIB)
endif(USE_EXTERNAL_YAML)

add_dependencies(ocio_core_tests LIBEXPAT_LIB)

if(EXTERNAL_LIBRARIES)
    target_link_libraries(ocio_core_tests ${EXTERNAL_LIBRARIES})
endif()

set_target_properties(ocio_core_tests PROPERTIES
    COMPILE_FLAGS       "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS          "${EXTERNAL_LINK_FLAGS}"
    ## to avoid import/export link dll incoherences (since we build an app with OpenColorIO sources)
    COMPILE_DEFINITIONS OpenColorIO_STATIC
)

# Extract test files
execute_process(
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_UNIT_TEST_FILES_DIRECTORY}
)
execute_process(
	COMMAND ${CMAKE_COMMAND} -E tar xvz ${CMAKE_SOURCE_DIR}/testdata/unittestsfiles.tar.gz
	WORKING_DIRECTORY ${CMAKE_UNIT_TEST_FILES_DIRECTORY}
)

###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
    message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

    add_custom_target(MainTests
                      COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh
                      DEPENDS ocio_core_tests
                      COMMENT "Setting Up and Running OCIO UNIT tests")
else()
    message(STATUS "Create ocio_core_tests.bat.in from ocio_core_tests.bat")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.bat.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat @ONLY)

    add_custom_target(MainTests
                      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat
                      DEPENDS ocio_core_tests
                      COMMENT "Setting Up and Running OCIO UNIT tests")
endif()

add_test(ocio_core_tests "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target MainTests)
