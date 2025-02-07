<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Full chain examples for development and (manual) tests

**Warning** This is a set of internal tools used for the development of the core
library. This is **absolutely not** intended for production use. Only the final
build artifacts are installed; the code is not.

This module provides standalone simulation, reconstruction, and other usage
examples for various parts of the core library based on a simple event
processing framework.

-   `Framework` contains the event processing framework, shared event data
    model, and shared utilities.
-   `Detectors` contains example tracking geometries; some hardcoded, some
    based e.g. on DD4hep or TGeo input. Includes shared magnetic field
    definitions.
-   `Algorithms` contains algorithms for the event processing framework; they
    are mostly wrappers around functionality of the core library using the
    shared event data model.
-   `Io` contains algorithms for reading and writing event data.
-   `Run` contains the executable code; mostly using the framework tools
    previously defined.
-   `Scripts` contains additional helper and plotting scripts. These usually
    process the outputs from example executables.
