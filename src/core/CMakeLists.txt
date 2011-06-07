
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${EXTERNAL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
)

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h")

message(STATUS "Create OpenColorABI.h from OpenColorABI.h.in")
configure_file(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorABI.h.in
    ${CMAKE_BINARY_DIR}/export/OpenColorABI.h @ONLY)
list(APPEND core_export_headers ${CMAKE_BINARY_DIR}/export/OpenColorABI.h)

# SHARED

if(OCIO_BUILD_SHARED)
    add_library(OpenColorIO SHARED ${core_src_files})
    add_dependencies(OpenColorIO tinyxml YAML_CPP_LOCAL)
    target_link_libraries(OpenColorIO ${EXTERNAL_LIBRARIES})
    set_target_properties(OpenColorIO PROPERTIES
        OUTPUT_NAME OpenColorIO
        COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
    install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib)
endif()

# STATIC

if(OCIO_BUILD_STATIC)
    list(REMOVE_ITEM core_src_files ${CMAKE_SOURCE_DIR}/src/core/UnitTest.cpp)
    add_library(OpenColorIO_STATIC STATIC ${core_src_files})
    add_dependencies(OpenColorIO_STATIC tinyxml YAML_CPP_LOCAL)
    target_link_libraries(OpenColorIO_STATIC ${EXTERNAL_LIBRARIES})
    set_target_properties(OpenColorIO_STATIC PROPERTIES
        OUTPUT_NAME OpenColorIO
        COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
    install(TARGETS OpenColorIO_STATIC DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib)
endif()

#
if (SOVERSION)
    message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(OpenColorIO PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})
    set_target_properties(OpenColorIO_STATIC PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})
endif()

# public interface
install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)

# pkg-config
message(STATUS "Create OpenColorIO.pc from OpenColorIO.pc.in")
configure_file(${CMAKE_SOURCE_DIR}/export/pkgconfig/OpenColorIO.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc
    DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/include/pkgconfig/)
