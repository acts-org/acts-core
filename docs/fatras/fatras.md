<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Fatras

Fatras is an ACTS implementation for fast track simulation which was primarily built to easily obtain hits from a tracking geometry.
Note that it is not meant for detailed physical simulations or studies.

Fatras is using the {class}`Acts::Propagator` as a backend for the track extrapolation. The simulation is just an **Actor** in the terminology of the ACTS propagation.

:::{tip}
A dedicated description of the propagation can be found [here](propagation_impl).
:::

Hits on sensitive surfaces are recorded and tagged with the particle passing through the surface.
Later, this hit information can be used by a digitization algorithm to mimic a detector response.

ACTS Fatras is fully capable of handling secondary particles even though the current set of interactions is not utilizing this to a big extent.
The simulation will propagate each particle until it reaches the end of the detector or until a specified path length is reached.
Afterwards, the first secondary particle that might have been generated in the process will be propagated.
This continues until all particles are transported.

## Supported interactions

Fatras implements a few interactions
 - Bethe-Bloch energy loss for charged particles (see `ActsFatras::BetheBloch`)
 - Bethe-Heitler energy loss for electrons (and positrons) (see `ActsFatras::BetheHeitler`)
 - Photon conversion for pair production (see `ActsFatras::PhotonConversion`)
 - Scattering for charged particles (see `ActsFatras::GenericScattering`)

These interactions are meant to be physical accurate within their boundaries but are far from sufficient for a full simulation.

## Use-cases

The primary use-case for ACTS Fatras is early algorithm validation.
It is fast enough to simulate thousands of small to medium events (about 100 tracks) on consumer hardware in a few seconds.
This provides a quick turnaround time for development.

Fatras does not replace a full detector simulation like Geant4 as only a few interactions are implemented and the geometry is highly simplified.
