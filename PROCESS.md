<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

# OpenColorIO Project Processes

This document outlines important development and administrative processes and 
procedures adhered to by the OpenColorIO project. It should be reviewed in 
conjunction with [CONTRIBUTING.md](CONTRIBUTING.md) prior to proposing or 
submitting changes.

* [Core Library Changes](#Core-Library-Changes)
* [Required Approvals](#Required-Approvals)

## Core Library Changes

This procedure outlines the OpenColorIO project's process for design, 
development, and review of changes to the core library, large new features, or 
anything that might be perceived as changing the overall direction of the 
project. The intent is for discussion and decisions around such changes to be 
made in public and have transparent outcomes, seeking the guidance of both the 
TSC (Technical Steering Commitee) and the wider OCIO community and stakeholders.

**NOTE:** Changes to OpenColorIO process or governance have cascading effects 
across the project so are treated as core changes.

1. All core changes begin with community discussion, which can involve informal 
   conversation in [Slack](http://slack.opencolorio.org/) or on 
   [ocio-dev](https://lists.aswf.io/g/ocio-dev), but are ultimately brought to a 
   TSC meeting where consensus can be reached around the proposal's relevance 
   and approach. TSC meetings are public and meeting notes are posted to the 
   [OCIO repo](https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/ASWF/meetings/tsc) 
   within days of a meeting for a permanent record of decisions.
2. Following acceptance of the proposal by the TSC, an issue is created in 
   GitHub to present a design proposal. A period of further community 
   discussion is encouraged around the proposed changes, captured in issue 
   comments. Ongoing concerns may continue to be raised in this issue, and 
   brought to the TSC prior to and during development.
3. Feature proposal issues are added to the 
   [Feature Development](https://github.com/AcademySoftwareFoundation/OpenColorIO/projects/3) 
   project in GitHub by a committer for tracking the status of all accepted 
   proposals in one place.
4. During development, work in progress may optionally be shared for feedback 
   and early review via a 
   [draft pull request](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests#draft-pull-requests).
5. Upon completion of feature development, a pull request is created for review 
   and 
   [linked to the associated issue]((https://docs.github.com/en/github/managing-your-work-on-github/linking-a-pull-request-to-an-issue) for review.
   Linking the issue and PR will result in the issue being closed automatically 
   upon the PR being merged, and facilitate tracking of the proposed feature in
   [Feature Development](https://github.com/AcademySoftwareFoundation/OpenColorIO/projects/3). 
   See [CONTRIBUTING.md](CONTRIBUTING.md) for an overview of commit signing 
   requirements and a guide on submitting a pull request.
6. The pull request is merged after being approved as documented in the 
   [Required Approvals](#Required-Approvals) section below.

**NOTE**: Software design is often an iterative process with development. While 
a design proposal is required, it is understood that development of the 
proposed feature may begin prior to opening a design proposal issue in GitHub. It 
is however highly encouraged to propose the design early in the interest of 
respecting the contributor's time and effort. This avoids getting too far into 
development without community involvement, which may lead to redesign and 
extensive rework before changes can be approved.

## Required Approvals

Modifications of the contents of the OpenColorIO repository are made on a
collaborative basis. Anyone with a GitHub account may propose a modification via
pull request and it will be considered by the project Committers.

Pull requests must meet a minimum number of Committer approvals prior to being
merged. Rather than having a hard rule for all PRs, the requirement is based on
the complexity and risk of the proposed changes, factoring in the length of
time the PR has been open to discussion. The following guidelines outline the
project's established approval rules for merging:

* All changes must receive a minimum of two approvals from Committers listed in 
  [COMMITTERS.md](COMMITTERS.md) prior to being merged. GitHub branch 
  protections are enabled to enforce minimum committer count and Committer 
  status. A Committer approval is denoted in a PR with a green check mark. A 
  gray check mark beside an approval means the approver does not count toward 
  the minimum approval count, but such approvals are welcome and encouraged 
  from the community.

* Small changes (bug fixes, docs, tests, cleanups) may be approved by any two 
  Committers (other than the author).

* Large or significant changes must receive both approvals from Committers that 
  are not affiliated with the author or their company.

* All pull requests, except critical emergency fixes, must be posted for a 48 
  hour minimum prior to merging, even with the required approvals. This gives 
  everybody a chance to see it across time zones and schedules.

* If approval does not happen within two weeks of opening a pull request and 
  there are no raised objections or unresolved discussions happening about the 
  feature, a Committer that is affiliated with the author may approve it. At 
  some point, we have to assume that the people who know and care are 
  monitoring the PRs and that an extended period without objections is really 
  assent. Notice must be given in the pull request between 2 and 4 days in 
  advance of the intended merge date. This gives a windows for reviewers to 
  comment or request additional time for review.

If one or more Committers oppose a proposed change, then the change cannot be 
accepted unless:

* Discussions and/or additional changes result in no Committers objecting to 
  the change. Previously-objecting Committers do not necessarily have to 
  sign-off on the change, but they should not be opposed to it.

* The change is escalated to the TSC and the TSC votes to approve the change. 
  This should only happen if disagreements between Committers cannot be 
  resolved through discussion.

Committers may opt to elevate significant or controversial modifications to the
TSC by assigning the `tsc-review` label to a pull request or issue. The TSC
should serve as the final arbiter where required.
