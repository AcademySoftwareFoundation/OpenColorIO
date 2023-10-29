<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

# Contributing to OpenColorIO

Thank you for your interest in contributing to OpenColorIO. This document
explains our contribution process and procedures, so please review it first:

* [Get Connected](#Get-Connected)
* [Legal Requirements](#Legal-Requirements)
* [Getting Started](#Getting-Started)
* [Repository Structure](#Repository-Structure)
* [Development and Pull Requests](#Development-and-Pull-Requests)
* [Coding Standards](#Coding-Standards)
* [Test Policy](#Test-Policy)
* [Versioning Policy](#Versioning-Policy)
* [Creating a Release](#Creating-a-Release)

For a description of the roles and responsibilities of the various
members of the OpenColorIO community, see [GOVERNANCE](GOVERNANCE.md), and
for further details, see the project's
[Technical Charter](ASWF/Charter.md). Briefly, Contributors are anyone
who submits content to the project, Committers review and approve such
submissions, and the Technical Steering Committee provides general project
oversight.

## Get Connected

The first thing to do, before anything else, is talk to us! Whether you're
reporting an issue, requesting or implementing a feature, or just asking a
question; please don’t hesitate to reach out to project maintainers or the
community as a whole. This is an important first step because your issue,
feature, or the question may have been solved or discussed already, and you’ll
save yourself a lot of time by asking first.

How do you talk to us? There are several ways to get in touch:

* [ocio-dev](https://lists.aswf.io/g/ocio-dev):
This is a development focused mail list with a deep history of technical
conversations and decisions that have shaped the project.

* [ocio-user](https://lists.aswf.io/g/ocio-user):
This is an end-user oriented mail list, focused on how to use OCIO’s features
within a host application. Common topics include crafting configs, DCC behavior,
and general color questions.

* [Slack](https://opencolorio.slack.com):
The OpenColorIO Slack group is where developers and expert users go to have
real-time communication around OCIO. The group is invitation only, but just
email us and we'll add anyone who is interested in participating in the
discussion.

* [GitHub Issues](https://github.com/AcademySoftwareFoundation/OpenColorIO/issues):
GitHub **issues** are a great place to start a conversation! Issues aren’t
restricted to bugs; we happily welcome feature requests and other suggestions
submitted as issues. The only conversations we would direct away from issues are
questions in the form of “How do I do X”. Please direct these to the ocio-dev or
ocio-user mail lists, and consider contributing what you've learned to our
docs if appropriate!

## Legal Requirements

OpenColorIO is a project hosted by the Academy Software Foundation (ASWF) and
follows the open source software best practice policies of the ASWF TAC with the
guidance from the Linux Foundation.

### License

OpenColorIO is licensed under the [BSD-3-Clause](LICENSE)
license. Contributions to the library should abide by that 
license unless otherwised approved by the OCIO TSC and ASWF Governing Board.

### Contributor License Agreements

Developers who wish to contribute code to be considered for inclusion
in OpenColorIO (OCIO) must first complete a **Contributor License Agreement
(CLA)**.

OCIO uses [EasyCLA](https://lfx.linuxfoundation.org/tools/easycla) for managing CLAs, which
automatically checks to ensure CLAs are signed by a contributor before a commit
can be merged.

* If you are an individual writing the code on your own time and
  you're SURE you are the sole owner of any intellectual property you
  contribute, you can
  [sign the CLA as an individual contributor](https://docs.linuxfoundation.org/lfx/easycla/contributors/individual-contributor).

* If you are writing the code as part of your job, or if there is any
  possibility that your employers might think they own any
  intellectual property you create, then you should use the 
  [Corporate Contributor Licence Agreement](https://docs.linuxfoundation.org/lfx/easycla/contributors/corporate-contributor).

The OCIO CLA's are the standard forms used by Linux Foundation projects and
[recommended by the ASWF TAC](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contributor-license-agreement-cla).

### Commit Sign-Off

Every commit must be signed off. That is, every commit log message must include
a “`Signed-off-by`” line (generated, for example, with
“`git commit --signoff`” or "`git commit -s`"), indicating that the committer
wrote the code and has the right to release it under the
[Modified-BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)
license.

Here is an example Signed-off-by line, which indicates that the submitter
accepts the DCO:

`Signed-off-by: John Doe <john.doe@example.com>`

If John Doe has signed an individual CLA, or his corporation's CLA Manager
has included his GitHub account in a corporate CLA approved list, his pull request
can be merged. Otherwise the EasyCLA system will provide instructions on
signing a CLA, or request inclusion in an existing corporate CLA approved list.

See the
[ASWF TAC CONTRIBUTING.md](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contribution-sign-off)
file for more information on this requirement.

### Copyright Notices

All new source files should begin with a copyright and license stating:

    // SPDX-License-Identifier: BSD-3-Clause
    // Copyright Contributors to the OpenColorIO Project.

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

All development work is done directly on the main branch. This represents the
bleeding-edge of the project and any contributions should be done on top of it.

After sufficient work is done on the main branch and OCIO leadership
determines that a release is due, we will bump the relevant internal versioning
and tag a commit with the corresponding version number, e.g. v2.0.1. Each Minor
version also has its own “Release Branch”, e.g. RB-1.1. This marks a branch of
code dedicated to that Major.Minor version, which allows upstream bug fixes to
be cherry-picked to a given version while still allowing the main branch to
continue forward onto higher versions. This basic repository structure keeps
maintenance low, while remaining simple to understand.

## Development and Pull Requests

Contributions should be submitted as Github pull requests. See
[Creating a pull request](https://help.github.com/articles/creating-a-pull-request/)
if you're unfamiliar with this concept.

Please review [PROCESS.md](PROCESS.md) for important procedures related to 
making changes to OpenColorIO. Small bug fixes and documentation changes can be
more informal, but modifications to core OpenColorIO functionality MUST follow
the 
[Core Library Changes](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/main/PROCESS.md#Core-Library-Changes) process.

The development cycle for a code change should follow this protocol:

1. Create a topic branch in your local repository, following the naming format
"feature/<your-feature>" or "bugfix/<your-fix>".

2. Make changes, compile, and test thoroughly. Code style should match existing
style and conventions, and changes should be focused on the topic the pull
request will be addressing. Make unrelated changes in a separate topic branch
with a separate pull request.

3. Push commits to your fork.

4. Create a Github pull request from your topic branch.

5. All pull requests trigger CI builds on 
[GitHub Actions](https://github.com/AcademySoftwareFoundation/OpenColorIO/actions).
These builds verify that code compiles and all unit tests succeed. CI build
status is displayed on the GitHub PR page, and changes will only be considered
for merging after all builds have succeeded.

6. Pull requests will be reviewed by project Committers and Contributors,
who may discuss, offer constructive feedback, request changes, or approve
the work.

7. Upon receiving the required number of Committer approvals (as outlined in 
[Required Approvals](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/main/PROCESS.md#Required-Approvals)), 
a Committer other than the PR contributor may squash and merge changes into the 
main branch.

For a more detailed description of the contribution process, please see the 
Contributing Guide in the main OCIO documentation:

* [Getting Started](https://opencolorio.readthedocs.io/en/latest/guides/contributing/contributing.html#getting-started)
* [Submitting Changes](https://opencolorio.readthedocs.io/en/latest/guides/contributing/contributing.html#submitting-changes)

## Coding Standards

Please see the OpenColorIO
[Coding guidelines](https://opencolorio.readthedocs.io/en/latest/guides/contributing/contributing.html#coding-style-guide)
for a reference on project code style and best practices.

For standards on contributing to documentation, see the
[Documentation guidelines](https://opencolorio.readthedocs.io/en/latest/guides/contributing/contributing.html#documentation-guidelines).

## Test Policy

All functionality in OpenColorIO must be covered by an automated test. Tests
should be implemented in a separate but clearly associated source file under
the `tests` subdirectory (e.g. tests for `src/OpenColorIO/Context.cpp` are
located in `tests/cpu/Context_tests.cpp`). This test suite is collectively
expected to validate the behavior of every part of OCIO:

* Any new functionality should be accompanied by a test that validates
  its behavior.

* Any change to existing functionality should have tests added if they
  don't already exist.

The test should be run, via ``ctest``, before submitting a pull request.

## Versioning Policy

OpenColorIO labels each version with three numbers: Major.Minor.Patch, where:

* **MAJOR** indicates major architectural changes
* **MINOR** indicates an introduction of significant new features
* **PATCH** indicates ABI-compatible bug fixes and minor enhancements
