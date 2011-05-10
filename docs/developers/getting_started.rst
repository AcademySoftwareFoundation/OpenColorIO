Getting started
===============

Checking Out The Codebase
*************************

The master code repository is available on Github:  http://github.com/imageworks/OpenColorIO

For those unfamiliar with git, the wonderful part about it is that even though only a limited number people have write access to the master repository, anyone is free to create, and even check in, changes to their own local git repository.  Your local changes will not automatically be pushed back to the master repository, so everyone feel free to informally play around with the codebase.  Also - unlike svn - when you download the git repository you have a full copy of the project's history (including revision history, logs, etc), so the majority of code changes you will make, including commits, do not require network server access.

The first step is to install git on your system.  For those new to the world of git, github has an excellent tutorial stepping you through the process, available at: http://help.github.com/

To check out a read-only version of the repository (no github signup required)::

    git clone git://github.com/imageworks/OpenColorIO.git ocio

For write-access, you must first register for a Github account (free).  Then, you must create a local fork of the OpenColorIO repository by visiting http://github.com/imageworks/OpenColorIO and clicking the "Fork" icon. If you get hung up on this, further instructions on this process are available at http://help.github.com/forking/

To check out a read-write version of the repository (github acct required)::

    git clone git@github.com:yourgitusername/OpenColorIO.git ocio

    Initialized empty Git repository in /mcp/ocio/.git/
    remote: Counting objects: 2220, done.
    remote: Compressing objects: 100% (952/952), done.
    remote: Total 2220 (delta 1434), reused 1851 (delta 1168)
    Receiving objects: 100% (2220/2220), 2.89 MiB | 2.29 MiB/s, done.
    Resolving deltas: 100% (1434/1434), done.

Both read + read/write users should then add the Imageworks (SPI) master branch as a remote. This will allow you to more easily fetch updates as they become available::

    cd ocio
    git remote add spi git://github.com/imageworks/OpenColorIO.git

Optionally, you may then add any additional users who have individual working forks (just as you've done).  This will allow you to track, view, and potentially merge intermediate changes before they're been pushed into the main trunk. (For really bleeding edge folks).  For example, to add Jeremy Selan's working fork::

    git remote add js git://github.com/jeremyselan/OpenColorIO.git

You should then do a git fetch, and git merge (detailed below) to download the remote branches you've just added.


Merging changes
***************

More detailed guide coming soon, for now, see http://help.github.com/remotes/

.. TODO: Write this