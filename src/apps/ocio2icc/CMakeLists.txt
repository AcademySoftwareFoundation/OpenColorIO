
# LCMS
#set(LCMS_VERSION 2.1)
#ExternalProject_Add(LCMS
#    URL ${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.tar.gz
#    BUILD_IN_SOURCE 1
#    CONFIGURE_COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/ext/dist --without-jpeg --without-tiff --without-zlib
#    BUILD_COMMAND make
#    INSTALL_COMMAND make install
#)
#set(LCMS_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
#set(LCMS_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
#set(LCMS_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/liblcms2.a)

file(GLOB_RECURSE share_src_files "${CMAKE_SOURCE_DIR}/src/apps/share/*.cpp")

# Build
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${CMAKE_SOURCE_DIR}/src/apps/share/
    ${LCMS_INCLUDE_DIRS}
)

add_executable(ocio2icc
    ${share_src_files}
    main.cpp)

add_dependencies(ocio2icc LCMS)

target_link_libraries(ocio2icc
                      ${LCMS_STATIC_LIBRARIES}
                      OpenColorIO)

install(TARGETS ocio2icc DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
