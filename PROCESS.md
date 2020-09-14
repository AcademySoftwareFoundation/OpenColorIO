<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

# OpenColorIO Project Processes

This document outlines important development and administrative processes and 
procedures adhered to by the OpenColorIO project. It should be reviewed in 
conjuntion with [CONTRIBUTING.md](CONTRIBUTING.md) prior to proposing or 
submitting changes.

* [Core Library Changes](#Core-Library-Changes)
* [Required Approvals](#Required-Approvals)

## Core Library Changes

This procedure outlines the OpenColorIO project's process for design, 
development, and review of changes to the core library. The intent is for 
discussion and decisions around such changes to be made in public and have 
transparent outcomes, seeking the guidance of both the TSC (Technical Steering 
Commitee) and the wider OCIO community and stakeholders.

**NOTE:** Changes to OpenColorIO process or governance are considered core 
library changes since they have cascading effects across the project.

1. All core changes begin with community discussion, which can involve informal 
   conversation in Slack or on 
   [ocio-dev](https://lists.aswf.io/g/ocio-dev), but are ultimately brought to a 
   TSC meeting where concensus can be reached around the proposal's relevance 
   and approach. TSC meetings are public and meeting notes are posted to the 
   [OCIO repo](https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/ASWF/meetings/tsc) 
   within days of a meeting for a permanent record of decisions.
2. Following TSC resolution, an issue is created in GitHub to present a design 
   proposal. A period of further community discussion is encouraged around the
   proposed changes, captured in issue comments. Ongoing concerns may continue 
   to be brought to the TSC prior to and during development.
3. Feature proposal issues are then be added to the 
   [Roadmap](https://github.com/AcademySoftwareFoundation/OpenColorIO/projects/3) 
   project in GitHub for tracking development progress of all accepted 
   proposals.
4. During development, work in progress may optionally be shared for feedback 
   and early review via a 
   [draft pull request](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests#draft-pull-requests).
5. Upon completion of feature development, a pull request is created for review 
   and 
   [linked to the associated issue]((https://docs.github.com/en/github/managing-your-work-on-github/linking-a-pull-request-to-an-issue) for review.
   Linking the issue and PR will result in the issue being closed automatically 
   upon the PR being merged, and facilitate tracking of the proposed feature in
   the 
   [Roadmap](https://github.com/AcademySoftwareFoundation/OpenColorIO/projects/3). 
   See [CONTRIBUTING.md](CONTRIBUTING.md) for an overview of commit signing 
   requirements and a guide on submitting a pull request.
6. The pull request is merged only after obtaining 2 approvals from committers 
   listed in [COMMITTERS.md](COMMITTERS.md) that are not affiliated with the 
   author or their company. If approval does not happen within two weeks and 
   there are no raised objections or unresolved discussions happening about the 
   feature, a committer that is affiliated with the author may approve it. See 
   [Required Approvals](#Required-Approvals) for a detailed breakdown of OCIO 
   pull request approval policy.

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
