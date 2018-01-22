Docker files
===========


Usage
-----
$ docker build -f /path/to/a/Dockerfile .

Please refer to the link below to have more details.


Windows Specifics
-----------------
Depending of the directory, you could have a build failure when creating the image from the docker file.
The error message is related to the 'AppData\Local' directory which contains some white spaces.
The solution is to create a sub directory and run from it.

Please refer to the link below to have the full explanation.


References
----------

How to use docker files:  https://docs.docker.com/engine/reference/builder/#usage

Windows limitation: https://stackoverflow.com/questions/41286028/docker-build-error-checking-context-cant-stat-c-users-username-appdata
