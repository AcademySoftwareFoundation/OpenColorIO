# LCMS
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

target_link_libraries(ociobakelut
                      ${LCMS_LIBRARIES}
                      OpenColorIO)

install(TARGETS ociobakelut DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
