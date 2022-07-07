# Building OCIO on Windows using MS Visual Studio 2022

# Manual steps for installing pre-requisites
- Install Microsoft Visual Studio
- Install Python
- Install CMake
- Install Doxygen (optional: only for python documentations)

## Microsoft Visual Studio (Desktop development with C++)
Please use the official Microsoft Visual Studio installer.
> See https://visualstudio.microsoft.com/downloads/

## Python
Please use the official Python installer
> See https://www.python.org/downloads/

**Make sure to check the option to add Python to PATH environment variable while running the installer**

## Cmake
Please use the official CMake installer.
> See https://cmake.org/download/
>
> Example: https://github.com/Kitware/CMake/releases/download/v3.23.1/cmake-3.23.1-windows-x86_64.msi

**Make sure to check the option to add CMake to PATH environment variable while running the installer**

### Doxygen (optional: for editing the documentation)
Please use the official Doxygen installer.
> See https://www.doxygen.nl/download.html
> 
> Example: https://www.doxygen.nl/files/doxygen-1.9.3-setup.exe


# Running the script to install the remaining pre-requisites

## Using ocio_deps.bat
**ocio_deps.bat** script can be used to verify and install the rest of OCIO dependencies.
<br/>
The script can be used to install the following:
- Vcpkg
- OpenImageIO
- FreeGLUT
- Glew
- Python documentation dependencies (six, testresources, recommonmark, sphinx-press-theme, sphinx-tabs, and breathe)

Run ocio script and follow the steps.
```bash
ocio_deps.bat --vcpkg <path to vcpkg root>

See ocio_deps --help:

Mandatory option:
--vcpkg        path to an existing vcpkg installation or path where you want to install vcpkg
```

# Running the script to build and install OCIO
## Using ocio.bat
**Ocio.bat** can be used to build and install OCIO.
```bash
# Options can be removed by modifying the value inside ocio.bat script.
# see the top of ocio.bat file

# Produce a release build using the default build and install paths.
ocio.bat --vcpkg <vcpkg directory> --type Release --ocio <path to OCIO source>
# Produce a debug build using the default build and install paths.
ocio.bat --vcpkg <vcpkg directory> --type Debug --ocio <path to OCIO source>
# Produce a release build using a custom build path (--b) and a custom install paths (--i).
ocio.bat --vcpkg <vcpkg directory> --type Release --b C:\ocio\__build --i C:\ocio\__install --ocio <path to OCIO source>

See ocio --help:

Mandatory options:
--vcpkg        path to an existing vcpkg installation

--ocio         path to OCIO source code

Optional options depending on the environment:
--python       Python installation location

--msvs         path where to find vcvars64.bat from Microsoft Visual Studio
               Default: C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build

--b            build location
               Default: C:\Users\cfuoco\AppData\Local\Temp\OCIO\build

--i            installation location
               Default: C:\Users\cfuoco\AppData\Local\Temp\OCIO\install

--type         Release or Debug
               Default: Release
--configure    Run cmake configure or not
               Default: false
```