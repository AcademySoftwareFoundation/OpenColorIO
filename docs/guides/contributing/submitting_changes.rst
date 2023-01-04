..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _submitting-changes:

Submitting Changes
==================

Code Review
***********

Ask early, and ask often!

All new contributors are highly encouraged to post development ideas, questions,
or any other thoughts to :ref:`slack` or the :ref:`mailing_lists` before starting to
code. This greatly improves the process and quality of the resulting library. Code
reviews (particularly for non-trivial changes) are typically far simpler if the
reviewers are aware of a development task beforehand. (And, who knows? Maybe they
will have implementation suggestions as well!)

Development and Pull Requests
*****************************

This will outline the general mechanics of working with git and GitHub to
successfully contribute to the project. If any of the terms used are unfamiliar
to you please do a quick search and then ask any of the contributors for
assistance. 

.. tip::
    There's an excellent tutorial demonstrating how to contribute to 
    an OSS GitHub project on `Rob Allen's DevNotes 
    <https://akrabat.com/the-beginners-guide-to-contributing-to-a-github-project/>`__.
    It is highly recommended to give this post a read prior to cloning the 
    OpenColorIO repository if you intend to contribute.

* Fork the `AcademySoftwareFoundation/OpenColorIO 
  <https://github.com/AcademySoftwareFoundation/OpenColorIO>`__ repository

* Clone your fork to your local workspace::

    git clone https://github.com/$USER/OpenColorIO.git

* ``cd`` into the cloned directory

* Connect your cloned repo to the original upstream repository as a remote::

    git remote add upstream https://github.com/AcademySoftwareFoundation/OpenColorIO.git

* You should now have two remotes::

    git remote -v
    origin https://github.com/$USER/OpenColorIO (fetch)
    origin https://github.com/$USER/OpenColorIO (push)
    upstream https://github.com/AcademySoftwareFoundation/OpenColorIO (fetch)
    upstream https://github.com/AcademySoftwareFoundation/OpenColorIO (push)

* Pull the latest changes from upstream::

    git checkout main
    git pull upstream main

* Create a branch for your contribution::

    git checkout -b myFeature

* Check if it successfully compiles and passes all unit tests

* Commit your changes. The ``-s`` option will sign-off your commit, which is 
  a requirement for contributing to OpenColorIO. Every commit MUST be 
  signed-off by someone authorized to contribute under a current ICLA or CCLA::

    git add .
    git commit -s -m 'Implement my feature'

* If your PR changes anything that appears in the public API documentation,
  you should follow the directions below in "Updating the Python docs".
  If this proves problematic for you, please reach out for help on Slack.
  One of the other developers will probably be able to make the updates
  for you in another PR.

* Push your changes back to origin (your fork)::

    git push -u origin myFeature

* Ensure that all CI tests complete on GitHub actions

* Visit your fork in a web browser on github.com

* When ready click the "New pull request" button, make sure it can merge, and
  add appropriate comments and notes. GitHub will also provide a convenient 
  "Compare & pull request" button for recent pushes to streamline this step.

* Wait for code review and comments from the community.

* At various points in the review process other pull requests will have been 
  merged into OpenColorIO main, at which point merging of your PR will be 
  blocked with the message "This branch is out-of-date with the base branch". 
  Clicking the "Update branch" button will automatically merge the updated 
  branch into your feature branch. This step may need to be repeated multiple
  times depending on current commit activity.

See `CONTRIBUTING.md 
<https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/main/CONTRIBUTING.md#Repository-Structure>`__ 
for more information.
