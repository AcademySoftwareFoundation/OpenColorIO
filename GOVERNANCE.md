# OpenColorIO Project Governance

The OpenColorIO project is governed by its Committers, including a Technical
Steering Committee (TSC) which is responsible for high-level guidance of the
project.

## Contributors

The OpenColorIO project grows and thrives from assistance from Contributors.
Contributors include anyone in the community that contributes code,
documentation, or other technical artifacts that have been incorporated into the
projects repository.

## Committers

The OpenColorIO GitHub repository is maintained by OCIO Committers. Committers
are Contributors who have earned the ability to modify source code,
documentation, or other technical artifacts to the project. Upon becoming
Committers, they become members of the OCIO leadership team.

Their privileges include but are not limited to:

* Commit access to the OpenColorIO repository
* Moderator status on all communication channels

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

### Committer Activities

Typical activities of a Committer include:

* Helping users and novice contributors
* Contributing code and documentation changes that improve the project
* Reviewing and commenting on issues and pull requests
* Participation in working groups
* Merging pull requests

The TSC periodically reviews the Committer list to identify inactive Committers.
Past Committers are typically given Emeritus status. Emeriti may request that
the TSC restore them to active Committer status.

## Committer Nominations

Any existing Committer can nominate an individual making significant and
valuable contributions to the OpenColorIO project to become a new Committer.

To nominate a new Committer, open an issue in the OCIO repository, with a
summary of the nominee's contributions.

If there are no objections raised by any Committers one week after the issue is
opened, the nomination will be considered as accepted. Should there be any
objections against the nomination, the TSC is responsible for working with the
individuals involved and finding a resolution. The nomination must be approved
by the TSC, which is assumed when there are no objections from any TSC members.

Prior to the public nomination, the Committer initiating it may seek feedback
from other Committers in private channels and work with the nominee to improve
the nominee's contribution profile, in order to make the nomination as
frictionless as possible.

If individuals making valuable contributions do not believe they have been
considered for a nomination, they may log an issue or contact a Committer
directly.

## Technical Steering Committee

A subset of the Committers forms the Technical Steering Committee (TSC), which
has final authority over this project. As defined in the project charter, TSC
responsibilities include, but not limited to:

* Coordinating technical direction of the Project
* Project governance and process (including this policy)
* Contribution policy
* GitHub repository hosting
* Conduct guidelines
* Maintaining the list of additional Committers
* Appointing representatives to work with other open source or open standards
communities
* Discussions, seeking consensus, and where necessary, voting on technical
matters relating to the code base that affect multiple projects
* Coordinating any marketing, events, or communications regarding the project

Within the TSC are three elected leadership roles to be held by its members and
voted on annually. Any TSC member can express interest in serving in a role, or
nominate another member to serve. There are no term limits, and one person may
hold multiple roles simultaneously. Should a TSC member resign from a leadership
role before their term is complete, a successor shall be elected through the
standard nomination and voting process to complete the remainder of the term.
The leadership roles are:

* Chair: This position acts as the project manager, organizing meetings and
providing oversight to project administration.

* Chief Architect: This position makes all the final calls on design and
technical decisions, and is responsible for avoiding "design by committee"
pitfalls.

* ASWF TAC Representative: This position represents the project at all ASWF
(Academy Software Foundation) TAC (Technical Advisory Council) meetings.

### TSC Leaders

* Chair: TBD
* Chief Architect: TBD
* ASWF TAC Representative: TBD

### TSC Members

* Mark Boorer - ILM
* Sean Cooper - DNEG
* Michael Dolan - SPI
* Larry Gritz - SPI
* Patrick Hodoul - Autodesk
* Doug Walker - Autodesk

### TSC Meetings

Any meetings of the TSC are intended to be open to the public, except where
there is a reasonable need for privacy. The TSC meets regularly in a voice
conference call, at a cadence deemed appropriate by the TSC. The meeting is run
by a designated meeting chair approved by the TSC. Meetings may also be streamed
online where appropriate; connection details will be posted to the ocio-dev mail
list in advance of the scheduled meeting.

Items are added to the TSC agenda which are considered contentious or are
modifications of governance, contribution policy, TSC membership, or release
process, in addition to topics involving the high-level technical direction of
the project.

The intention of the agenda is not to approve or review all patches. That should
happen continuously on GitHub and be handled by the larger group of Committers.

Any community member or Contributor can ask that something be reviewed by the
TSC by logging a GitHub issue. Any Committer, TSC member, or the meeting chair
can bring the issue to the TSC's attention by applying the `tsc-review` label.

Prior to each TSC meeting, the meeting chair will share the agenda with members
of the TSC. TSC members can also add items to the agenda at the beginning of
each meeting. The meeting chair and the TSC cannot veto or remove items.

The TSC may invite additional persons to participate in a non-voting capacity.

The meeting chair is responsible for ensuring that minutes are taken and that
notes are submitted after the meeting to the public mailing list.

Due to the challenges of scheduling a global meeting with participants in
several time zones, the TSC will seek to resolve as many agenda items as
possible outside of meetings on the public mailing list.

### TSC Nomination

Any proposal for additional members of the TSC may be submitted by Committers
and/or TSC members by opening an issue outlining their case. Formal consensus
should be reached by the existing TSC members. No objections raised by any TSC
members two weeks after the issue is opened does not imply acceptance, if
general consensus cannot be reached by this time a formal vote is required.

Should there be any objections against the nomination, the TSC is responsible
for working with the individuals involved and finding a resolution.

### TSC Succession

A voting member of the TSC may nominate a successor in the event that such
voting member decides to leave the TSC, and the TSC, including the departing
member, shall confirm or reject such nomination by a vote.

In the event that the departing member’s nomination for successor is rejected by
vote of the TSC, the departing member shall be entitled to continue nominating
successors until one such successor is confirmed by vote of the TSC. If the
departing member fails or is unable to nominate a successor, the TSC may
nominate one on the departing member’s behalf.
