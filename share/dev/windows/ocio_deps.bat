@echo off
rem install_pkg_managers
rem Download and install some windows package managers to handle
rem the installations of OCIO dependencies.

setlocal enabledelayedexpansion

rem Check if x64 or x86
set isX64=0 && if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" ( set isX64=1 ) else ( if /I "%PROCESSOR_ARCHITEW6432%"=="AMD64" ( set isX64=1 ) )

set IS_VCPKG_PRESENT=0
set VCPKG_INTEGRATE=0

set IS_OPENIMAGEIO_PRESENT=0
set IS_FREEGLUT_PRESENT=0
set IS_GLEW_PRESENT=0

rem Define flags to indicate if automated install or not.
set AUTO_VCPKG=n
set AUTO_OPENIMAGEIO=n
set AUTO_FREEGLUT=n
set AUTO_GLEW=n

rem Python documentation related variables
set AUTO_PYTHON_DEPS=n

rem Skip doxygen (e.g. false positive) and continue the script
set DOXYGEN_CONTINUE_ANYWAY=n

set %PYTHON_DOCUMENTATION_REQUIREMENTS=..\..\..\docs\requirements.txt

rem Command line arguments processing
:args_loop
if NOT "%~1"=="" (
    if "%~1"=="--vcpkg" (
        set VCPKG_INSTALL_DIR=%~2
    )

    if "%~1"=="--help" (
        goto :help
    )
    shift
    goto :args_loop
)

if "!VCPKG_INSTALL_DIR!"=="" ( 
    echo Please provide the location to install vcpkg or modify VCPKG_INSTALL_DIR in this script.
    rem The double dash are in quote here because otherwise the echo command thow an error.
    echo "--vcpkg <path>"
    exit /b
)

echo Checking for Git...
where /q git
if ErrorLevel 1 (
    echo.
    echo Please make sure that Git is already installed.
    echo See https://git-scm.com/download/win
    echo e.g. https://github.com/git-for-windows/git/releases/download/v2.36.0.windows.1/Git-2.36.0-64-bit.exe
    echo Do not forget to open a new command prompt after Git installation.
    exit /b
)

echo Checking for Python...
where /q python
if ErrorLevel 1 (
    echo.
    echo Please make sure that Python is already installed.
    echo See https://www.python.org/downloads/
    echo e.g. https://www.python.org/ftp/python/3.10.4/python-3.10.4-amd64.exe
    echo While installing Python, make sure to check "Add PYTHON to path"
    echo Do not forget to open a new command prompt after Python installation.
    exit /b
)

echo Checking for Pip...
where /q pip
if ErrorLevel 1 (
    echo.
    echo Please make sure that Pip is already installed.
    echo See https://www.python.org/downloads/
    echo e.g. https://www.python.org/ftp/python/3.10.4/python-3.10.4-amd64.exe
    echo While installing Python, make sure to check "Add PYTHON to path"
    echo Do not forget to open a new command prompt after Python installation.
    exit /b
)

IF NOT EXIST "%PYTHON_DOCUMENTATION_REQUIREMENTS%" ( 
    echo.
    echo Could not find %PYTHON_DOCUMENTATION_REQUIREMENTS%.
    exit /b
)

echo Checking for CMake...
where /q cmake
if ErrorLevel 1 (
    echo.
    echo Please make sure that CMake is already installed.
    echo See https://cmake.org/download/
    echo e.g. https://github.com/Kitware/CMake/releases/download/v3.23.1/cmake-3.23.1-windows-x86_64.msi
    echo While installing CMake, make sure to check "Add CMake to the system PATH"
    echo Do not forget to open a new command prompt after CMake installation.
    exit /b
)

echo Checking for Microsoft Visual Studio...
set MSVS=0
for /d %%a in ("%programfiles%\Microsoft Visual Studio*") do (
    for /f "tokens=3 delims=\" %%x in ("%%a") do set MSVS=1
)

if !MSVS!==0 (
    echo No Microsoft Visual Studio installation was found in !programfiles!.
    echo For non-standard installation path, please use the following option:
    rem The double dash are in quote here because otherwise the echo command thow an error. 
    echo "ocio_deps --vs <path>"
    exit /b
)

