@echo OFF
setlocal enabledelayedexpansion

rem Build type - Release or Debug
set CMAKE_BUILD_TYPE=Release

rem Vcpkg location
set VCPKG_PATH=
rem OpenColorIO source location
set OCIO_PATH=
rem OpenColorIO build location
rem defaut to %TEMP% directory
set BUILD_PATH=%TEMP%\OCIO\build
set CUSTOM_BUILD_PATH=0
rem OpenColorIO install location
rem default to %TEMP% directory
set INSTALL_PATH=%TEMP%\OCIO\install
set CUSTOM_INSTALL_PATH=0
set BUILD_PATH_OK=n
set INSTALL_PATH_OK=n

rem Python location
set PYTHON_PATH=

rem Microsoft Visual Studio path
set MSVS_PATH=%programfiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build

set DO_CONFIGURE=0

set CMAKE_CONFIGURE_STATUS=Not executed
set CMAKE_BUILD_STATUS=Not executed
set CMAKE_INSTALL_STATUS=Not executed

rem Command line arguments processing
:args_loop
if NOT "%~1"=="" (
    if "%~1"=="--help" (
        goto :help
    )

    if "%~1"=="--vcpkg" (
        set VCPKG_PATH=%~2
    )

    if "%~1"=="--python" (
        set PYTHON_PATH=%~2
    )

    if "%~1"=="--msvs" (
        set MSVS_PATH=%~2
    )

    if "%~1"=="--ocio" (
        set OCIO_PATH=%~2
    )

    if "%~1"=="--b" (
        set BUILD_PATH=%~2
        set CUSTOM_BUILD_PATH=1
    )

    if "%~1"=="--i" (
        set INSTALL_PATH=%~2
        set CUSTOM_INSTALL_PATH=1
    )

    if "%~1"=="--type" (
        set CMAKE_BUILD_TYPE=%~2
    )

    if "%~1"=="--configure" (
        set DO_CONFIGURE=1
    )
    shift
    goto :args_loop
)

rem Testing the path before cmake
IF NOT EXIST "!VCPKG_PATH!" ( 
    echo Could not find Vcpkg. Please provide the location for vcpkg or modify VCPKG_PATH in this script.
    rem The double dash are in quote here because otherwise the echo command thow an error.
    echo "--vcpkg <path>"
    exit /b
)

IF NOT EXIST "!OCIO_PATH!" ( 
    echo Could not find OCIO source. Please provide the location for ocio or modify OCIO_PATH in this script.
    rem The double dash are in quote here because otherwise the echo command thow an error.
    echo "--ocio <path>"
    exit /b
)

if NOT EXIST "!PYTHON_PATH!\python.exe" ( 
    where /q python
    if not ErrorLevel 1 (
        for /f "tokens=* USEBACKQ" %%p in (`python -c "import os, sys; print(os.path.dirname(sys.executable))"`) do SET PYTHON_PATH=%%p
    )
    if NOT EXIST "!PYTHON_PATH!\python.exe" ( 
        echo Could not find Python. Please provide the location for python or modify PYTHON_PATH in this script.
        rem The double dash are in quote here because otherwise the echo command thow an error.
        echo "--python <path>"
        exit /b
    )
)

IF NOT EXIST "!MSVS_PATH!" ( 
    echo Could not find MS Visual Studio. Please provide the location for Microsoft Visual Studio vcvars64.bat or modify MSVS_PATH in the script.
    rem The double dash are in quote here because otherwise the echo command thow an error.
    echo "--msvs <path>"
    echo E.g. C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build
    exit /b
)

if !CUSTOM_BUILD_PATH!==0 (
    echo.
    set /p BUILD_PATH_OK=Default build path [!BUILD_PATH!] is used. Is it ok? [y/n]:
    if !BUILD_PATH_OK!==n (
        echo Please provide the build location.
        rem The double dash are in quote here because otherwise the echo command thow an error.
        echo "--b <path>"
        exit /b
    )
)

