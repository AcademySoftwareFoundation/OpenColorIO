set(SOURCES
    main.cpp
    ocioicc.cpp
)

add_executable(ociobakelut ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociobakelut
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

set_target_properties(ociobakelut PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociobakelut 
    PRIVATE 
        OpenColorIO
        lcms2
        apputils
)

install(TARGETS ociobakelut
    RUNTIME DESTINATION bin
)
