
if( NUKE_FOUND )
    
    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
    endif()
    
    include_directories(
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ColorSpace
        Display
        ${Nuke_INCLUDE_DIR}
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
    ### Nuke Targets ###
    
    add_custom_target(Nuke
        DEPENDS NukeOCIOColorSpace
        DEPENDS NukeOCIODisplay
    )
    install(TARGETS NukeOCIOColorSpace DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke)
    install(TARGETS NukeOCIODisplay DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke)
    
endif()