if !CUSTOM_INSTALL_PATH!==0 (
    echo.
    set /p INSTALL_PATH_OK=Default install path [!INSTALL_PATH!] is used. Is it ok? [y/n]:
    if !INSTALL_PATH_OK!==n (
        echo Please provide the install location.
        rem The double dash are in quote here because otherwise the echo command thow an error.
        echo "--i <path>"
        exit /b
    )
)

rem Update build and install variables
set BUILD_PATH=!BUILD_PATH!\!CMAKE_BUILD_TYPE!
set INSTALL_PATH=!INSTALL_PATH!\!CMAKE_BUILD_TYPE!

rem ****************************************************************************************************************
rem Setting up the environment using MS Visual Studio batch script
set VCVARS64_PATH="!MSVS_PATH!\vcvars64.bat"
IF NOT EXIST !VCVARS64_PATH! (
    rem Checking for vcvars64.bat script.
    rem !MSVS_PATH! is checked earlier in the script
    echo VCVARS64_PATH=!VCVARS64_PATH! does not exist
    echo Make sure that Microsoft Visual Studio is installed.
    exit /b
)

rem Checking if it was already previously ran. Some issues might happend when ran multiple time.
if not defined DevEnvDir (
    call !VCVARS64_PATH! x64
)

echo *** Important: Note that the build path should be as small as possible. ***
echo *** Important: Python documentation generation could failed because of Maximum Path Length Limitation of Windows. ***

rem Freeglut root directory
set GLUT_ROOT=!VCPKG_PATH!\packages\freeglut_x64-windows
rem Glew root directory
set GLEW_ROOT=!VCPKG_PATH!\packages\glew_x64-windows
rem OpenImageIO root directory
set OPENIMAGEIO_DIR=!VCPKG_PATH!\packages\openimageio_x64-windows

rem Testing the path before cmake
echo Checking OCIO_PATH=!OCIO_PATH!...
IF NOT EXIST "!OCIO_PATH!" ( 
    echo OCIO_PATH=!OCIO_PATH! does not exist
    exit /b
)
echo Checking GLUT_ROOT=%GLUT_ROOT%...
IF NOT EXIST "%GLUT_ROOT%" ( 
    echo GLUT_ROOT=%GLUT_ROOT% does not exist
    exit /b
)
echo Checking GLEW_ROOT=%GLEW_ROOT%...
IF NOT EXIST "%GLEW_ROOT%" ( 
    echo GLEW_ROOT=%GLEW_ROOT% does not exist
    exit /b
)
echo Checking OPENIMAGEIO_DIR=%OPENIMAGEIO_DIR%...
IF NOT EXIST "%OPENIMAGEIO_DIR%" ( 
    echo OPENIMAGEIO_DIR=%OPENIMAGEIO_DIR% does not exist
    exit /b
)
echo Checking PYTHON_PATH="!PYTHON_PATH!"...
IF NOT EXIST "!PYTHON_PATH!" ( 
    echo PYTHON_PATH=!PYTHON_PATH! does not exist
    exit /b
)

if !DO_CONFIGURE!==1 (
    echo Running CMake...
    cmake -B "!BUILD_PATH!"^
        -DCMAKE_INSTALL_PREFIX=!INSTALL_PATH!^
        -DOCIO_INSTALL_EXT_PACKAGES=ALL^
        -DCMAKE_BUILD_TYPE=!CMAKE_BUILD_TYPE!^
        -DGLEW_ROOT="!GLEW_ROOT!"^
        -DGLUT_ROOT="!GLUT_ROOT!"^
        -DOpenImageIO_ROOT="!OPENIMAGEIO_DIR!"^
        -DOCIO_BUILD_PYTHON=ON^
        -DPython_ROOT="!PYTHON_PATH!"^
        -DBUILD_SHARED_LIBS=ON^
        -DOCIO_BUILD_APPS=ON^
        -DOCIO_BUILD_TESTS=ON^
        -DOCIO_BUILD_GPU_TESTS=ON^
        -DOCIO_BUILD_DOCS=OFF^
        -DOCIO_USE_SIMD=ON^
        -DOCIO_WARNING_AS_ERROR=ON^
        -DOCIO_BUILD_JAVA=OFF^
        "!OCIO_PATH!"

    if not ErrorLevel 1 (
        set CMAKE_CONFIGURE_STATUS=Ok
    ) else (
        set CMAKE_CONFIGURE_STATUS=Failed
    )
)

