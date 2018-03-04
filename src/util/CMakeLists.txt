set(SOURCES
	argparse.cpp
	strutil.cpp
)

add_library(util STATIC ${SOURCES})
target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
