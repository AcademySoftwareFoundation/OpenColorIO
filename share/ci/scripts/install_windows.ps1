# Install Python
Invoke-WebRequest https://www.python.org/ftp/python/2.7.16/python-2.7.16.amd64.msi -OutFile C:\python-2.7.16.amd64.msi
msiexec /i C:\python-2.7.16.amd64.msi /quiet /l* C:\_python27.log TARGETDIR=C:\_python27
Write-Host "##vso[task.prependpath]C:\_python27"
Write-Host "##vso[task.prependpath]C:\_python27\Scripts"

# Install CMake
Invoke-WebRequest https://cmake.org/files/v3.14/cmake-3.14.4-win64-x64.zip -OutFile C:\cmake-3.14.4-win64-x64.zip
Expand-Archive C:\cmake-3.14.4-win64-x64.zip -DestinationPath C:\
Rename-Item -Path C:\cmake-3.14.4-win64-x64 -NewName C:\_cmake
Write-Host "##vso[task.prependpath]C:\_cmake\bin"