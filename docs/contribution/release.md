<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# How to make a release

We use [conventional commits](https://www.conventionalcommits.org/en/v1.0.0/#summary) for all of our PRs. We use the common prefixes: `fix:`, `feat:`, `build:`, `chore:`, `ci:`, `docs:`, `style:`, `refactor:`, `perf:`, `test:`.

The next version number for a release is automatically determined from the PRs (commits) that have been merged since the last release, following semantic versioning. On top of that, release notes are automatically generated from the PR titles, using the prefixes as groups.

## Step 1: Update the release branch

:::{note}
This assumes you have a local clone of the ACTS repository at hand.
You'll also need write permissions on the upstream ACTS repository to be able to push to the `releases` branch.
:::


```console
$ git checkout releases
$ git fetch upstream # make sure you're up to date
$ git merge --no-ff upstream/main
```

At this point, your commit graph should look something like this:

```console
de4bffa 2022-06-23 11:02 +0200 Paul Gessinger             M─┐ [releases] Merge remote-tracking branch 'origin/main' into releases
75f0835 2022-06-22 19:27 +0200 Andreas Stefl              │ o fix: SimulationActor produces negative MaterialSlab thickness (#1288)
c586094 2022-06-22 18:41 +0200 Andreas Stefl              │ o refactor: python examples vertex_fitting.py (#1286)
e6e7949 2022-06-22 10:31 +0200 Paul Gessinger             │ o ci: Bump ccache max size to 500MB (#1285)
bf8cb1c 2022-06-21 11:26 -0400 Joe Osborn                 │ o {fork/main} refactor: Move propagator options to general fitter options (#1282)
7bf3981 2022-06-20 20:16 +0100 Scott Hurley               │ o feat: Add Profiling Support with gperftools (#1274)
4a21df3 2022-06-14 18:32 +0200 Paul Gessinger             │ o fix: Add another compat fix for clang+libstdc++ (#1269)
93c608d 2022-06-14 16:35 +0200 Paul Gessinger             │ o ci: Retry nodeps build if it fails the first time (#1283)
9920a81 2022-06-14 14:53 +0200 Paul Gessinger             │ o ci: Reduce ccache limit to 150MB (#1270)
baa4222 2022-06-14 10:16 +0200 Benjamin Huth              │ o fix: Add missing dependency in Exa.TrkX plugin (#1276)
adf079e 2022-06-08 12:47 +0000 github-actions[bot]        o │ <v19.2.0> Bump to version v19.2.0
4ee53fd 2022-06-08 14:46 +0200 Paul Gessinger             M─│─┐ Merge branch main into releases
f9a0979 2022-06-08 09:35 +0200 Noemi Calace               │ o─┘ feat: Allow passing seed quality to seeds (#1268)
8a2260b 2022-06-07 16:31 +0200 Alexander J. Pfleger       │ o feat: RKN-monitoring to AtlasStepper, change type int->size_t (#1264)
bd55ecb 2022-06-07 15:25 +0200 Noemi Calace               │ o feat: configurable phi values in grid construction (#1275)
29f3649 2022-06-07 14:22 +0200 Noemi Calace               │ o fix: Fixing typo in name of variable and associated methods (#1277)
239ece3 2022-06-03 17:39 +0200 robertlangenberg           │ o  bump macos dependencies (#1279)
f77f377 2022-05-30 15:46 +0200 Noemi Calace               │ o feat: Adding features for sorting space points in r in (phi, z) grid (#1267)
a7ee09d 2022-05-25 11:17 +0200 Luis Falda Coelho          │ o fix: Bug in xyz strip coordinates transformation  (#1265)
82f42a2 2022-05-25 09:02 +0000 github-actions[bot]        o │ <v19.1.0> Bump to version v19.1.0
7c59e12 2022-05-25 10:26 +0200 Paul Gessinger             M─│─┐ Merge remote-tracking branch 'origin/main' into releases
4ceddf3 2022-05-25 10:26 +0200 Luis Falda Coelho          │ o─┘ feat: ITk seedFilter integration and seed quality confirmation  (#1201)
```

You can now push the updated `releases` branch to the remote `releases` branch using `git push -u upstream releases`.

On push, a CI job should run and create an additional commit on the `releases` branch, which bumps a number of version numbers. That commit is going to be the one tagged with the correct version. It doesn't hurt to make sure that commit looks right, as in it bumps to a sensible next version number.


## Step 2: Prepare milestone

By convention, we assign all open PRs and issues to the `next` milestone. When a new release is cut, all closed issues and PRs are moved over to a dedicated milestone named after the next version number. To do this, go to the *Pull Requests* view on the main repository, and click *Milestones*:

:::{image} figures/release/milestones.png
:width: 300px
:alt: The milestones button
:::

First, create a new milestone here. Name it `vX.Y.Z`, corresponding to the next version (you can check the release CI job mentioned above to determine this. The job will also create an unreleased GitHub *release* which should have the right name).

Next, go back to the list of milestones and go to the `next` milestone, and click on *closed* to get to a list of all closed PRs and issues assigned to it:

:::{image} figures/release/next_milestone.png
:alt: The next milestone, which has a "closed" button
:::

You'll be taken to a list view which shows all closed PRs and issues assigned to the `next` milestone. On the top left there should be a checkbox allowing you to select all visible items:

:::{image} figures/release/select_all_closed.png
:alt: Checkbox to select all visible PRs and issues.
:width: 200px
:::

```{note} The checkbox will only select the items on the visible page. If there are many items, it's possible you'll have to redo this step until it has been applied to all items.
```

Towards the right there's a button called *Milestone* which allows you to select a milestone to assign. Assign all closed items to the new milestone you created above.

:::{image} figures/release/milestone_drop.png
:alt: Dropdown to assign the selected items to a new milestone
:width: 300px
:::

## Step 3: Publish release

The release CI job should have created a draft release visible in the GitHub interface. If you navigate to the list of all releases, there should be an entry for this new release. You should

1. Inspect the auto-generated release note, and see if there are any obvious mistakes here that the job might have made.
2. Check that the target commit hash, that the release will generate a tag for once published, actually corresponds to the commit on the `releases` branch that the CI job from before created.
3. Click the *Publish* button to create the release.
