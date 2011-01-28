
# LCMS
set(LCMS_VERSION 2.1)
ExternalProject_Add(LCMS
    URL ${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/ext/dist --without-jpeg --without-tiff --without-zlib
    BUILD_COMMAND make
    INSTALL_COMMAND make install
)
set(LCMS_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
set(LCMS_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
set(LCMS_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/liblcms2.a)

# Build
include_directories(
  ${CMAKE_SOURCE_DIR}/export/
  ${CMAKE_BINARY_DIR}/export/
  ${LCMS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
)

add_executable(ociobuildicc main.cpp)

add_dependencies(ociobuildicc LCMS)

target_link_libraries(ociobuildicc
                      ${Boost_LIBRARIES}
                      ${LCMS_STATIC_LIBRARIES}
                      OpenColorIO)

install(TARGETS ociobuildicc DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
