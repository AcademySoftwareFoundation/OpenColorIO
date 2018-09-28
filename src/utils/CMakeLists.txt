if(TARGET test_gpu_exec OR 
   TARGET ociodisplay
)
	add_subdirectory(openglbuilder)
endif()
add_subdirectory(pystring)
