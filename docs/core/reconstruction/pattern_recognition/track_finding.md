<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

(track_finding)=
# Track Finding

:::{todo}
Write CKF documentation
:::

(ckf_core)=
## Combinatorial Kalman Filter

ACTS provides a Combinatorial Kalman Filter (CKF) implementation for track finding
({class}`Acts::CombinatorialKalmanFilter`). Its usage is demonstrated in the
example algorithm `ActsExamples::TrackFindingAlgorithm`.

The CKF requires as input a track seed i.e. an estimation of the track parameters,
and a list of measurements with an association to the (sensitive) surfaces of the
tracking geometry. The CKF propagates track states initialised by the track
parameter estimates in direction of the momentum (forward propagation). Whenever
a surface is reached, the CKF searches for compatible measurements. In case of multiple
compatible measurements the trajectory is branched. The track states of all
branches are updated and smoothed following the Kalman filter prescription. The
propagation is aborted if either the maximum path length is exceeded or if user
customizable abort conditions are reached.

The CKF is customizable via template parameters, and options. The template parameters
allow

1. to steer the propagation of the track states e.g. {class}`Acts::Propagator`, and
2. to define the storage container for the trajectories and its track states
   e.g. {class}`Acts::VectorMultiTrajectory`

The options {struct}`Acts::CombinatorialKalmanFilterOptions` are also customizable via
template parameters:

1. an iterator to iterate over links to (selected) measurements per tracking
   surface and which returns an {class}`Acts::SourceLink` when dereferenced.

2. the storage container for the trajectories and states which must be the same as
   for the CKF.

and provide:

1. user defined geometry (e.g. alignment), magnetic field and measurement
   calibration context which are unused by ACTS but passed to the possibly
   user defined delegates.

2. the reference surface with respect to which the track defining parameters are
   expressed (perigee)

3. delegates {class}`Acts::Delegate` mostly via the
   {struct}`Acts::CombinatorialKalmanFilterExtensions` to define
   - updater e.g. {class}`Acts::GainMatrixUpdater`
   - smoother e.g. {class}`Acts::GainMatrixSmoother`
   - measurement selector e.g. {class}`Acts::MeasurementSelector`
   - SourceLinkAccessor
   - measurement calibrator
   - branch stopper (optional)

Typically, users have to provide the tracking geometry, an implementation of a
{class}`Acts::MagneticFieldProvider`, which is needed by the stepper
e.g. {class}`Acts::EigenStepper` which is needed by the propagator, a
source link accessor and a measurement calibrator. An implementation of a source link
accessor and the measurement calibrator can be found among the examples
`ActsExamples::IndexSourceLinkAccessor`, and `ActsExamples::MeasurementCalibratorAdapter`


% Functional approach
% Implementation as an actor
% Limitations (no inward filtering)
% Requirement to copy out after smoothing if smoothed track states are desired

## Machine-Learning based Track Finding

There is a lot of research ongoing about machine-learning based approaches to Track Finding. Because these are not yet stable and bullet-prove, there are no such algorithms distributed with the core library. However, there exists a [plugin](exatrkxplugin), that implements the *Exa.TrkX* algorithm in ACTS.