echo Checking for Vcpkg...
where /q vcpkg
set IS_VCPKG_PRESENT=False && if ErrorLevel 1 ( set IS_VCPKG_PRESENT=False ) && if EXIST !VCPKG_INSTALL_DIR!\vcpkg.exe ( set IS_VCPKG_PRESENT=True )

if %IS_VCPKG_PRESENT%==False (
    echo Vcpkg is needed to install the following OCIO dependencies: openimageio, freeglut and glew.
    set /p AUTO_VCPKG=Do you want to install Vcpkg in [!VCPKG_INSTALL_DIR!]? [y/n]:

    if !AUTO_VCPKG!==y (
        echo.
        echo Installing vcpkg...
        echo Cloning Vcpkg repository to !VCPKG_INSTALL_DIR!....
        git clone https://github.com/Microsoft/vcpkg.git !VCPKG_INSTALL_DIR! >nul 2>&1
        echo Running bootstrap-vcpkg script...
        call !VCPKG_INSTALL_DIR!\bootstrap-vcpkg.bat >nul 2>&1
    ) else (
        echo.
        echo For Vcpkg, please use the following options:
        echo 1. Install it through this script by answering [y]es to the question
        rem The double dash are in quote here because otherwise the echo command thow an error.
        echo 2. Install it manually and provide the installation location using "--vcpkg <path>" option
        exit /b
    )
)

rem Make sure vcpkg in avaiable in the command prompt environment
set Path=%Path%;!VCPKG_INSTALL_DIR!
echo All set! Everything is present to continue....

rem **************************************************************************************
rem ** OCIO dependencies installation                                                   **
rem **************************************************************************************

echo.
echo **************************************************************************************
echo ** Vcpkg is used to install the following package:                                  **
echo **    - OpenImageIO                                                                 **
echo **    - Freeglut                                                                    **
echo **    - Glew                                                                        **
echo **************************************************************************************

if %isX64%==1 (
    IF NOT EXIST "!VCPKG_INSTALL_DIR!\packages\openimageio_x64-windows" ( 
        set /p AUTO_OPENIMAGEIO=OpenImageIO is needed. Do you want to install it? [y/n]:
    ) else (
        echo OpenImageIO is already present. Skipping...
        set IS_OPENIMAGEIO_PRESENT=1
    )

    if !AUTO_OPENIMAGEIO!==y (
        echo.
        echo **************************************************************************************
        echo ** Installing OpenImageIO...                                                        **
        echo **************************************************************************************
        vcpkg install openimageio:x64-windows
        set VCPKG_INTEGRATE=1
        set IS_OPENIMAGEIO_PRESENT=1
    ) else (
        if !IS_OPENIMAGEIO_PRESENT!==0 (
            echo Without OpenImageIO, some part of OCIO will not be built. Please install it with this script or manually.
            exit /b
        )
    )

    IF NOT EXIST "!VCPKG_INSTALL_DIR!\packages\freeglut_x64-windows" ( 
        set /p AUTO_FREEGLUT=Freeglut is needed. Do you want to install it? [y/n]:
    ) else (
        echo Freeglut is already present. Skipping...
        set IS_FREEGLUT_PRESENT=1
    )

    if !AUTO_FREEGLUT!==y (
        echo.
        echo **************************************************************************************
        echo ** Installing Freeglut...                                                           **
        echo **************************************************************************************
        vcpkg install freeglut:x64-windows
        set VCPKG_INTEGRATE=1
        set IS_FREEGLUT_PRESENT=1
    ) else (
        if !IS_FREEGLUT_PRESENT!==0 (
            echo Without Freeglut, some part of OCIO will not be built. Please install it with this script or manually
            exit /b
        )
    )

    IF NOT EXIST "!VCPKG_INSTALL_DIR!\packages\glew_x64-windows" ( 
        set /p AUTO_GLEW=Glew is needed. Do you want to install it? [y/n]:
    ) else (
        echo Glew is already present. Skipping...
        set IS_GLEW_PRESENT=1
    )

    if !AUTO_GLEW!==y (
        echo.
        echo **************************************************************************************
        echo ** Installing Glew...                                                               **
        echo **************************************************************************************
        vcpkg install glew:x64-windows
        set VCPKG_INTEGRATE=1
        set IS_GLEW_PRESENT=1
    ) else (
        if !IS_GLEW_PRESENT!==0 (
            echo Without Glew, some part of OCIO will not be built. Please install it with this script or manually.
            exit /b
        )
        
    )

    rem Integrate vcpkg with Visual Studio
    if !VCPKG_INTEGRATE!==1 (
        echo Running Vcpkg integrate command...
        vcpkg integrate install >nul 2>&1
    )

    echo.
    echo **************************************************************************************
    echo ** Python pip will be used to install the documentations dependencies               **
    echo **    six                                                                           **
    echo **    testresources                                                                 **
    echo **    recommonmark                                                                  **
    echo **    sphinx-press-theme                                                           **
    echo **    sphinx-tabs                                                                   **
    echo **    breathe                                                                       **
    echo **************************************************************************************
    
    set /p AUTO_PYTHON_DEPS=Do you want to install Python documentations dependencies? [y/n]:
    if !AUTO_PYTHON_DEPS!==y (
        echo Checking for Doxygen...
        if NOT EXIST "%programfiles%\doxygen" (
            echo.
            echo Please make sure that Doxygen is already installed.
            echo See https://www.doxygen.nl/download.html
            echo e.g. https://www.doxygen.nl/files/doxygen-1.9.3-setup.exe
            echo Do not forget to open a new command prompt after Doxygen installation.

            set /p DOXYGEN_CONTINUE_ANYWAY=It could be a false positive. Do you want to continue anyway? [y/n]:
        ) else (
            set DOXYGEN_CONTINUE_ANYWAY=y
        )

        if !DOXYGEN_CONTINUE_ANYWAY!==y (
            echo Installing python documentation dependencies...
            rem NOTE: Change path based on script location
            pip install -r %PYTHON_DOCUMENTATION_REQUIREMENTS%
            where /q python
            if not ErrorLevel 1 (
                for /f %%p in ('python -c "import os, sys; print(os.path.dirname(sys.executable))"') do SET PYTHON_PATH=%%p
            )
        ) else (
            exit /b
        )
    )
)

