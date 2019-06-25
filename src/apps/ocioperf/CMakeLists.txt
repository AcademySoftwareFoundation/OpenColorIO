set(SOURCES
    main.cpp
)

add_executable(ocioperf ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ocioperf
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(ocioperf PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ocioperf
    PRIVATE 
        apputils
        oglbuilder
        oiiohelpers
        OpenColorIO
        OpenImageIO
        ilmbase
)

install(TARGETS ocioperf
    RUNTIME DESTINATION bin
)