rem Run cmake --build.
if Not "%CMAKE_CONFIGURE_STATUS%"=="Failed" (
    rem Build OCIO
    cmake --build !BUILD_PATH! --config !CMAKE_BUILD_TYPE! --parallel %NUMBER_OF_PROCESSORS%)
    if not ErrorLevel 1 (
        set CMAKE_BUILD_STATUS=Ok
    ) else (
        set CMAKE_BUILD_STATUS=Failed
    )
)

rem Run cmake --install only if cmake --build was successful.
if Not "%CMAKE_BUILD_STATUS%"=="Failed" (
    rem Install OCIO
    cmake --install !BUILD_PATH! --config !CMAKE_BUILD_TYPE!
    if not ErrorLevel 1 (
        set CMAKE_INSTALL_STATUS=Ok
    ) else (
        set CMAKE_INSTALL_STATUS=Failed
    )
)

rem Run ctest only if cmake --build was successful.
if Not "%CMAKE_BUILD_STATUS%"=="Failed" (
    rem Run Tests
    rem Need cmake version >= 3.20 for --test-dir argument
    rem if <= 3.20 --> cd !BUILD_PATH!; ctest -V -C !CMAKE_BUILD_TYPE!
    ctest -V --test-dir "!BUILD_PATH!" -C !CMAKE_BUILD_TYPE!
    rem Not testing %errorlevel% because ctest returns is not reliable (e.g. returns 0 even when a test fails)
)

rem Output summary
echo **************************************************************************************************************
echo **                                                                                                          **
echo ** Summary                                                                                                  **
echo **                                                                                                          **
echo **************************************************************************************************************
echo.** Source location: !OCIO_PATH!
echo.**
echo.** Statuses:                                                                                                
echo.**    Configure: %CMAKE_CONFIGURE_STATUS%                                                                   
echo.**    Build:     %CMAKE_BUILD_STATUS%                                                                       
echo.**    CTest:     See output above summary                                                                          
echo.**    Install:   %CMAKE_INSTALL_STATUS%                                                                     
echo.**                                                                                                          
echo.** Build type:         !CMAKE_BUILD_TYPE!                                                                           
echo.** Build location:     !BUILD_PATH!                                                                             
echo.** Install location:   !INSTALL_PATH!                                                                         
echo.**                                                                                                          
echo **************************************************************************************************************

exit /b 0

:help
echo.
echo **************************************************************************************************************
echo **                                                                                                          **
echo ** Script to help build and install OCIO                                                                    **
echo **                                                                                                          **
echo **************************************************************************************************************
echo Usage: ocio --vcpkg <path to vcpkg> [OPTIONS]...
echo.
echo Please consider modifying the following script variables to reduce the number of options needed:
echo    VCPKG_PATH
echo    BUILD_PATH
echo    INSTALL_PATH
echo    PYTHON_PATH
echo    MSVS_PATH
echo.
echo Mandatory options:
echo --vcpkg        path to an existing vcpkg installation
echo.
echo --ocio         path to OCIO source code
echo.
echo Optional options depending on the environment:
echo --python       Python installation location
echo.
echo --msvs         path where to find vcvars64.bat from Microsoft Visual Studio
echo                Default: C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build
echo.
echo --b            build location
echo                Default: %TEMP%\OCIO\build
echo.
echo --i            installation location
echo                Default: %TEMP%\OCIO\install
echo.
echo --type         Release or Debug
echo                Default: Release
echo --configure    Run cmake configure or not
echo                Default: false
exit /b 0

endlocal