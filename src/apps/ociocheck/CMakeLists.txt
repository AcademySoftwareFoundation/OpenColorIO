include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${Boost_INCLUDE_DIR}
    )
add_executable(ociocheck main.cpp)

target_link_libraries(ociocheck
    ${Boost_LIBRARIES}
    OpenColorIO
    )

install(TARGETS ociocheck DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
