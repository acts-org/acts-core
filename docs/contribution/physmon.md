<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# What is physmon?

The ACTS CI runs a suite of physics performance monitoring jobs dubbed
*physmon*. The purpose is to monitor and detect changes in the physics
performance, both intentional and accidental.

The associated job will run a number of workflow combinations. Currently, this
includes the truth tracking and OpenDataDetector *full chain* workflows. The
latter is further split into configurations with full seeding, truth smeared or
truth estimated seeds. These jobs produce performance output files.

These performance output files are then evaluated using the programs dubbed
[](analysis_apps). After this step, the job will then run comparisons of the
diagnostics histograms created by these programs. If the comparisons indicate
that the histogram contents changed meaningfully, the job will fail and report
this on a pull request.

## How do I investigate a physmon failure?

The physmon CI job attaches its results as an artifact to the CI run (also for successful runs)
From your pull request, you need to click on the *Checks* tab at the top:

:::{figure} figures/physmon/physmon_checks.png
:width: 150px
:align: center
:::

From there, click on the *Builds* workflow on the left:

:::{figure} figures/physmon/physmon_run.png
:width: 300px
:align: center
:::

On the workflow overview, scroll down to find the attached artifacts, and locate the *physmon* artifact. You can click to download it:

:::{figure} figures/physmon/physmon_artifact.png
:width: 100%
:align: center
:::

:::{note}
GitHub Actions artifacts are deleted after a fixed amount of time. If the CI
completed a while ago, it is possible this report is no longer available.
:::

After the download, you need to unzip the archive, whose contents will look similar to this:

```
acts_analysis_residuals_and_pulls.root
ckf_seeded.html
ckf_seeded_plots
ckf_truth_estimated.html
ckf_truth_estimated_plots
ckf_truth_smeared.html
ckf_truth_smeared_plots
ivf_seeded.html
ivf_seeded_plots
ivf_truth_estimated.html
ivf_truth_estimated_plots
ivf_truth_smeared.html
ivf_truth_smeared_plots
performance_ckf_seeded.root
performance_ckf_truth_estimated.root
performance_ckf_truth_smeared.root
performance_truth_tracking.root
performance_vertexing_seeded.root
performance_vertexing_seeded_hist.root
performance_vertexing_truth_estimated.root
performance_vertexing_truth_estimated_hist.root
performance_vertexing_truth_smeared.root
performance_vertexing_truth_smeared_hist.root
run.log
truth_tracking.html
truth_tracking_plots
```

The `.root` files are the performance output files and corresponding histogram
files. The physmon job log file is also available. Finally, the `_plots` folder
contain plots of all the histogram comparisons, and the `.html` files contain
single-file reports showing the detailed results. An example of an HTML report
looks like this:

:::{figure} figures/physmon/physmon_report.png
:width: 100%
:align: center
:::

If you get a physmon job failure on your pull request, please investigate the
failing report(s), and try to understand if the change causing the
discrepancies is expected.

In case the conclusion is that the changes are indeed expected, the reference
files used to generate the comparisons are located in `CI/physmon/reference/`,
and can be updated to the output files found in the artifact zip archive.
