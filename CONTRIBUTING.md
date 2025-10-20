# Contributing to OpenQMC

Thank you for your interest in contributing to OpenQMC. This document explains the contribution process and procedures:

* [How to contribute a bug fix or change](#how-to-contribute-a-bug-fix-or-change)
* [Development workflow](#developer-workflow)
* [Coding style](#coding-style)
* [Documentation style](#documentation-style)

For a description of the roles and responsibilities of the various members of the OpenQMC community, see the [governance policies](GOVERNANCE.md), and for further details, see the project's [Technical Charter](tsc/charter.md). Briefly, Contributors are anyone who submits content to the project, Committers review and approve such submissions, and the Technical Steering Committee provides general project oversight.

If you just need help or have a question, refer to [SUPPORT.md](SUPPORT.md).

## How to contribute a bug fix or change

<!-- MKDOCS_SPLIT: contributing.md -->
To contribute code to the project, first read over the [governance policies](GOVERNANCE.md) page to understand the roles involved.

Each contribution must meet the [coding style](#coding-style) and include...

* Tests and documentation to explain the functionality.
* Any new files have [copyright and license headers](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#license-specification).
* A [Developer Certificate of Origin signoff](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contribution-sign-off).
* Submitted to the project as a pull request.
* Clear description of changes in a [signed commit](https://docs.github.com/en/authentication/managing-commit-signature-verification).

OpenQMC is licensed under the [Apache 2.0](LICENSE) license. Contributions should abide by that standard license.

Project committers review contributions in a timely manner, and advise of any changes needed to merge the request.

## Developer workflow

For information on how to build and work with the code base, see the [Developer workflow](README.md#developer-workflow) section on the README page.

### Signing and DCO signoff

Commits must be [signed](https://docs.github.com/en/authentication/managing-commit-signature-verification), as well as have a [DCO signoff](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contribution-sign-off). The first is for verification, and the second is an agreement to the licensing terms of the project. You can configure git to always sign commits for verification. Once configured, the DCO signoff can also be applied automatically by passing the `git commit` command the option `--signoff`:

```bash
git commit --signoff
```

### Commit messages

Commit messages should consist of three parts, each separated with a line of white space. The first is a subject line of 50 characters or fewer, without a full stop at the end. Text should be in present-tense and imperative-style.

The second two parts are paragraphs. The first paragraph states the reason for the changes, and the second describes what those changes are. These can be multi line and must wrap at 72 characters. Sentences end in a full stop.

You can use [.gitmessage](.gitmessage) as a template for your commit messages and even configure Git to automatically copy the contents into the editor:

```bash
git config commit.template .gitmessage
```

Once you have configured the repository, the template text should appear in your editor upon calling `git commit`. Only the `*` characters need replacing. Git sees that the rest of the template content as a comment and won't include it in the final message.

### Documentation

Public functions, types, and their non-private members should have clear documentation. The format for this documentation follows the standard [Doxygen](https://www.doxygen.nl) style. See their site for details.

For other code, plain C style documentation is fine. For inline documentation, the author should only add extra comments when a reader would find it difficult to interpret the code itself, and the author can't make the code easer to read. It's a good sign if the code describes the how, and the documentation describes the why.

### Testing

Any new public functions or types should also come with appropriate unit and statistical tests. The project aims to maintain full test coverage for all public library code. For more information on building and running tests, see the [Testing](README.md#testing) section on the README page.

## Coding style

Coding style follows a predefined definition for both C++ and Python. And the project enforces compliance through the CI process. Prior to committing code, you can validate the changes pass formatting tests using this script:

```bash
./scripts/check-format.sh
```

If there are any formatting errors, these should ideally be resolved prior to committing and pushing the code, as the CI process will not allow it to be merged otherwise. Any issues can be automatically fixed using the following script:

```bash
./scripts/fix-format.sh
```

For automated formatting upon save in your editor, make sure your development environment is set up to use both ClangFormat and Black. This can be done using the [Language Server Protocol](https://microsoft.github.io/language-server-protocol).

## Linting

Code linting is evaluated against a set of rules for both C++ and Python. These rules are checked during the CI process, similarly to [formatting](#coding-style). Prior to committing code changes, you can validate those changes do not cause any linting errors using this script:

```bash
./scripts/check-lint.sh
```

If there are errors, these should ideally be addressed prior to committing the changes. If this is not done then the CI process will fail and prevent those changes from merging.

For automated diagnostic checks in your editor, make sure your development environment is set up to integrate both ClangTidy and Flake8. This can be done using [Language Server Protocol](https://microsoft.github.io/language-server-protocol).

## Coverage

Test coverage is evaluated using [llvm](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html) and is run on the CI pipeline. However, you can also run the check locally. First step is to add the appropriate build flags for enabling compiler instrumentation:

```bash
cmake --preset unix -D CMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping'
```

Then follow the steps in [Testing](README.md#testing) to build the unit test binary. Once the tests are built, the instrumentation will need to run all tests to gather the coverage data, before reporting the results. You can do this last step using the following script:

```bash
./scripts/check-coverage.sh
```

## Documentation style

More general documentation outside of code itself follows the [Google developer documentation style guide](https://developers.google.com/style). Contributors should see this as a guide and not a list of rules. Strict compliance to the guide isn't required and isn't validated during the CI process. It's instead up to the PR process to make sure documentation is up-to-date and of high quality.

Some simple guidelines to follow are:

- Use a friendly and conversational tone. This isn't academic writing.
- But be concise and keep to the point. Avoid saying please or being over polite.
- Use second person, not first person. Saying 'you' is good. But 'we' is bad.
- Use present tense, not future tense. Avoid 'will' and 'would'.
- Use active voice, not passive voice. Make it clear what's performing the action.
- Put conditional clauses before instructions. This lets the reader skip the instruction.
- Use abbreviations as they're harder to miss. Use 'wouldn't' over 'would not'.
- Be inclusive. Make use of gender-neutral pronouns.
<!-- MKDOCS_SPLIT_END -->
