# Release process

Each release follows semantic versioning. You can read more about this in the
[Versioning](README.md#versioning) section of the README.

## Cadence

New releases are created based on the amount of meaningful change ready to be
shipped, and not based on a regular time period.

Major new releases will be agreed upon by the TSC.

## Criteria

Making a new release requires at least one change that meaningfully impacts
end users of the library and/or a patched bugfix. Major versions should require
multiple API breaking changes to be bundled together to minimise maintenance for
downstream projects.

Internal changes for the project itself should not constitute a new release.

All tests must pass before a release is made. This is already enforced by CI on
the `main` branch, but it should be confirmed before tagging.

## Methodology

When creating a new release, the following procedure should be followed:

1. Evaluate the version number for the release based on semantic versioning and
   the included changes.
2. Push up a PR adding the version number for Unreleased changes, starting a
   new Unreleased section.
3. Once the PR is merged to main, tag the commit using the new version number
   (see [Tagging](#tagging)).
4. Create a release on GitHub with notes following the format described in
   [Release notes](#release-notes).
5. Send an email to the mailing list: https://lists.aswf.io/g/openqmc-discussion
6. Update the Slack channel: https://academysoftwarefdn.slack.com/archives/C09BXDAMFTP

## Tagging

Releases are tagged on the merge commit on `main` using the new version number
with a `v` prefix, for example `v0.7.2`. Create a tag and push it:

```sh
git tag -a v0.7.2
git push origin v0.7.2
```

## Release notes

The GitHub release notes start with a brief overview sentence summarising the
release, followed by the relevant sections copied from the version's entry in
[CHANGELOG.md](CHANGELOG.md). Only include sections that have entries. For
example:

```md
Minor patch release primarily to resolve issue with unwanted static noise
patterns across frames.

### Changed

- Improve quality of uniform float distribution in `oqmc::uintToFloat`.
```
