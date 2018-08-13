SET CMAKE_PATH=D:\ocio\3rdParty\cmake-3.9.3
SET GLUT_PATH=D:\ocio\3rdParty\freeglut-3.0.0-2
SET GLEW_PATH=D:\ocio\3rdParty\glew-1.9.0
SET PYTHONPATH=C:\Python27

SET PATH=%GLEW_PATH%\bin;%GLUT_PATH%\bin;%CMAKE_PATH%\bin;%PYTHONPATH%;%PATH%'

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

cd build_dbg

cmake -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=../install_dbg ^
    -DOCIO_BUILD_SHARED=ON ^
    -DOCIO_BUILD_STATIC=ON ^
    -DOCIO_BUILD_APPS=ON ^
    -DOCIO_BUILD_TESTS=ON ^
    -DOCIO_BUILD_GPU_TESTS=ON ^
    -DOCIO_BUILD_DOCS=OFF ^
    -DOCIO_BUILD_PYGLUE=ON ^
    -DCMAKE_PREFIX_PATH=D:\ocio\3rdParty\compiled-dist_dbg\IlmBase-2.2.0 ^
    ..

nmake all
nmake test_verbose
nmake install

cd ..\build_rls

cmake -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=../install_rls ^
    -DOCIO_BUILD_SHARED=ON ^
    -DOCIO_BUILD_STATIC=ON ^
    -DOCIO_BUILD_APPS=ON ^
    -DOCIO_BUILD_TESTS=ON ^
    -DOCIO_BUILD_GPU_TESTS=ON ^
    -DOCIO_BUILD_DOCS=OFF ^
    -DOCIO_BUILD_PYGLUE=ON ^
    -DCMAKE_PREFIX_PATH=D:\ocio\3rdParty\compiled-dist_rls\IlmBase-2.2.0 ^
    ..

nmake all
nmake test_verbose
nmake install

cd ..