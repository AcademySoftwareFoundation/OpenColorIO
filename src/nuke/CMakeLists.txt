
if( NUKE_FOUND )
    
    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
    endif()
    
    include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ColorSpace
        Display
        FileTransform
        LogConvert
        ${Nuke_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
    )
    
    ###############################################################################
    ### NukeOCIOColorSpace ###
    
    add_library(NukeOCIOColorSpace MODULE
        ColorSpace/ColorSpace.cpp
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
        Display/Display.cpp
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
        FileTransform/FileTransform.cpp
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
        LogConvert/LogConvert.cpp
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
    ### Nuke Targets ###
    
    add_custom_target(Nuke
        DEPENDS NukeOCIOColorSpace
        DEPENDS NukeOCIODisplay
        DEPENDS NukeOCIOFileTransform
        DEPENDS NukeOCIOLogConvert
    )
    
    install(TARGETS NukeOCIOColorSpace
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke${Nuke_DDImageVersion})
    
    install(TARGETS NukeOCIODisplay
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke${Nuke_DDImageVersion})
    
    install(TARGETS NukeOCIOLogConvert
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke${Nuke_DDImageVersion})
    
    install(TARGETS NukeOCIOFileTransform
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke${Nuke_DDImageVersion})
    
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/share/nuke
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/
            PATTERN .svn EXCLUDE)
    
endif()
