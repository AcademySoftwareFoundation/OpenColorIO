set(SOURCES
	main.cpp
)

add_executable(ociocheck ${SOURCES})
target_link_libraries(ociocheck PRIVATE OpenColorIO apputils)

install(TARGETS ociocheck
	RUNTIME DESTINATION bin
)
