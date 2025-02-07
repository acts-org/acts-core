<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Integration tests

These are tests of large or multiple components of the library.

## Propagation

These are test of different propagators and propagator configurations. There are
two types of tests: self-consistency tests of single propagators and comparison
tests between two separate propagators.

The actual tests are implemented in `PropagationTests.hpp` and are templated on
the propagator type. The different test suites implemented in the
`Propagation....cpp` files only handle the surrounding infrastructure, e.g.
propagator construction and data-driven testcases, and then call the test
functions.
