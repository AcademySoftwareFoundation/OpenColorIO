if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/export/
    ${CMAKE_BINARY_DIR}/export/
    ${Nuke_INCLUDE_DIR}
)

###############################################################################
### NukeOCIOColorSpace ###

add_library(NukeOCIOColorSpace MODULE
    OCIOColorSpace/OCIOColorSpace.cpp
)
target_link_libraries(NukeOCIOColorSpace
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIOColorSpace
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOColorSpace"
)

###############################################################################
### NukeOCIODisplay ###

add_library(NukeOCIODisplay MODULE
    OCIODisplay/OCIODisplay.cpp
)
target_link_libraries(NukeOCIODisplay
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIODisplay
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIODisplay"
)

###############################################################################
### NukeOCIOFileTransform ###

add_library(NukeOCIOFileTransform MODULE
    OCIOFileTransform/OCIOFileTransform.cpp
)
target_link_libraries(NukeOCIOFileTransform
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIOFileTransform
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOFileTransform"
)

###############################################################################
### NukeOCIOLogConvert ###

add_library(NukeOCIOLogConvert MODULE
    OCIOLogConvert/OCIOLogConvert.cpp
)
target_link_libraries(NukeOCIOLogConvert
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIOLogConvert
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOLogConvert"
)

###############################################################################
### NukeOCIOLookTransform ###

add_library(NukeOCIOLookTransform MODULE
    OCIOLookTransform/OCIOLookTransform.cpp
)
target_link_libraries(NukeOCIOLookTransform
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIOLookTransform
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOLookTransform"
)

###############################################################################
### NukeOCIOCDLTransform ###

add_library(NukeOCIOCDLTransform MODULE
    OCIOCDLTransform/OCIOCDLTransform.cpp
)
target_link_libraries(NukeOCIOCDLTransform
    OpenColorIO
    #${Nuke_LIBRARIES}
)
set_target_properties(NukeOCIOCDLTransform
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "OCIOCDLTransform"
)

###############################################################################
### Nuke Targets ###

add_custom_target(Nuke
    DEPENDS NukeOCIOColorSpace
    DEPENDS NukeOCIODisplay
    DEPENDS NukeOCIOFileTransform
    DEPENDS NukeOCIOLogConvert
    DEPENDS NukeOCIOCDLTransform
)

install(TARGETS NukeOCIOColorSpace
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(TARGETS NukeOCIODisplay
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(TARGETS NukeOCIOLogConvert
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(TARGETS NukeOCIOFileTransform
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(TARGETS NukeOCIOLookTransform
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(TARGETS NukeOCIOCDLTransform
        DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/nuke${Nuke_API_VERSION})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/share/nuke
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/
        PATTERN .svn EXCLUDE)
