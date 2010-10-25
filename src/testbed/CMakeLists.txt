
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${Boost_INCLUDE_DIR}
)

add_executable(OpenColorIOTest
    main.cpp
)

target_link_libraries(OpenColorIOTest OpenColorIO)

install(TARGETS OpenColorIOTest DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES
    main.py
    ocio_info.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/scripts)

###############################################################################
### CTEST ###

message(STATUS "Create OpenColorIOTest.sh.in from OpenColorIOTest.sh")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/OpenColorIOTest.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIOTest.sh @ONLY)

add_test(NAME OpenColorIOTest
    COMMAND /bin/sh ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIOTest.sh)
