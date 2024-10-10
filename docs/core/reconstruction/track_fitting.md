# Track Fitting

The track fitting algorithms estimate the track parameters.
It is part of the pattern recognition/track  reconstruction/tracking.
We can run the track fitting algorithms, after we allocated all hits to single tracks with the help of a track finding algorithm.
It is not necessary, that all points of a track are present.

Currently, we have implementations for three different fitters:
* Kalman Filter (KF)
* Gaussian Sum Filter (GSF)
* Global Chi-Square Fitter (GX2F) [in development]
Even though all of them are least-squares fits, the concepts are quite different.
Therefore, we should not expect identical results from all of them.

(kf_core)=
## Kalman Filter (KF) [wip]
The Kalman Filter is an iterative fitter.
It successively combines measurements to obtain an estimate of the track parameters.
The KF needs an estimate as a starting point. The procedure alternates between two methods:
1. Extrapolate the current state to the next surface.
2. Update the extrapolation using the measurement of the new surface.[^billoir]
The meaning of "this surface" and "the next surface" changes with the context.
There are three different interpretations for this.
The KF can give us those three interpretations as sets of track parameters:
    * predicted: Uses "older" data (i.e. from the last surfaces) to make the prediction. This prediction is an extrapolation from the old data onto the current surface.
    * filtered: Uses the "current" data (i.e. the predicted data updated with the measurement on the current surface). It is some kind of weighted mean.
    * smoothed: Uses the "future" data to predict the current parameters. This can only be evaluated if the whole propagation is finished once. This can be done in to ways: one uses backwards-propagation and one does not.

:::{todo}
Complete Kalman Filter description
:::

(gsf_core)=
## Gaussian Sum Filter (GSF)

The GSF is an extension of the Kalman-Filter that allows to handle non-gaussian errors by modelling the track state as a gaussian mixture:

$$
p(\vec{x}) = \sum_i w_i \varphi(\vec{x}; \mu_i, \Sigma_i), \quad \sum_i w_i = 1
$$

A common use case of this is electron fitting. The energy-loss of Bremsstrahlung for electrons in matter are highly non-Gaussian, and thus cannot be modelled accurately by the default material interactions in the Kalman Filter. Instead, the Bremsstrahlung is modelled as a Bethe-Heitler distribution, where $z$ is the fraction of the energy remaining after the interaction ($E_f/E_i$), and $t$ is the material thickness in terms of the radiation length:

$$
f(z) = \frac{(- \ln z)^{c-1}}{\Gamma(c)}, \quad c = t/\ln 2
$$

(figBetheHeitler)=
:::{figure} figures/gsf_bethe_heitler_approx.svg
:width: 450px
:align: center
The true Bethe-Heitler distribution compared with a gaussian mixture approximation (in thin lines the individual components are drawn) at t = 0.1 (corresponds to ~ 10mm Silicon).
:::

To be able to handle this with the Kalman filter mechanics, this distribution is approximated by a gaussian mixture as well (see {numref}`figBetheHeitler`). The GSF Algorithm works then as follows (see also {numref}`figGsf`)

* On a surface with material, the Bethe-Heitler energy-loss distribution is approximated with a fixed number of gaussian components for each component. Since this way the number of components would grow exponentially with each material interaction, components that are close in terms of their *Kullback–Leibler divergence* are merged to limit the computational cost.
* On a measurement surface, for each component a Kalman update is performed. Afterwards, the component weights are corrected according to each component's compatibility with the measurement.

(figGsf)=
:::{figure} figures/gsf_overview.svg
:width: 450px
:align: center
Simplified overview of the GSF algorithm.
:::

### The Multi-Stepper
To implement the GSF, a special stepper is needed, that can handle a multi-component state internally: The {class}`Acts::MultiEigenStepperLoop`, which is based on the {class}`Acts::EigenStepper` and thus shares a lot of code with it. It interfaces to the navigation as one aggregate state to limit the navigation overhead, but internally processes a multi-component state. How this aggregation is performed can be configured via a template parameter, by default maximum weight is used ({type}`Acts::MaxWeightReducerLoop`).

Even though the multi-stepper interface exposes only one aggregate state and thus is compatible with most standard tools, there is a special aborter is required to stop the navigation when the surface is reached, the {struct}`Acts::MultiStepperSurfaceReached`. It checks if all components have reached the target surface already and updates their state accordingly. Optionally, it also can stop the propagation when the aggregate state reaches the surface.


### Using the GSF

The GSF is implemented in the class {struct}`Acts::GaussianSumFitter`. The interface of its `fit(...)`-functions is very similar to the one of the {class}`Acts::KalmanFitter` (one for the standard {class}`Acts::Navigator` and one for the {class}`Acts::DirectNavigator` that takes an additional `std::vector<const Acts::Surface *>` as an argument):

