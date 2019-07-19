$pythonVersion = $Args[0]

Invoke-WebRequest "https://www.python.org/ftp/python/${pythonVersion}/python-${pythonVersion}.amd64.msi" -OutFile "C:\python-${pythonVersion}.amd64.msi"
msiexec /i "C:\python-${pythonVersion}.amd64.msi" /quiet /l* C:\_python.log TARGETDIR=C:\_python
Write-Host "##vso[task.prependpath]C:\_python"
Write-Host "##vso[task.prependpath]C:\_python\Scripts"
