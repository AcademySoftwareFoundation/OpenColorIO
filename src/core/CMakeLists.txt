
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
)

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h")

message(STATUS "Create OpenColorVersion.h from OpenColorVersion.h.in")
configure_file(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorVersion.h.in
    ${CMAKE_BINARY_DIR}/export/OpenColorVersion.h @ONLY)
list(APPEND core_export_headers ${CMAKE_BINARY_DIR}/export/OpenColorVersion.h)

add_library(OpenColorIO SHARED ${core_src_files})
install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# TODO: RegisterFileFormat() mechanism doesn't seem to work when static linked?
#add_library(OpenColorIOStatic STATIC ${core_src_files})
#set_target_properties(OpenColorIOStatic PROPERTIES OUTPUT_NAME OpenColorIO)
#install(TARGETS OpenColorIOStatic DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)

# pkg-config
message(STATUS "Create OpenColorIO.pc from OpenColorIO.pc.in")
configure_file(${CMAKE_SOURCE_DIR}/export/pkgconfig/OpenColorIO.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/pkgconfig/)
