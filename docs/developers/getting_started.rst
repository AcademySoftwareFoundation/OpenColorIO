.. _getting-started:

Getting started
===============

Checking Out The Codebase
*************************

The master code repository is available on Github:  http://github.com/imageworks/OpenColorIO

For those unfamiliar with git, the wonderful part about it is that even though
only a limited number people have write access to the master repository, anyone
is free to create, and even check in, changes to their own local git repository. 
Your local changes will not automatically be pushed back to the master
repository, so everyone feel free to informally play around with the codebase.
Also - unlike svn - when you download the git repository you have a full copy of
the project's history (including revision history, logs, etc), so the majority
of code changes you will make, including commits, do not require network server
access.

The first step is to install git on your system.  For those new to the world of
git, GitHub has an excellent tutorial stepping you through the process,
available at: http://help.github.com/

To check out a read-only version of the repository (no GitHub signup required)::

    git clone git://github.com/imageworks/OpenColorIO.git ocio

For write-access, you must first register for a GitHub account (free).  Then,
you must create a local fork of the OpenColorIO repository by visiting
http://github.com/imageworks/OpenColorIO and clicking the "Fork" icon. If you
get hung up on this, further instructions on this process are available at
http://help.github.com/forking/

To check out a read-write version of the repository (GitHub acct required)::

    git clone git@github.com:$USER/OpenColorIO.git ocio

    Initialized empty Git repository in /mcp/ocio/.git/
    remote: Counting objects: 2220, done.
    remote: Compressing objects: 100% (952/952), done.
    remote: Total 2220 (delta 1434), reused 1851 (delta 1168)
    Receiving objects: 100% (2220/2220), 2.89 MiB | 2.29 MiB/s, done.
    Resolving deltas: 100% (1434/1434), done.

Both read + read/write users should then add the Imageworks (SPI) master branch
as a remote. This will allow you to more easily fetch updates as they become
available::

    cd ocio
    git remote add upstream git://github.com/imageworks/OpenColorIO.git

Optionally, you may then add any additional users who have individual working
forks (just as you've done).  This will allow you to track, view, and
potentially merge intermediate changes before they're been pushed into the main
trunk. (For really bleeding edge folks).  For example, to add Jeremy Selan's
working fork::

    git remote add js git://github.com/jeremyselan/OpenColorIO.git

You should then do a git fetch, and git merge (detailed below) to download the
remote branches you've just added.

Reference Build Environment
***************************

To aid new developers to the project and provide a baseline standard,
OCIO provides a reference build environment through Docker. Docker essentially is a
container that consits of both a Linux distro and the dependencies needed to run
a client application. This is typically used for deploying apps and services to
servers, but we are using it to provide an isolated development environment to build
and test OCIO with. With this environment you are guaranteed to be able to compile OCIO
and run its non-GUI command line applications.

For more information on Docker, start here:
https://docs.docker.com/engine/docker-overview/

In order to run the Docker environment you will have to build it from the Dockerfile
provided in the repo directory::

    OpenColorIO/shared/docker

Run this command in order to build the Docker image (aprox. 20min)::

    docker build . -t ocio:centos7_gcc48 -f dockerfile_centos7_gcc48

You can then mount the current OCIO directory and compile using the Docker image with::

    docker run --volume $PWD/../../:/src/ociosrc -t ocio:centos7_gcc48 bash -c 'mkdir /build && cd /build && cmake /src/ociosrc && make -j2`


Merging changes
***************

More detailed guide coming soon, for now, see http://help.github.com/remotes/

.. TODO: Write this
