# LCMS
include(FindPkgConfig FindPackageMessage)
pkg_check_modules(LCMS QUIET lcms2)
if(LCMS_FOUND AND (LCMS_VERSION VERSION_EQUAL 2.1 OR LCMS_VERSION VERSION_GREATER 2.1))
    FIND_PACKAGE_MESSAGE(LCMS "Found lcms: ${LCMS_LIBRARIES}"
        "${LCMS_INCLUDE_DIR}")
else()
    find_package(Git)
    if(NOT GIT_EXECUTABLE)
        message("Git not found, could not build external LCMS as we cannot apply patch")
    endif()
    message(STATUS "Using bundled lcms.")
    set(LCMS_VERSION 2.1)
    set(LCMS_BUNDLED TRUE)
    if(CMAKE_TOOLCHAIN_FILE)
        set(LCMS_CMAKE_ARGS ${LCMS_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    set(LCMS_INSTALL_DIR ${CMAKE_BINARY_DIR}/ext/dist)
    ExternalProject_Add(LCMS
	PREFIX 		${CMAKE_BINARY_DIR}/lcms2-prefix
        URL 		${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.tar.gz
	PATCH_COMMAND 	${GIT_EXECUTABLE} apply ${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.patch
        BINARY_DIR 	${CMAKE_BINARY_DIR}/ext/build/lcms2
        INSTALL_DIR 	${LCMS_INSTALL_DIR}
        CMAKE_ARGS 	${LCMS_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${LCMS_INSTALL_DIR}
    )
    set(LCMS_INCLUDE_DIRS 	${PROJECT_BINARY_DIR}/ext/dist/include)
    set(LCMS_LIBRARY_DIRS 	${PROJECT_BINARY_DIR}/ext/dist/lib)
    set(LCMS_LIBRARIES 		${PROJECT_BINARY_DIR}/ext/dist/lib/lcms2${CMAKE_STATIC_LIBRARY_SUFFIX})
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

target_link_libraries(ociobakelut ${LCMS_LIBRARIES})
target_link_OCIO(ociobakelut)

install(TARGETS ociobakelut EXPORT OpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/bin)
