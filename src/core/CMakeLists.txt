
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
)

file( GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp" )
file( GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h" )

add_library(OpenColorIO SHARED ${core_src_files} )

install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)
