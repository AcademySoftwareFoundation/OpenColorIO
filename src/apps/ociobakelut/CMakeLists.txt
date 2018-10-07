set(SOURCES
	main.cpp
	ocioicc.cpp
)

add_executable(ociobakelut ${SOURCES})
target_link_libraries(ociobakelut PRIVATE OpenColorIO lcms2 apputils)

install(TARGETS ociobakelut
	RUNTIME DESTINATION bin
)
