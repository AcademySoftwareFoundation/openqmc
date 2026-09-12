# Security Policy

This file describes the project security policy.

## Reporting a Vulnerability

If you think you've found a potential vulnerability in OpenQMC, please report it by emailing openqmc-tsc-private@lists.aswf.io. Only Technical Steering Committee members and Academy Software Foundation project management have access to these messages. Include detailed steps to reproduce the issue, and any other information that could aid an investigation. Our policy is to respond to vulnerability reports within 14 days, address critical security vulnerabilities rapidly and post patches as quickly as possible.

## Known Vulnerabilities

There are no known vulnerabilities at this time.

See the [release notes](CHANGELOG.md) for more information.

## Signed Releases

Release artifacts are signed via
[sigstore](https://www.sigstore.dev). See
[release-sign.yml](.github/workflows/release-sign.yml) for details.

To verify a download, replace `<tag>` with the release tag, such as `v0.7.1`, and
`<version>` with the tag without the `v`, such as `0.7.1`:

```bash
pip install sigstore
sigstore verify github --cert-identity https://github.com/AcademySoftwareFoundation/openqmc/.github/workflows/release-sign.yml@refs/tags/<tag> openqmc-<version>.tar.gz
```

If your system doesn't allow `pip install`, run the command with `pipx run sigstore` or `uvx sigstore` instead.
