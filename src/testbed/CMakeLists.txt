
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
