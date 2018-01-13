.. _submitting-changes:

Submitting Changes
==================

Code Review
***********

Ask early, and ask often!

All new contributors are highly encouraged to post development ideas, questions,
or any other thoughts to the :ref:`mailing_lists` before starting to code.  This
greatly improves the process and quality of the resulting library.   Code
reviews (particularly for non-trivial changes) are typically far simpler if the
reviewers are aware of a development task beforehand. (And, who knows? Maybe they
will have implementation suggestions as well!)

GitHub Basics
*************

This will outline the general mechanics of working with git and GitHub to
successfully contribute to the project. If any of the terms used are unfamiliar
to you please do a quick search and then ask any of the contributors for
assistance.

* Fork the Imageworks OpenColorIO repository
* Activate Travis-CI and Appveyor for your fork
* Clone your fork to your local workspace::

    git clone https://github.com/$USER/OpenColorIO.git

* cd into the cloned directory
* Connect your cloned repo to the original upstream repository as a remote::

    git remote add upstream https://github.com/imageworks/OpenColorIO.git

* You should now have two remotes::

    git remote -v
    origin https://github.com/$USER/OpenColorIO (fetch)
    origin https://github.com/$USER/OpenColorIO (fetch)
    upstream https://github.com/imageworks/OpenColorIO (fetch)
    upstream https://github.com/imageworks/OpenColorIO (push)

* Pull the latest changes from upstream::

    git checkout master
    git pull upstream master

* Create a branch for your contribution::

    git checkout -b myFeature

* Check if it successfully compiles and passes all unit tests
* Commit your changes::

    git add .
    git commit -m 'Implement my feature'

* Push your changes back to origin (your fork)::

    git push origin myFeature

* Ensure that all CI tests complete on Travis-CI and Appveyor
* Visit your fork in a web browser on github.com
* When ready click the "New pull request" button, make sure it can merge, and
  add appropriate comments and notes
* Wait for code review and comments from the community
