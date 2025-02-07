<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# The ACTS project

The *A Common Tracking Software (ACTS)* project is an attempt to preserve and evolve the track reconstruction software of the LHC era towards HL-LHC and beyond. It has been initiated in 2016 starting from the [ATLAS Common Tracking Software](https://gitlab.cern.ch/atlas/athena/-/tree/main/Tracking). Given the changing computing landscape, dedicated care of parallel code execution is taken, and is written in `C++20`.

A [coherent write-up of the project](https://link.springer.com/article/10.1007/s41781-021-00078-8) has been published in 2022 in Springer's CSBS.

```{note}
ACTS is designed as a library that *contains components* for assembling a track reconstruction suite for High Energy Physics and Nuclear Physics. ACTS does not strive to provide a complete experiment framework, but rather modules and connections to be used within an experiment context. These connections contain e.g. binding mechanisms to different geometry libraries, a cost-free yet flexible mechanism to use experiment specific contextual data (calibrations, detector alignment, experiment conditions), or simply the possibility to integrate an external screen logging facility.
```

The library is structured as follows:
 * The `Core` library contains considered production ready components (except for components located in the `Acts::Experimental` namespace) that can be interfaced to experiment code
 * The `Plugin` folder contains additional extensions that can be optionally switched on to use increase the functionality of the software suite, but also in general increase dependencies to other/thirdparty libraries
 * The `Fatras` library contains a fast track simulation module, that is based on the same concepts that are used for the [ATLAS Fatras](https://cds.cern.ch/record/1091969) fast track simulation
 * An `Examples` folder that contains a minimal test framework used for showcasing and integration testing,
 * A `Tests` folder that contains unit tests, benchmark tests and other integration tests


```{tip}
Developers and R&D meetings can be found in the [ACTS indico category](https://indico.cern.ch/category/7968/).
```

 ## Philosophy

 In order to minimize virtual function calls and complex inheritance schemes, ACTS does - in general - not define module interfaces, but rather relies on a data-centric pattern. Wherever possible, composition is favoured over inheritance/code extension.

 Code execution performance is a big focus in ACTS in order to serve the needs of the HL-LHC experiments; however, this is attempted to be reached mainly by technical means rather than compromising the physics performance.

 ## R&D Testbed

ACTS should also provide a testbed for fruitful algorithm R&D, and hence is closely coupled to the development of the [Open Data Detector](https://gitlab.cern.ch/acts/OpenDataDetector) which can be built as a `thirdparty` dependency as part of the ACTS project, and builds the backbone of the chain demonstrators.

In addition, two dedicated R&D lines are part of the `acts-project`, one for machine learning based/inspired modules [acts-machine-learning](mailto:acts-machine-learning@cern.ch), and one for massively parallel code execution [acts-parallelization](mailto:acts-parallelization@cern.ch), which mainly focuses on GPU accelerators and portability.

Code spill-over from the R&D lines into the main ACTS repository are performed on demand and depending on the maturity of the R&D projects.
