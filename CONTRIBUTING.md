# Contributing to OpenColorIO

Thank you for your interest in contributing to OpenColorIO. This document
explains our contribution process and procedures, so please review it first.

## Get Connected

The first thing to do, before anything else, is talk to us! Whether you're
reporting an issue, requesting or implementing a feature, or just asking a
question; please don’t hesitate to reach out to project maintainers or the
community as a whole. This is an important first step because your issue,
feature, or the question may have been solved or discussed already, and you’ll
save yourself a lot of time by asking first.

How do you talk to us? There are several ways to get in touch:

* [ocio-dev](https://groups.google.com/forum/#!forum/ocio-dev):
This is a development focused mail list with a deep history of technical
conversations and decisions that have shaped the project.

* [ocio-users](https://groups.google.com/forum/#!forum/ocio-users):
This is an end-user oriented mail list, focused on how to use OCIO’s features
within a host application. Common topics include crafting configs, DCC behavior,
and general color questions.

* [Slack](https://opencolorio.slack.com):
The OpenColorIO Slack group is where developers and expert users go to have
real-time communication around OCIO. This group is invitation only, but we’re
quite lackadaisical in our barrier for entry. Simply put, if you just have
a quick question it's not worth joining, but if you want to start actively
participating and communicating more “casually” with project developers, this is
the place for you. Please request an invitation on the ocio-dev email list
mentioned above.

* [GitHub Issues](https://github.com/imageworks/OpenColorIO/issues):
GitHub **issues** are a great place to start a conversation! Issues aren’t
restricted to bugs; we happily welcome feature requests and other suggestions
submitted as issues. The only conversations we would direct away from issues are
questions in the form of “How do I do X”. Please direct these to the ocio-dev or
ocio-users mail lists, and consider contributing what you've learned to our
docs if appropriate!

## Getting Started

So you’ve broken the ice and chatted with us, and it turns out you’ve found a
gnarly bug that you have a beautiful solution for. Wonderful!

From here on out we’ll be using a significant amount of Git and GitHub based
terminology. If you’re unfamiliar with these tools or their lingo, please look
at the [GitHub Glossary](https://help.github.com/articles/github-glossary/) or
browse [GitHub Help](https://help.github.com/). It can be a bit confusing at
first, but feel free to reach out if you need assistance.

The first requirement for contributing is to have a GitHub account. This is
needed in order to push changes to the upstream repository. After setting up
your account you should then **fork** the OpenColorIO repository to your
account. This creates a copy of the repository under your user namespace and
serves as the “home base” for your development branches, from which you will
submit **pull requests** to the upstream repository to be merged.

You will also need Git installed on your local development machine. If you need
setup assistance, please see the official
[Git Documentation](https://git-scm.com/doc).

Once your Git environment is operational, the next step is to locally
**clone** your forked OpenColorIO repository, and add a **remote** pointing to
the upstream OpenColorIO repository. These topics are covered in
[Cloning a repository](https://help.github.com/articles/cloning-a-repository/)
and
[Configuring a remote for a fork](https://help.github.com/articles/configuring-a-remote-for-a-fork/),
but again, if you need assistance feel free to reach out on the ocio-dev mail
list.

You are now ready to contribute.

## Repository Structure

The OpenColorIO repository has a relatively straight-forward structure, and a
simple branching and merging strategy.

All development work is done directly on the master branch. This represents the
bleeding-edge of the project and any contributions should be done on top of it.

After sufficient work is done on the master branch and OCIO leadership
determines that a release is due, we will bump the relevant internal versioning
and tag a commit with the corresponding version number, e.g. v2.0.1. Each Minor
version also has its own “Release Branch”, e.g. RB-1.1. This marks a branch of
code dedicated to that Major.Minor version, which allows upstream bug fixes to
be cherry-picked to a given version while still allowing the master branch to
continue forward onto higher versions. This basic repository structure keeps
maintenance low, while remaining simple to understand.

## Development and Pull Requests

Contributions should be submitted as Github pull requests. See
[Creating a pull request](https://help.github.com/articles/creating-a-pull-request/)
if you're unfamiliar with this concept.

The development cycle for a code change should follow this protocol:

1. Create a topic branch in your local repository, following the naming format
"feature/<your-feature>" or "bugfix/<your-fix>".

2. Make changes, compile, and test thoroughly. Code style should match existing
style and conventions, and changes should be focused on the topic the pull
request will be addressing. Make unrelated changes in a separate topic branch
with a separate pull request.

3. Push commits to your fork.

4. Create a Github pull request from your topic branch.

5. All pull requests trigger CI builds on [Travis CI](https://travis-ci.org/)
for Linux and Mac OS and [AppVeyor](https://www.appveyor.com/) for Windows.
These builds verify that code compiles and all unit tests succeed. CI build
status is displayed on the GitHub PR page, and changes will only be considered
for merging after all builds have succeeded.

6. Pull requests will be reviewed by project Committers and Contributors,
who may discuss, offer constructive feedback, request changes, or approve
the work.

7. Upon receiving the required number of Committer approvals (as outlined in
[GOVERNANCE.md](https://github.com/imageworks/OpenColorIO/blob/master/GOVERNANCE.md#committers)),
a Committer other than the PR contributor may squash and merge changes into the
master branch.
