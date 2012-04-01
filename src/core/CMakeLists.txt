###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${EXTERNAL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
)

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColourIO/*.h")

message(STATUS "Create OpenColourABI.h from OpenColourABI.h.in")
configure_file(${CMAKE_SOURCE_DIR}/export/OpenColourIO/OpenColourABI.h.in
    ${CMAKE_BINARY_DIR}/export/OpenColourABI.h @ONLY)
list(APPEND core_export_headers ${CMAKE_BINARY_DIR}/export/OpenColourABI.h)

# SHARED

if(OCIO_BUILD_SHARED)
    add_library(OpenColourIO SHARED ${core_src_files})
    add_dependencies(OpenColourIO tinyxml YAML_CPP_LOCAL)
    if(WIN32)
        target_link_libraries(OpenColourIO
            debug ${EXTERNAL_DEBUG_LIBRARIES}
            optimized ${EXTERNAL_OPTIMIZED_LIBRARIES}
            general ${EXTERNAL_GENERAL_LIBRARIES})
    else()
        target_link_libraries(OpenColourIO ${EXTERNAL_GENERAL_LIBRARIES})
    endif()
    set_target_properties(OpenColourIO PROPERTIES
        OUTPUT_NAME OpenColourIO
        COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
    
    message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(OpenColourIO PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})
    
    install(TARGETS OpenColourIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib${LIB_SUFFIX})
endif()

# STATIC

if(OCIO_BUILD_STATIC)
    list(REMOVE_ITEM core_src_files ${CMAKE_SOURCE_DIR}/src/core/UnitTest.cpp)
    add_library(OpenColourIO_STATIC STATIC ${core_src_files})
    add_dependencies(OpenColourIO_STATIC tinyxml YAML_CPP_LOCAL)
    if(WIN32)
        target_link_libraries(OpenColourIO_STATIC
            debug ${EXTERNAL_DEBUG_LIBRARIES}
            optimized ${EXTERNAL_OPTIMIZED_LIBRARIES}
            general ${EXTERNAL_GENERAL_LIBRARIES})
    else()
        target_link_libraries(OpenColourIO_STATIC ${EXTERNAL_GENERAL_LIBRARIES})
    endif()
    set_target_properties(OpenColourIO_STATIC PROPERTIES
        OUTPUT_NAME OpenColourIO
        COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
    
    message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(OpenColourIO_STATIC PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})
    
    install(TARGETS OpenColourIO_STATIC DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib)
endif()

# public interface
install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColourIO/)

# pkg-config
message(STATUS "Create OpenColourIO.pc from OpenColourIO.pc.in")
configure_file(${CMAKE_SOURCE_DIR}/export/pkgconfig/OpenColourIO.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColourIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColourIO.pc
    DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib${LIB_SUFFIX}/pkgconfig/)