```{doxygenstruct} Acts::GaussianSumFitter
---
members: fit
outline:
---
```

The fit can be customized with several options. Important ones are:
* *maximum components*: How many components at maximum should be kept.
* *weight cut*: When to drop components.
* *component merging*: How a multi-component state is reduced to a single set of parameters and covariance. The method can be chosen with the enum {enum}`Acts::ComponentMergeMethod`. Two methods are supported currently:
    * The *mean* computes the mean and the covariance of the mean.
    * *max weight* takes the parameters of component with the maximum weight and computes the variance around these. This is a cheap approximation of the mode, which is not implemented currently.
* *mixture reduction*: How the number of components is reduced to the maximum allowed number. Can be configured via a {class}`Acts::Delegate`:
    * *Weight cut*: Keep only the N components with the largest weights. Implemented in {func}`Acts::reduceMixtureLargestWeights`.
    * *KL distance*: Merge the closest components until the required amount is reached. The distance measure is the *Kullback-Leibler distance* in the *q/p* component. Implemented in {func}`Acts::reduceMixtureWithKLDistance`.

:::{note}
A good starting configuration is to use 12 components, the *max weight* merging and the *KL distance* reduction.
:::

All options can be found in the {struct}`Acts::GsfOptions`:

```{doxygenstruct} Acts::GsfOptions
---
outline:
---
```

If the GSF finds the column with the string identifier *"gsf-final-multi-component-state"* (defined in `Acts::GsfConstants::kFinalMultiComponentStateColumn`) in the track container, it adds the final multi-component state to the track as a `std::optional<Acts::MultiComponentBoundTrackParameters<SinglyCharged>>` object.