rem Output summary
echo **************************************************************************************************************
echo **                                                                                                          **
echo ** Summary                                                                                                  **
echo **                                                                                                          **
echo **************************************************************************************************************
echo.**
echo.** Vcpkg location:           !VCPKG_INSTALL_DIR!
if !IS_OPENIMAGEIO_PRESENT!==1 echo.** OpenImageIO:              !VCPKG_INSTALL_DIR!\packages\openimageio_x64-windows
if !IS_FREEGLUT_PRESENT!==1 echo.** Freeglut:                 !VCPKG_INSTALL_DIR!\packages\freeglut_x64-windows
if !IS_GLEW_PRESENT!==1 echo.** Glew:                     !VCPKG_INSTALL_DIR!\packages\glew_x64-windows
echo.**                                                                                                          
if Defined PYTHON_PATH (
    echo.** Python packages location: %PYTHON_PATH%\lib\site-packages
    echo.**  
)                                                                                                                                                                              
echo **************************************************************************************************************

exit /b 0


:help
echo.
echo **************************************************************************************
echo ** Verify and install OCIO dependencies                                             **
echo **                                                                                  **
echo ** Before running this script, please see the README.md file                        **
echo ** and manually install the following:                                              **
echo **    - Git                                                                         **
echo **    - Python and Pip                                                              **
echo **    - CMake                                                                       **
echo **    - Microsoft Visual Studio                                                     **
echo **                                                                                  **
echo ** Modifications are needed if Microsoft Visual Studio is not used.                 **
echo **                                                                                  **
echo ** Depending on presence and choices, the following could be installed:             **
echo **    - Vcpkg                                                                       **
echo **    - OpenImageIO (using Vcpkg)                                                   **
echo **    - Freeglut (using Vcpkg)                                                      **
echo **    - Glew (using Vcpkg)                                                          **
echo **                                                                                  **
echo **    - Python documentations (optional):                                           **
echo **          six                                                                     **
echo **          testresources                                                           **
echo **          recommonmark                                                            **
echo **          sphinx-press-theme                                                     **
echo **          sphinx-tabs                                                             **
echo **          breathe                                                                 **
echo **************************************************************************************
echo.
echo Usage: ocio_deps --vcpkg <path to vcpkg> [OPTIONS]...
echo.
echo Mandatory option:
echo --vcpkg        path to an existing vcpkg installation
echo                or path where you want to install vcpkg
exit /b 0




endlocal