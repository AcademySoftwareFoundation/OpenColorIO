###############################################################################
### CORE GPU UNIT TESTS ###

#
# The GPU unit tests are based on the black box approach which means that
# the framework is exclusively using the OpenColorIO dynanic library.
#

add_definitions("-DOCIO_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

set( CMAKE_UNIT_TEST_FILES_DIRECTORY ${CMAKE_BINARY_DIR}/unittestsfiles )
add_definitions("-DOCIO_UNIT_TEST_FILES_DIR=${CMAKE_UNIT_TEST_FILES_DIRECTORY}")

include_directories(
    ${CMAKE_SOURCE_DIR}/src/core_gpu_tests
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_SOURCE_DIR}/src/lang-glsl/
    ${CMAKE_BINARY_DIR}/export/
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${GLEW_INCLUDES}
    )

set(EXTERNAL_COMPILE_FLAGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

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

    # Mute a design issue where the Exception public class inherits 
    # from a STL Exception. STL classes are never supposed to
    # be exported among different dynamic libraries.
    set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} /wd4275")

endif()

file( GLOB_RECURSE core_test_src_files "${CMAKE_SOURCE_DIR}/src/core_gpu_tests/*.cpp" )
add_executable(ocio_core_gpu_tests ${core_test_src_files})

set_target_properties(ocio_core_gpu_tests PROPERTIES COMPILE_FLAGS ${EXTERNAL_COMPILE_FLAGS})

target_link_libraries(ocio_core_gpu_tests OCIOOpenGL ${GLUT_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY})

target_link_OCIO(ocio_core_gpu_tests)


###############################################################################
### CTEST ###

set(OCIO_TEST_AREA ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
    message(STATUS "Create ocio_core_tests.sh.in from ocio_core_tests.sh")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh @ONLY)

    add_custom_target(MainGPUTests
                      COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.sh
                      DEPENDS ocio_core_gpu_tests OpenColorIO
                      COMMENT "Setting Up and Running OCIO GPU UNIT tests")
else()

    message(STATUS "Create ocio_core_tests.bat.in from ocio_core_tests.bat")

    if(MSVC_IDE)
        set( MY_PATH ${CMAKE_BINARY_DIR}/src/core/${CMAKE_BUILD_TYPE} )
    else()
        set( MY_PATH ${CMAKE_BINARY_DIR}/src/core )
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ocio_core_tests.bat.in
        ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat)

    add_custom_target(MainGPUTests
                      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ocio_core_tests.bat
                      DEPENDS ocio_core_gpu_tests OpenColorIO
                      COMMENT "Setting Up and Running OCIO GPU UNIT tests")
endif()

add_test(ocio_core_gpu_tests "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target MainGPUTests)
