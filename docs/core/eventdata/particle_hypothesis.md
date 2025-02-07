<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Particle hypothesis

The *particle hypothesis* consists of the information about a particle necessary for accurate track reconstruction. This is the
 - *absolute PDG*
 - *mass*
 - *absolute charge*

This is redundant, as the *absolute PDG* can be used to look up *mass* and *absolute charge*, but used as a caching mechanism and can be an opportunity to use different, not necessarily physical values.

Internally, ACTS will use the *absolute PDG* to determine the energy loss or multiple scattering distribution for the particle. *mass* parameterizes these functions and is used for time transport. *absolute charge* is used as a conversion factor between the track parameter $\frac{q}{p}$ and $p$ whenever required.

The implementation consists of a generic, templated class `Acts::GenericParticleHypothesis` which depends on the charge type.
There are four different charge types
 - {struct}`Acts::Neutral`
 - {struct}`Acts::SinglyCharged`
 - {class}`Acts::NonNeutralCharge`
 - {class}`Acts::AnyCharge`
The reason for these different charge types is that in special cases it is not necessary to carry the *absolute charge* as it is already described by the type.

Ultimately there is a collection of classes which fill the charge type into the `Acts::GenericParticleHypothesis` for the user and provide a set of common particle hypotheses.
 - {class}`Acts::SinglyChargedParticleHypothesis`
 - {class}`Acts::NeutralParticleHypothesis`
 - {class}`Acts::NonNeutralChargedParticleHypothesis`
 - {class}`Acts::ParticleHypothesis`

Internally, ACTS will use {class}`Acts::ParticleHypothesis`, if not specified otherwise. This is the most generic one which is convertible from all the other hypotheses.
