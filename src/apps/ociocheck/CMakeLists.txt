set(SOURCES
    main.cpp
)

add_executable(ociocheck ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ociocheck
        PRIVATE
        OpenColorIO_SKIP_IMPORTS
    )
endif()

if(MSVC)
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /wd4996")
endif()

set_target_properties(ociocheck PROPERTIES 
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(ociocheck 
    PRIVATE 
        OpenColorIO
        apputils
)

install(TARGETS ociocheck
    RUNTIME DESTINATION bin
)
