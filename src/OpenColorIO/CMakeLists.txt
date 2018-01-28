###############################################################################
### OCIO CORE ###

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/ext/oiio/src/include
    ${CMAKE_SOURCE_DIR}/ext/sampleicc/src/include
    ${EXTERNAL_INCLUDE_DIRS}
)

file(GLOB_RECURSE core_src_files "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE core_export_headers "${CMAKE_SOURCE_DIR}/export/OpenColorIO/*.h")

message(STATUS "Create OpenColorABI.h from OpenColorABI.h.in")
configure_file(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorABI.h.in
    ${CMAKE_BINARY_DIR}/export/OpenColorABI.h @ONLY)
list(APPEND core_export_headers ${CMAKE_BINARY_DIR}/export/OpenColorABI.h)

if(OCIO_WARNING_AS_ERROR)
    if(WIN32)
        # On debug mode there are other kinds of warning...
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
            set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} /WX")
        endif()
    else()
        set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} -Werror")
    endif()
endif()

# SHARED

if(OCIO_BUILD_SHARED)
    add_library(OpenColorIO SHARED ${EXTERNAL_OBJECTS} ${core_src_files})

    if(USE_EXTERNAL_TINYXML)
        target_link_libraries(OpenColorIO ${TINYXML_LIBRARIES})
    else(USE_EXTERNAL_TINYXML)
        add_dependencies(OpenColorIO TINYXML_LIB)
    endif(USE_EXTERNAL_TINYXML)

    if(USE_EXTERNAL_YAML)
        target_link_libraries(OpenColorIO ${YAML_CPP_LIBRARIES})
    else(USE_EXTERNAL_YAML)
        add_dependencies(OpenColorIO YAML_CPP_LIB)
    endif()

    if(EXTERNAL_LIBRARIES)
        target_link_libraries(OpenColorIO ${EXTERNAL_LIBRARIES})
    endif()

    if(WIN32)
        # Mute a design issue where the Exception public class inherits
        # from a STL Exception. STL classes are never supposed to
        # be exported among different dynamic libraries.
        set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} /wd4275")
    endif()

    set_target_properties(OpenColorIO PROPERTIES
        OUTPUT_NAME OpenColorIO
        COMPILE_FLAGS   "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS      "${EXTERNAL_LINK_FLAGS}")

    message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(OpenColorIO PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})

    install(TARGETS OpenColorIO EXPORT OpenColorIO
        ARCHIVE DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib${LIB_SUFFIX}
        LIBRARY DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib${LIB_SUFFIX}
        RUNTIME DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin
    )
endif()

# STATIC

if(OCIO_BUILD_STATIC)
    list(REMOVE_ITEM core_src_files ${CMAKE_SOURCE_DIR}/src/core/UnitTest.cpp)
    add_library(OpenColorIO_STATIC STATIC ${EXTERNAL_OBJECTS} ${core_src_files})
    add_dependencies(OpenColorIO_STATIC TINYXML_LIB YAML_CPP_LIB)
    if(EXTERNAL_LIBRARIES)
        target_link_libraries(OpenColorIO_STATIC ${EXTERNAL_LIBRARIES})
    endif()

	set_target_properties(OpenColorIO_STATIC PROPERTIES COMPILE_DEFINITIONS "OpenColorIO_STATIC")
    set_target_properties(OpenColorIO_STATIC PROPERTIES
        OUTPUT_NAME                 OpenColorIO
        ARCHIVE_OUTPUT_DIRECTORY    "${CMAKE_CURRENT_BINARY_DIR}/static"
        COMPILE_FLAGS               "${EXTERNAL_COMPILE_FLAGS}"
        LINK_FLAGS                  "${EXTERNAL_LINK_FLAGS}")

    message(STATUS "Setting OCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(OpenColorIO_STATIC PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION})

    install(TARGETS OpenColorIO_STATIC EXPORT OpenColorIO ARCHIVE DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/static)
endif()

macro(target_link_OCIO target)
    if(OCIO_BUILD_SHARED)
        target_link_libraries(${target} OpenColorIO         ${ARGN})
    elseif(OCIO_BUILD_STATIC)
        target_link_libraries(${target} OpenColorIO_STATIC  ${ARGN})
        set_target_properties(${target} PROPERTIES COMPILE_DEFINITIONS OpenColorIO_STATIC)
    endif()
endmacro()


# public interface
install(FILES ${core_export_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)

# pkg-config
message(STATUS "Create OpenColorIO.pc from OpenColorIO.pc.in")
configure_file(${CMAKE_SOURCE_DIR}/export/pkgconfig/OpenColorIO.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc
    DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib${LIB_SUFFIX}/pkgconfig/)
