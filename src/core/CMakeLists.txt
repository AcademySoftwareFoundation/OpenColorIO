
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${YAML_CPP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
)

if(Boost_FOUND)
    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
endif()

if(TRUELIGHT_FOUND)
  set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Truelight_INCLUDE_DIR})
  set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} ${Truelight_COMPILE_FLAGS}")
  set(EXTERNAL_LINK_FLAGS "${EXTERNAL_LINK_FLAGS} ${Truelight_LINK_FLAGS}")
  set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Truelight_LIBRARIES})
endif()

include_directories(${EXTERNAL_INCLUDE_DIRS})

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h")

message(STATUS "Create OpenColorVersion.h from OpenColorVersion.h.in")
configure_file(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorVersion.h.in
    ${CMAKE_BINARY_DIR}/export/OpenColorVersion.h @ONLY)
list(APPEND core_export_headers ${CMAKE_BINARY_DIR}/export/OpenColorVersion.h)

# SHARED

add_library(OpenColorIO SHARED ${core_src_files})
add_dependencies(OpenColorIO pystring md5 tinyxml YAML_CPP_LOCAL)
target_link_libraries(OpenColorIO ${EXTERNAL_LIBRARIES})
set_target_properties(OpenColorIO PROPERTIES
    OUTPUT_NAME OpenColorIO
    COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# STATIC

list(REMOVE_ITEM core_src_files ${CMAKE_SOURCE_DIR}/src/core/UnitTest.cpp)
add_library(OpenColorIO_STATIC STATIC ${core_src_files})
add_dependencies(OpenColorIO_STATIC pystring md5 tinyxml YAML_CPP_LOCAL)
target_link_libraries(OpenColorIO_STATIC ${EXTERNAL_LIBRARIES})
set_target_properties(OpenColorIO_STATIC PROPERTIES
    OUTPUT_NAME OpenColorIO
    COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")
install(TARGETS OpenColorIO_STATIC DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

#
if (SOVERSION)
message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
set_target_properties(OpenColorIO PROPERTIES
    VERSION ${OCIO_VERSION}
    SOVERSION ${SOVERSION}
)
set_target_properties(OpenColorIO_STATIC PROPERTIES
    VERSION ${OCIO_VERSION}
    SOVERSION ${SOVERSION}
)
endif ()

# public interface
install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)

# pkg-config
message(STATUS "Create OpenColorIO.pc from OpenColorIO.pc.in")
configure_file(${CMAKE_SOURCE_DIR}/export/pkgconfig/OpenColorIO.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/pkgconfig/)
