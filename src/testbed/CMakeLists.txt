
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
)

add_executable(OpenColorIOTest
    main.cpp
)

target_link_libraries(OpenColorIOTest OpenColorIO)

install(TARGETS OpenColorIOTest DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES
    main.py
    ocio_info.py
    spi_to_ocio.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/scripts)
