<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# CI instructions

This directory comntains scripts and other files relating to the CI.

## Use poetry to manage requirements

Since [`poetry`](https://python-poetry.org) supports more robust dependency locking than `pip` this is used for dependency tracking. The authoritative files are `pyproject.toml` which defines the primary dependencies, and `poetry.lock` which defines the full locked dependencies. This can be exported to a `requirements.txt` file via

```console
poetry export -f requirements.txt > requirements.txt
```
