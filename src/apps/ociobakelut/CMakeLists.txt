
file(GLOB_RECURSE share_src_files "${CMAKE_SOURCE_DIR}/src/apps/share/*.cpp")

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/src/apps/share/
)

add_executable(ociobakelut ${share_src_files} main.cpp)
target_link_libraries(ociobakelut OpenColorIO)
install(TARGETS ociobakelut DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
