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
* [Required Approvals](#Required-Approvals)
* [Coding Standards](#Coding-Standards)
* [Test Policy](#Test-Policy)
* [Versioning Policy](#Versioning-Policy)
* [Creating a Release](#Creating-a-Release)

For a description of the roles and responsibilities of the various
members of the OpenColorIO community, see [GOVERNANCE](GOVERNANCE.md), and
for further details, see the project's
[Technical Charter](docs/aswf/Charter.md). Briefly, Contributors are anyone
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

OCIO uses [EasyCLA](https://corporate.lfcla.com/) for managing CLAs, which
automatically checks to ensure CLAs are signed by a contributor before a commit
can be merged.

* If you are an individual writing the code on your own time and
  you're SURE you are the sole owner of any intellectual property you
  contribute, you can
  [sign the CLA as an individual contributor](https://docs.linuxfoundation.org/docs/communitybridge/communitybridge-easycla/contributors/sign-a-cla-as-an-individual-contributor-to-github).

* If you are writing the code as part of your job, or if there is any
  possibility that your employers might think they own any
  intellectual property you create, then you should use the 
  [Corporate Contributor Licence Agreement](https://docs.linuxfoundation.org/docs/communitybridge/communitybridge-easycla/contributors/contribute-to-a-github-company-project).

The OCIO CLA's are the standard forms used by Linux Foundation projects and
[recommended by the ASWF TAC](https://github.com/AcademySoftwareFoundation/tac/blob/master/process/contributing.md#contributor-license-agreement-cla).

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
has included his GitHub account in a corporate CLA whitelist, his pull request
can be merged. Otherwise the EasyCLA system will provide instructions on
signing a CLA, or request inclusion in an existing corporate CLA whitelist.

See the
[ASWF TAC CONTRIBUTING.md](https://github.com/AcademySoftwareFoundation/tac/blob/master/process/contributing.md#contribution-sign-off)
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

7. Upon receiving the required number of Committer approvals (as outlined
in [Required Approvals](#required-approvals)), a Committer other than the PR
contributor may squash and merge changes into the master branch.

See also (from the OCIO Developer Guide):
* [Getting started](http://opencolorio.org/developers/getting_started.html)
* [Submitting Changes](http://opencolorio.org/developers/submitting_changes.html)

## Required Approvals

Modifications of the contents of the OpenColorIO repository are made on a
collaborative basis. Anyone with a GitHub account may propose a modification via
pull request and it will be considered by the project Committers.

Pull requests must meet a minimum number of Committer approvals prior to being
merged. Rather than having a hard rule for all PRs, the requirement is based on
the complexity and risk of the proposed changes, factoring in the length of
time the PR has been open to discussion. The following guidelines outline the
project's established approval rules for merging:

* Core design decisions, large new features, or anything that might be perceived
as changing the overall direction of the project should be discussed at length
in the mail list before any PR is submitted, in order to: solicit feedback, try
to get as much consensus as possible, and alert all the stakeholders to be on
the lookout for the eventual PR when it appears.

* Small changes (bug fixes, docs, tests, cleanups) can be approved and merged by
a single Committer.

* Big changes that can alter behavior, add major features, or present a high
degree of risk should be signed off by TWO Committers, ideally one of whom
should be the "owner" for that section of the codebase (if a specific owner
has been designated). If the person submitting the PR is him/herself the "owner"
of that section of the codebase, then only one additional Committer approval is
sufficient. But in either case, a 48 hour minimum is helpful to give everybody a
chance to see it, unless it's a critical emergency fix (which would probably put
it in the previous "small fix" category, rather than a "big feature").

* Escape valve: big changes can nonetheless be merged by a single Committer if
the PR has been open for over two weeks without any unaddressed objections from
other Committers. At some point, we have to assume that the people who know and
care are monitoring the PRs and that an extended period without objections is
really assent.

Approval must be from Committers who are not authors of the change. If one or
more Committers oppose a proposed change, then the change cannot be accepted
unless:

* Discussions and/or additional changes result in no Committers objecting to the
change. Previously-objecting Committers do not necessarily have to sign-off on
the change, but they should not be opposed to it.

* The change is escalated to the TSC and the TSC votes to approve the change.
This should only happen if disagreements between Committers cannot be resolved
through discussion.

Committers may opt to elevate significant or controversial modifications to the
TSC by assigning the `tsc-review` label to a pull request or issue. The TSC
should serve as the final arbiter where required.

## Coding Standards

Please see the OpenColorIO
[Coding guidelines](http://opencolorio.org/developers/coding_guidelines.html)
for a reference on project code style and best practices.

For standards on contributing to documentation, see the
[Documentation guidelines](http://opencolorio.org/developers/documentation_guidelines.html).

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

The test should should be run, via ``ctest``, before submitting a pull request.

## Versioning Policy

OpenColorIO uses [semantic versioning](https://semver.org), which labels each
version with three numbers: Major.Minor.Patch, where:

* **MAJOR** indicates incompatible API changes
* **MINOR** indicates functionality added in a backwards-compatible manner
* **PATCH** indicates backwards-compatible bug fixes

## Creating a Release

To create a new release from the master branch:

1. Update the release notes in ``CHANGELOG.md`` with a high-level summary of
   the features and improvements. Also include the summary in the Release
   comments.

2. Create a new release on the GitHub Releases page.

3. Tag the release with name beginning with '``v``', e.g. '``v2.1.0``'.

4. Download and sign the release tarball, as described
   [here](https://wiki.debian.org/Creating%20signed%20GitHub%20releases),

5. Attach the detached ``.asc`` signature file to the GitHub release as a
   binary file.
