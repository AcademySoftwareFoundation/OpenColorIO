set(SOURCES
	pystring.cpp
)

add_library(pystring STATIC ${SOURCES})
set_target_properties(pystring PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(pystring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(pystring public_api)
