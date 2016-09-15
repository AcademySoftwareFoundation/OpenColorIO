# LCMS





###############################################################################
### LCMS ###

if(USE_EXTERNAL_LCMS)
    # Set minimum LCMS version for non-patched sources.
    set(LCMS_VERSION_MIN "0.0.0")
    find_package(LCMS)
    if(LCMS_FOUND)
        if(LCMS_VERSION VERSION_EQUAL ${LCMS_VERSION_MIN} OR
           LCMS_VERSION VERSION_GREATER ${LCMS_VERSION_MIN})
            message(STATUS "System LCMS-cpp library will be used.")
            include_directories(BEFORE ${LCMS_INCLUDE_DIRS})
        else()
            message(FATAL_ERROR "ERROR: LCMS-cpp ${LCMS_VERSION_MIN} or greater is required.")
        endif()
    else(LCMS_FOUND)
        message(FATAL_ERROR "ERROR: System LCMS-cpp library was not found. Make sure the library is installed and the pkg-config file exists.")
    endif(LCMS_FOUND)

else(USE_EXTERNAL_LCMS)

    include(FindPkgConfig FindPackageMessage)
    pkg_check_modules(LCMS QUIET lcms2)
    if(LCMS_FOUND AND (LCMS_VERSION VERSION_EQUAL 2.1 OR LCMS_VERSION VERSION_GREATER 2.1))
        FIND_PACKAGE_MESSAGE(LCMS "Found lcms: ${LCMS_LIBRARIES}"
            "${LCMS_INCLUDE_DIR}")
    else()
        message(STATUS "Using bundled lcms.")
        set(LCMS_VERSION 2.1)
        set(LCMS_BUNDLED TRUE)
        ExternalProject_Add(LCMS
            URL ${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.tar.gz
            BUILD_IN_SOURCE 1
            CONFIGURE_COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/ext/dist --without-jpeg --without-tiff --without-zlib
            BUILD_COMMAND make
            INSTALL_COMMAND make install
        )
        set(LCMS_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
        set(LCMS_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
        set(LCMS_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/liblcms2.a)
    endif()

endif(USE_EXTERNAL_LCMS)



file(GLOB_RECURSE share_src_files "${CMAKE_SOURCE_DIR}/src/apps/share/*.cpp")

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/src/apps/share/
    ${LCMS_INCLUDE_DIRS}
)

link_directories(
    ${LCMS_LIBRARY_DIRS}
)

add_executable(ociobakelut ${share_src_files} main.cpp ocioicc.cpp)

if(LCMS_BUNDLED)
    add_dependencies(ociobakelut LCMS)
endif()

# LCMS
target_link_libraries(ociobakelut ${LCMS_LIBRARIES})

# TinyXML
if(USE_EXTERNAL_TINYXML)
    target_link_libraries(ociobakelut ${TINYXML_LIBRARIES})
else(USE_EXTERNAL_TINYXML)
    add_dependencies(ociobakelut tinyxml)
endif(USE_EXTERNAL_TINYXML)

# YAML
if(USE_EXTERNAL_YAML)
    target_link_libraries(ociobakelut ${YAML_CPP_LIBRARIES})
else(USE_EXTERNAL_YAML)
    add_dependencies(ociobakelut YAML_CPP_LOCAL)
endif(USE_EXTERNAL_YAML)

# OCIO
if(OCIO_BUILD_STATIC)
    add_definitions(-DOCIO_BUILD_STATIC)
    target_link_libraries(ociobakelut OpenColorIO_STATIC)
else(OCIO_BUILD_STATIC)
    target_link_libraries(ociobakelut OpenColorIO)
endif(OCIO_BUILD_STATIC)

install(TARGETS ociobakelut DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
