..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _getting-started:

Getting started
===============

Checking Out The Codebase
*************************

The main code repository is available on Github:  http://github.com/AcademySoftwareFoundation/OpenColorIO

For those unfamiliar with git, the wonderful part about it is that even though
only a limited number people have write access to the main repository, anyone
is free to create, and even check in, changes to their own local git repository. 
Your local changes will not automatically be pushed back to the main
repository, so everyone feel free to informally play around with the codebase.
Also - unlike svn - when you download the git repository you have a full copy of
the project's history (including revision history, logs, etc), so the majority
of code changes you will make, including commits, do not require network server
access.

The first step is to install git on your system.  For those new to the world of
git, GitHub has an excellent tutorial stepping you through the process,
available at: http://help.github.com/

To check out a read-only version of the repository (no GitHub signup required)::

    git clone git://github.com/AcademySoftwareFoundation/OpenColorIO.git ocio

For write-access, you must first register for a GitHub account (free).  Then,
you must create a local fork of the OpenColorIO repository by visiting
http://github.com/AcademySoftwareFoundation/OpenColorIO and clicking the "Fork" icon. If you
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

Both read + read/write users should then add the `AcademySoftwareFoundation/OpenColorIO <https://github.com/AcademySoftwareFoundation/OpenColorIO>`_ main branch
as a remote. This will allow you to more easily fetch updates as they become
available::

    cd ocio
    git remote add upstream git://github.com/AcademySoftwareFoundation/OpenColorIO.git

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
container that consists of both a Linux distro and the dependencies needed to run
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

See also :ref:`aswf-docker` for ASWF-managed docker images for building and 
testing OpenColorIO.

Merging changes
***************

After you fork and clone OCIO, the upstream repository will change continually. 
Your fork will not receive those changes until you manually pull and push the 
commits, and in the case of in-progress feature branches, you'll need to merge 
those changes or rebase your commits on top of them regularly to stay up to 
date. To update your fork from upstream run the::

    git checkout main
    git pull upstream main && git push origin main

The first command makes sure you have the main branch checked out, and the 
second combines an upstream pull (getting all the new commits to bring your
local clone up to date) and an origin push (updating your fork's remote 
main). Following these commands the main branch will be identical between
the OpenColorIO repository and your fork.

To merge these changes into a feature branch (making the branch able to merge 
with the upstream main), run::

    git checkout myFeature
    git merge main

git will report any merge conflicts encountered and allow you to resolve them 
locally prior to committing the merge.

Alternatively you can rebase the changes into your branch, which will replay
your branch commits on top of the latest main branch commits. A rebase should
only be used if you are the only contributor to a branch. Since rebasing alters
the branch's commit history, a force push is required to push the changes to
the remote repository, which can be problematic for others contributing to the
same branch. To rebase, run::

    git checkout myFeature
    git rebase main

Follow the interactive instructions that git provides during the rebase to 
resolve merge conflicts at each replayed commit. Update your remote branch
following a successful rebase with::

    git push origin myFeature --force

There are various reasons why you might prefer a merge or a rebase. This 
`article from Atlassian 
<https://www.atlassian.com/git/tutorials/merging-vs-rebasing>`__ provides a 
great basis for understanding both options along with their benefits and 
trade-offs.