A GSF example can be found in the ACTS Examples Framework [here](https://github.com/acts-project/acts/blob/main/Examples/Scripts/Python/truth_tracking_gsf.py).

### Customising the Bethe-Heitler approximation

The GSF needs an approximation of the Bethe-Heitler distribution as a Gaussian mixture on each material interaction (see above). This task is delegated to a separate class, that can be provided by a template parameter to {struct}`Acts::GaussianSumFitter`, so in principle it can be implemented in different ways.

However, ACTS ships with the class {class}`Acts::AtlasBetheHeitlerApprox` that implements the ATLAS strategy for this task: To be able to evaluate the approximation of the Bethe-Heitler distribution for different materials and thicknesses, the individual Gaussian components (weight, mean, variance of the ratio $E_f/E_i$) are parametrised as polynomials in $x/x_0$. This class can load files in the ATLAS format that can be found [here](https://gitlab.cern.ch/atlas/athena/-/tree/main/Tracking/TrkFitter/TrkGaussianSumFilter/Data). A default parameterization can be created with {func}`Acts::makeDefaultBetheHeitlerApprox`.

The {class}`Acts::AtlasBetheHeitlerApprox` is constructed with two parameterizations, allowing to use different parameterizations for different $x/x_0$. In particular, it has this behaviour:
* $x/x_0 < 0.0001$: Return no change
* $x/x_0 < 0.002$: Return a single gaussian approximation
* $x/x_0 < 0.1$: Return the approximation for low $x/x_0$.
* $x/x_0 \geq 0.1$: Return the approximation for high $x/x_0$. The maximum possible value is $x/x_0 = 0.2$, for higher values it is clipped to 0.2 and the GSF emits a warning.

### Further reading

* *Thomas Atkinson*, Electron reconstruction with the ATLAS inner detector, 2006, see [here](https://cds.cern.ch/record/1448253)
* *R Frühwirth*, Track fitting with non-Gaussian noise, 1997, see [here](https://doi.org/10.1016/S0010-4655(96)00155-5)
* *R Frühwirth*, A Gaussian-mixture approximation of the Bethe–Heitler model of electron energy loss by bremsstrahlung, 2003, see [here](https://doi.org/10.1016/S0010-4655(03)00292-3)

(gx2f_core)=
## Global Chi-Square Fitter (GX2F)

In general the *GX2F* is a weighted least squares fit, minimising the

$$
\chi^2 = \sum_i \frac{r_i^2}{\sigma_i^2}
$$

of a track.
Here, $r_i$ are our residuals that we weight with $\sigma_i^2$, the covariance of the measurement (a detector property).
Unlike the *KF* and the *GSF*, the *GX2F* looks at all measurements at the same time and iteratively minimises the starting parameters.

With the *GX2F* we can obtain the final parameters $\vec\alpha_n$ from starting parameters $\vec\alpha_0$.
We set the $\chi^2 = \chi^2(\vec\alpha)$ as a function of the track parameters, but the $\chi^2$-minimisation could be used for many other problems.
Even in the context of track fitting, we are quite free on how to use the *GX2F*.
Especially the residuals $r_i$ can have many interpretations.
Most of the time we will see them as the distance between a measurement and our prediction.
But we can also use scattering angles, energy loss, ... as residuals.
Therefore, the subscript $i$ stands most of the time for a measurement surface, since we want to go over all of them.

This chapter on the *GX2F* guides through:
- Mathematical description of the base algorithm
- Mathematical description of the multiple scattering
- (coming soon) Mathematical description of the energy loss
- Implementation in ACTS
- Pros/Cons

### Mathematical description of the base algorithm

:::{note}
The mathematical derivation is shortened at some places.
There will be a publication including the full derivation coming soon.
:::

To begin with, there will be a short overview on the algorithm.
Later in this section, each step is described in more detail.
1. Minimise the $\chi^2$ function
2. Update the initial parameters (iteratively)
3. Calculate the covariance for the final parameters

But before going into detail, we need to introduce a few symbols.
As already mentioned, we have our track parameters $\vec\alpha$ that we want to fit.
To fit them we, we need to calculate our residuals as

$$
r_i = m_i - f_i^m(\vec\alpha)
$$

where $f^m(\vec\alpha)$ is the projection of our propagation function $f(\vec\alpha)$ into the measurement dimension.
Basically, if we have a pixel measurement we project onto the surface, discarding all angular information.
This projection could be different for each measurement surface.

#### 1. Minimise the $\chi^2$ function

We expect the minimum of the $\chi^2$ function at

$$
\frac{\partial\chi^2(\vec\alpha)}{\partial\vec\alpha} = 0.
$$

To find the zero(s) of this function we could use any method, but we will stick to a modified [Newton-Raphson method](https://en.wikipedia.org/wiki/Newton%27s_method),
since it requires just another derivative of the $\chi^2$ function.

#### 2. Update the initial parameters (iteratively)

Since we are using the Newton-Raphson method to find the minimum of the $\chi^2$ function, we need to iterate.
Each iteration (should) give as improved parameters $\vec\alpha$.
While iterating we update a system, therefore we want to bring it in this form:

$$
\vec\alpha_{n+i} = \vec\alpha_n + \vec{\delta\alpha}_n.
$$

After some derivations of the $\chi^2$ function and the Newton-Raphson method, we find matrix equation to calculate $\vec{\delta\alpha}_n$:

$$
[a_{kl}] \vec{\delta\alpha}_n = \vec b
$$

with

$$
a_{kl} = \sum_{i=1}^N \frac{1}{\sigma_i^2} \frac{\partial f_i^m(\vec\alpha)}{\partial \alpha_k}\frac{\partial f_i^m(\vec\alpha)}{\partial \alpha_l}\\
$$

(where we omitted second order derivatives) and

$$
b_k = \sum_{i=1}^N \frac{r_i}{\sigma_i^2} \frac{\partial f_i^m(\vec\alpha)}{\partial \alpha_k}.
$$

At first sight, these expression might seem intimidating and hard to compute.
But having a closer look, we see, that those derivatives already exist in our framework.
All derivatives are elements of the Jacobian

$$
\mathbf{J} = \begin{pmatrix}
                 \cdot & \dots & \cdot\\
                 \vdots & \frac{\partial f^m(\vec\alpha)}{\partial \alpha_k} & \vdots\\
                 \cdot & \dots & \cdot
             \end{pmatrix}.
$$

At this point we got all information to perform a parameter update and repeat until the parameters $\vec\alpha$ converge.

#### 3. Calculate the covariance for the final parameters

The calculation of the covariance of the final parameters is quite simple compared to the steps before:

$$
cov_{\vec\alpha} = [a_{kl}]^{-1}
$$

Since it only depends on the $[a_{kl}]$ of the last iteration, the *GX2F* does not need an initial estimate for the covariance.

### Mathematical description of the multiple scattering

To describe multiple scattering, the *GX2F* can fit the scattering angles as they were normal parameters.
Of course, fitting more parameters increases the dimensions of all matrices.
This makes it computationally more expensive to.

But first shortly recap on multiple scattering.
To describe scattering, on a surface, only the two angles $\theta$ and $\phi$ are needed, where:
- $\theta$ is the angle between the extrapolation of the incoming trajectory and the scattered trajectory
- $\phi$ is the rotation around the extrapolation of the incoming trajectory

This description is only valid for thin materials.
To model thicker materials, one could in theory add multiple thin materials together.
It can be shown, that it is enough to two sets of $\theta$ and $\phi$ on both sides of the material.
We could name them $\theta_{in}$, $\theta_{out}$, $\phi_{in}$, and $\phi_{out}$.
But in the end they are just multiple parameters in our fit.
That's why we will only look at $\theta$ and $\phi$ (like for thin materials).

By defining residuals and covariances for the scattering angles, we can put them into our $\chi^2$ function.
For the residuals we choose (since the expected angle is 0)

$$
r_s = \begin{cases}
         0 - \theta_s(\vec\alpha) \\
         0 - \sin(\theta_{loc})\phi_s(\vec\alpha)
      \end{cases}
$$

with $\theta_{loc}$ the angle between incoming trajectory and normal of the surface.
(We cannot have angle information $\phi$ if we are perpendicular.)
For the covariances we use the Highland form [^cornelissen] as

$$
\sigma_{scat} = \frac{13.6 \text{MeV}}{\beta c p} Z\prime \sqrt{\frac{x}{X_0}} \left( 1 + 0.038 \ln{\frac{x}{X_0}} \right)
$$

with
- $x$ ... material layer with thickness
- $X_0$ ... radiation length
- $p$ ... particle momentum
- $Z\prime$ ... charge number
- $\beta c$ ... velocity

Combining those terms we can write our $\chi^2$ function including multiple scattering as

$$
\chi^2 = \sum_{i=1}^N \frac{r_i^2}{\sigma_i^2} + \sum_{s}^S \left(\frac{\theta_s^2}{\sigma_s^2} + \frac{\sin^2{(\theta_{loc})}\phi_s^2}{\sigma_s^2}\right)
$$

Note, that both scattering angles have the same covariance.

### (coming soon) Mathematical description of the energy loss [wip]

:::{todo}
Write *GX2F*: Mathematical description of the energy loss

The development work on the energy loss has not finished yet.
:::

### Implementation in ACTS

The implementation is in some points similar to the KF, since the KF interface was chosen as a starting point.
This makes it easier to replace both fitters with each other.
The structure of the *GX2F* implementation follows coarsely the mathematical outline given above.
It is best to start reading the implementation from `fit()`:

1. Set up the fitter:
    - Actor
    - Aborter
    - Propagator
    - Variables we need longer than one iteration
2. Iterate
    1. Update parameters
    2. Propagate through geometry
    3. Loop over track and calculate and sum over:
        - $\chi^2$
        - $[a_{kl}]$
        - $\vec b$
    4. Solve $[a_{kl}] \vec{\delta\alpha}_n = \vec b$
    5. Check for convergence
3. Calculate covariance of the final parameters
4. Prepare and return the final track

#### Configuration

Here is a simplified example of the configuration of the fitter.

```cpp
template <typename traj_t>
struct Gx2FitterOptions {
Gx2FitterOptions( ... ) : ... {}

Gx2FitterOptions() = delete;

...
//common options:
// geoContext, magFieldContext, calibrationContext, extensions,
// propagatorPlainOptions, referenceSurface, multipleScattering,
// energyLoss, freeToBoundCorrection

/// Max number of iterations during the fit (abort condition)
size_t nUpdateMax = 5;

/// Check for convergence (abort condition). Set to 0 to skip.
double relChi2changeCutOff = 1e-7;
};
```

Common options like the geometry context or toggling of the energy loss are similar to the other fitters.
For now there are three *GX2F* specific options:
1. `nUpdateMax` sets an abort condition for the parameter update as a maximum number of iterations allowed.
We do not really want to use this condition, but it stops the fit in case of poor convergence.
2. `relChi2changeCutOff` is the desired convergence criterion.
We compare at each step of the iteration the current to the previous $\chi^2$.
If the relative change is small enough, we finish the fit.

### Pros/Cons

There are some reasons for and against the *GX2F*.
The biggest issue of the *GX2F* is its performance.
Currently, the most expensive part is the propagation.
Since we need to do a full propagation each iteration, we end up with at least 4-5 full propagation.
This is a lot compared to the 2 propagations of the *KF*.
However, since the *GX2F* is a global fitter, it can easier resolve left-right-ambiguous measurements, like in the TRT (Transition Radiation Tracker – straw tubes).

[^billoir]: [https://twiki.cern.ch/twiki/pub/LHCb/ParametrizedKalman/paramKalmanV01.pdf](https://twiki.cern.ch/twiki/pub/LHCb/ParametrizedKalman/paramKalmanV01.pdf)
[^cornelissen]: [https://cds.cern.ch/record/1005181/files/thesis-2006-072.pdf#page=80](https://cds.cern.ch/record/1005181/files/thesis-2006-072.pdf)
