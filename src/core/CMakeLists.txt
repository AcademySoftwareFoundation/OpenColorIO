
###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${YAML_CPP_INCLUDE_DIRS}
)

set(EXTERNAL_INCLUDE_DIRS)
set(EXTERNAL_COMPILE_FLAGS "")
set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARIES)

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

add_library(OpenColorIO SHARED
    ${core_src_files})

add_dependencies(OpenColorIO
    YAML_CPP_LOCAL)

target_link_libraries(OpenColorIO
    ${YAML_CPP_STATIC_LIBRARIES})

target_link_libraries(OpenColorIO
    ${EXTERNAL_LIBRARIES})

set_target_properties(OpenColorIO PROPERTIES
    COMPILE_FLAGS "-fPIC -fvisibility-inlines-hidden -fvisibility=hidden ${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}")

if (SOVERSION)
message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
set_target_properties(OpenColorIO PROPERTIES
    VERSION ${OCIO_VERSION}
    SOVERSION ${SOVERSION}
)
endif ()

install(TARGETS OpenColorIO DESTINATION
    ${CMAKE_INSTALL_PREFIX}/lib)

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
