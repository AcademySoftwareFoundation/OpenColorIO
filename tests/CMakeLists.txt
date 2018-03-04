
if(OCIO_BUILD_TESTS)
	add_subdirectory(cpu)
endif()
if(OCIO_BUILD_GPU_TESTS)
	add_subdirectory(gpu)
endif()
