<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# What is clang-tidy?

[`clang-tidy`](https://clang.llvm.org/extra/clang-tidy/) is a static analysis
tool using the LLVM / clang tooling. It can detect a number of issues with C++
code, like common readability problems, performance or memory safety issues.

The ACTS CI automatically runs `clang-tidy` on all compilation units to detect
as many problems as possible, and requires developers to resolve them. It is
configured with a file `.clang-tidy` located in the repository root. Many
editors / IDEs support `clang-tidy` and will automatically pick up this
configuration file.

By default, only a limited number of checks are configured in this file, but
this list might change in the future. If you experience CI failures that are
associated with `clang-tidy`, you can drill down into the CI job outputs to get
a report on the issues it detected. The report should give you an error /
warning code, e.g. `readability-braces-around-statements`. The LLVM
documentation has details on all possible error codes, in this particular
example you would find it [here][readability].  This page will tell you that
`clang-tidy` wants you to replace

```cpp
if (condition)
   statement;
```

with

```cpp
if (condition) {
   statement;
}
```

Some error codes are less obvious, or not this trivial to fix. When in doubt,
@mention the ACTS maintainers on your pull request.

[readability]: https://clang.llvm.org/extra/clang-tidy/checks/readability/braces-around-statements.html
