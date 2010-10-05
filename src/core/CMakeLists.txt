
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
)

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h")

add_library(OpenColorIO SHARED ${core_src_files})
install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# TODO: RegisterFileFormat() mechanism doesn't seem to work when static linked?
#add_library(OpenColorIOStatic STATIC ${core_src_files})
#set_target_properties(OpenColorIOStatic PROPERTIES OUTPUT_NAME OpenColorIO)
#install(TARGETS OpenColorIOStatic DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)
