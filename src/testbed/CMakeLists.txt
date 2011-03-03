
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
)

add_executable(OpenColorIOTest
    main.cpp
)

target_link_libraries(OpenColorIOTest OpenColorIO)

###############################################################################
### CTEST ###

message(STATUS "Create OpenColorIOTest.sh.in from OpenColorIOTest.sh")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/OpenColorIOTest.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIOTest.sh @ONLY)

add_test(NAME OpenColorIOTest
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIOTest.sh)
