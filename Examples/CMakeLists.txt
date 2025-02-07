# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_subdirectory(Algorithms)
add_subdirectory(Detectors)
add_subdirectory(Framework)
add_subdirectory(Io)
add_subdirectory_if(Python ACTS_BUILD_EXAMPLES_PYTHON_BINDINGS)
add_subdirectory_if(Scripts ACTS_BUILD_ANALYSIS_APPS)

add_subdirectory(HelloWorld)
