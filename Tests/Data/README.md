<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Small test data files

**WARNING** All test files must be below 64kb in size.

This folder contains small test files, e.g. input data or comparison
references, for various parts of the test suite. These files should be
kept as small as possible. Access should always occur via the `getDataPath`
helper functions from the `CommonHelpers` package:

```cpp
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"

...
auto path = Acts::Test::getDataPath("some-data-file.csv");
```
