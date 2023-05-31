<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Copyright Contributors to the OpenQMC Project -->

# Code Reviewing and Merging OpenQMC Pull Requests

This documents the code review and merging process adopted by the OpenQMC TSC, and can be used as a basis for other ASWF projects.

The Technical Steering Committee have write-access to the OpenQMC repository and are responsible for triaging, reviewing and ultimately merging or rejecting GitHub pull requests. This document lists the policy and best practices to guide the TSC in this process.

## Policy

* No direct commits to the main (or any other protected) branch, every code change should be a pull request
* Any member of the community can provide feedback on any pull request (and is encouraged to do so)
* A CODEOWNERS file introduced to the root of the repo to configure ownership (global, per-directory and/or per-file) - this will automatically request pull request reviews from the relevant maintainers (https://help.github.com/articles/about-codeowners/)
* Minimum of one non-author TSC member approval on every pull request before merging
* Non fast-forward merges must be used (ie the merge must not be rebased onto main)
* Azure Pipelines CI and DCO status checks must strictly pass before merging, other (TBD) checks should loosely pass (https://help.github.com/articles/types-of-required-status-checks)

## Best Practices

* Prefer all requested reviewers to approve before merging
* Merging a pull request should be the responsibility of the author if they are a TSC member
* Any TSC member can merge a pull request authored by a non-TSC member, but with a preferred minimum of two approvals from TSC members (including themself)
* Re-writing the branch history by rebasing a pull request branch just before a merge is discouraged, unless it significantly improves the overall history (such as any broken commits on the review branch that might make reverting or bisecting more difficult)
* Prefer pull requests to be open for a minimum of 72 hours before merging in order to gather any feedback
* Aim for all pull requests to be responded to by one of the TSC members within a minimum of two weeks with either explanation of non-acceptance, request for changes or merge
* TSC meetings should regularly review and discuss any outstanding pull requests
* Pull requests should link to the associated ticketing system ticket (if applicable) in the description or title, this provides a route back to the ticketing system ticket through the Git history (git blame -> PR merge -> ticketing system ticket)
* All pull request comments should aim to be answered and resolved before committing
