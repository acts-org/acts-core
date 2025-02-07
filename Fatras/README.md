<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# FAst TRAck Simulation package

This packages provides tools to run a fast track simulation on top of the core
library. The fast track simulation (Fatras) uses the actor plug-in mechanism of
the `Acts::Propagator` and its predictive navigation to simulate particle
trajectories through the tracking detector.

As a fast track simulation, it uses the surface-based reconstruction geometry
description, i.e. `Acts::TrackingGeometry`, as a simulation geometry instead of
a detailed volumetric description. Interactions and material effects are
simulated using parametrized models:

-   Multiple Coulomb scattering is simulated by Gaussian (mixture)
    approximations.
-   Ionisation loss is simulated using the Bethe-Bloch formalism.
-   Radiation loss is simulated using the Bethe-Heitler formalism.
