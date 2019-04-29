if(TARGET test_gpu_exec OR 
   TARGET ociodisplay OR
   TARGET ocioconvert
)
    add_subdirectory(oglbuilder)
endif()

add_subdirectory(pystring)
