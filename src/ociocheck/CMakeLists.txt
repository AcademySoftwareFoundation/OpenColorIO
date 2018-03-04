set(SOURCES
	main.cpp
)

add_executable(ociocheck ${SOURCES})
target_link_libraries(ociocheck PRIVATE OpenColorIO util)

install(TARGETS ociocheck
	RUNTIME DESTINATION bin
)
