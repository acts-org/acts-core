# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import re
import copy
from itertools import cycle

import numpy as np

# This should allow to run the non-gui mode in the CI
try:
    from matplotlib.pyplot import subplots
except:

    def subplots(*args, **kwargs):
        raise RuntimeError("Matplotlib could not be imported")


class AverageTrackPlotter:
    def __init__(self, view_drawers, annotate_steps=False):
        self.annotate_steps = annotate_steps

        self.positions = []
        self.ibackward = None

        self.view_drawers = view_drawers

    def parse_step(self, step):
        for line in step:
            if line.count("Do backward propagation") == 1:
                self.ibackward = len(self.positions)

            elif line.count("at mean position") == 1:
                line = re.sub(r"^.*position", "", line)
                line = re.sub(r"with.*$", "", line)

                self.positions.append(np.array([float(item) for item in line.split()]))

    def name(self):
        return "Average track"

    def get_figure_axes(self):
        return subplots(1, len(self.view_drawers))

    def draw(self, fig, axes, step):
        for ax, drawer in zip(axes, self.view_drawers):
            ax = drawer.draw_detector(ax)

        fwd_to_plot = self.positions[: self.ibackward]
        bwd_to_plot = self.positions[self.ibackward :]

        if step < len(fwd_to_plot):
            fwd_to_plot = fwd_to_plot[:step]
            bwd_to_plot = []
        else:
            bwd_steps = step - len(fwd_to_plot)
            bwd_to_plot = bwd_to_plot[:bwd_steps]

        positions = [fwd_to_plot, bwd_to_plot]
        names = ["forward", "backward"]

        for positions, direction in zip(positions, names):
            if len(positions) == 0:
                continue

            positions = np.vstack(positions)

            for ax, drawer in zip(axes, self.view_drawers):
                ax = drawer.plot(ax, positions, label=direction, marker="x", lw=1)

                if self.annotate_steps:
                    for i, p in enumerate(positions):
                        ax = drawer.annotate(ax, p, str(i))

                ax = drawer.set_title(ax)
                ax.legend()

        fig.tight_layout()
        return fig, axes


class ComponentsPlotter:
    def __init__(self, view_drawers):
        self.current_direction = "forward"
        self.steps = []

        self.view_drawers = view_drawers

        self.colors = [
            "red",
            "orangered",
            "orange",
            "gold",
            "olive",
            "forestgreen",
            "lime",
            "teal",
            "cyan",
            "blue",
            "indigo",
            "magenta",
            "brown",
        ]

    def parse_step(self, step):
        surface_name = None
        component_sets = []
        components = []

        for line in step:
            if line.count("Step is at surface") == 1:
                surface_name = line[line.find("vol") :]

            elif line.count("Do backward propagation") == 1:
                self.current_direction = "backward"

            elif re.match(r"^.*#[0-9]+\spos", line):
                line = line.replace(",", "")
                splits = line.split()

                current_cmp = int(splits[3][1:])
                pos = np.array([float(part) for part in splits[5:8]])

                # There can be two sets of components printed in a step
                if current_cmp < len(components):
                    component_sets.append(copy.deepcopy(components))
                    components = []

                components.append(pos)

        component_sets.append(copy.deepcopy(components))
        assert len(component_sets) <= 2

        self.steps.append(
            (
                surface_name,
                self.current_direction,
                component_sets,
            )
        )

    def name(self):
        return "Components"

    def get_figure_axes(self):
        return subplots(1, len(self.view_drawers))

    def number_steps(self):
        return sum([len(s[3]) for s in self.stages])

    def draw(self, fig, axes, requested_step):
        kSurface = 0
        kDirection = 1
        kComponents = 2

        for ax, drawer in zip(axes, self.view_drawers):
            ax = drawer.draw_detector(ax)

        if requested_step >= len(self.steps):
            fig.suptitle("Step out of range")
            return fig, axes

        n_components = len(self.steps[requested_step][kComponents][-1])

        # go back to last surface
        start_surface = "<unknown>"
        positions = []
        for si in range(requested_step, 0, -1):
            if len(self.steps[si][kComponents][-1]) != n_components:
                fig.suptitle("error: component number mismatch")
                return fig, axes
            positions.append(self.steps[si][kComponents][-1])

            if self.steps[si][kSurface] is not None:
                start_surface = self.steps[si][kSurface]
                break

        fig.suptitle(
            "Stepping {} with {} components from {}".format(
                self.steps[requested_step][kDirection],
                n_components,
                start_surface.strip(),
            )
        )

        n_steps = len(positions)
        if n_steps == 0:
            return fig, axes

        try:
            positions = np.array(positions)
            assert len(positions.shape) == 3

            assert positions.shape[0] == n_steps
            assert positions.shape[1] == n_components
            assert positions.shape[2] == 3

            for i, color in zip(range(n_components), cycle(self.colors)):
                for ax, drawer in zip(axes, self.view_drawers):
                    ax = drawer.plot(ax, positions[:, i, :], c=color, marker="x", lw=1)
        except:
            fig.suptitle("Error drawing")
            import pprint

            pprint.pprint(positions)

        fig.tight_layout()
        return fig, axes


class GsfMomentumRecorder:
    def __init__(self):
        # Global state
        self.gsf_started_backwards = False
        self.gsf_accumulated_pathlength = 0
        self.printed_qop_warning = False

        # Recordings
        self.gsf_momenta = []
        self.gsf_cmp_data = []
        self.gsf_pathlengths = []

        # Current step
        self.gsf_current_step_cmp_momenta = []
        self.gsf_current_step_cmp_weights = []
        self.gsf_have_momentum = False
        self.gsf_last_step_number = None

    def parse_line(self, line):
        if line.count("Gsf step") == 1:
            # Last step appears twice in log, prevent this
            if int(line.split()[5]) == self.gsf_last_step_number:
                return

            self.gsf_last_step_number = int(line.split()[5])

            # Update component states if not in first step
            if len(self.gsf_current_step_cmp_momenta) > 0:
                self.gsf_cmp_data.append(
                    (
                        self.gsf_current_step_cmp_momenta,
                        self.gsf_current_step_cmp_weights,
                    )
                )
                self.gsf_current_step_cmp_momenta = []
                self.gsf_current_step_cmp_weights = []

            # Save momentum
            assert len(self.gsf_momenta) == len(self.gsf_pathlengths)
            self.gsf_momenta.append(float(line.split()[-4]))
            self.gsf_have_momentum = True

        elif re.match(r"^.*#[0-9]+\spos", line) and not self.gsf_started_backwards:
            line = line.replace(",", "")
            qop = float(line.split()[-3])
            p = abs(1 / qop)
            w = float(line.split()[-7])
            self.gsf_current_step_cmp_momenta.append(p)
            self.gsf_current_step_cmp_weights.append(w)

        elif line.count("Step with size") == 1:
            self.gsf_accumulated_pathlength += float(line.split()[-2])

            if self.gsf_have_momentum:
                self.gsf_have_momentum = False
                self.gsf_pathlengths.append(self.gsf_accumulated_pathlength)
                assert len(self.gsf_pathlengths) == len(self.gsf_momenta)


class MomentumGraph:
    def __init__(self):
        self._flipped = False

        self.momenta = []
        self.pathlenghts = []
        self.iBackward = None

    def parse_step(self, step):
        mom = None
        pl = None

        for line in step:
            if line.count("Gsf step") == 1:
                mom = float(line.split()[-4])

            elif line.count("Step with size") == 1:
                pl = float(line.split()[-2])

            if line.count("Do backward") == 1:
                self.iBackward = len(self.momenta)

        if mom is None or pl is None:
            return

        self.momenta.append(mom)
        self.pathlenghts.append(pl)

    def name(self):
        return "Momentum"

    def get_figure_axes(self):
        return subplots()

    def draw(self, fig, ax, requested_step):
        fwd_mom, bwd_mom, fwd_pls, bwd_pls = [], [], [], []

        if self.iBackward is None:
            fwd_mom = self.momenta
            fwd_pls = self.pathlenghts
        else:
            fwd_mom = self.momenta[: self.iBackward]
            fwd_pls = self.pathlenghts[: self.iBackward]

            bwd_mom = self.momenta[self.iBackward :]
            bwd_pls = self.pathlenghts[self.iBackward :]

        fwd_pls = np.cumsum(fwd_pls)
        ax.plot(fwd_pls, fwd_mom, color="tab:blue")

        bwd_pls = np.cumsum(bwd_pls)
        bwd_pls = max(abs(bwd_pls)) + bwd_pls
        ax.plot(bwd_pls, bwd_mom, color="tab:orange")

        if requested_step < self.iBackward:
            ax.vlines(
                fwd_pls[requested_step], *ax.get_ylim(), alpha=0.5, color="tab:blue"
            )
        else:
            s = requested_step - self.iBackward
            if s < len(bwd_pls):
                ax.vlines(bwd_pls[s], *ax.get_ylim(), alpha=0.5, color="tab:orange")


class BoundParametersProcessor:
    def __init__(self, key):
        assert key == "Filtered" or key == "Predicted"
        self.key = key
        self.pars_pattern = "^.*" + self.key + " parameters:(.*)$"
        self.cov_preamble = "^.*" + self.key + " covariance:$"

        self.step_data = []

    def parse_step(self, step):
        weights = []
        parameters = []
        covs = []
        for i in range(len(step)):
            line = step[i]

            m = re.match(r"^.*weight: (.*), status:.*$", line)
            if m:
                weights.append(float(m[1]))

            if not (
                f"{self.key} parameters" in line or f"{self.key} covariance" in line
            ):
                continue

            m = re.match(self.pars_pattern, line)
            if m:
                pars = np.array([float(f) for f in m[1].split()])
                assert len(pars) == 6
                parameters.append(pars)
                continue

            m = re.match(self.cov_preamble, line)

            if m:
                cov = []
                for j in range(i + 1, i + 7):
                    cov.append([float(f) for f in step[j].split()])
                covs.append(np.array(cov))
                assert covs[-1].shape == (6, 6)
                i = i + 6

        assert len(parameters) == len(covs)
        assert len(weights) >= len(parameters)
        if len(parameters) > 0:
            self.step_data.append((weights[: len(parameters)], parameters, covs))
        else:
            self.step_data.append(None)

    def name(self):
        return f"{self.key} state"

    def get_figure_axes(self):
        return subplots(2, 3)

    def draw(self, fig, axes, requested_step):
        import scipy

        if requested_step >= len(self.step_data):
            fig.suptitle("Error: Step out of bound")
            return fig, axes

        if self.step_data[requested_step] is None:
            fig.suptitle("nothing to draw, not on surface")
            return fig, axes

        ws, pars, covs = self.step_data[requested_step]

        w_str = ", ".join([f"{w:.2f}" for w in ws])
        fig.suptitle(f"draw {len(ws)} components\nweights: {w_str}")

        j_max = np.argmax(ws)

        for i, (ax, title) in enumerate(
            zip(axes.flatten(), ["l0", "l1", "theta", "phi", "qop", "t"])
        ):
            cmps = [(ws[j], pars[j][i], covs[j][i, i]) for j in range(len(ws))]

            # minx = min([ m[1]-3*np.sqrt(m[2]) for m in cmps ])
            # maxx = max([ m[1]+3*np.sqrt(m[2]) for m in cmps ])
            minx = cmps[j_max][1] - 3 * np.sqrt(cmps[j_max][2])
            maxx = cmps[j_max][1] + 3 * np.sqrt(cmps[j_max][2])

            minx = min(minx, min([m[1] for m in cmps if m[0] > 0.05]))
            maxx = max(maxx, max([m[1] for m in cmps if m[0] > 0.05]))

            # minx = pars[0][i]-3*covs[0]
            mixture = lambda x: sum(
                [
                    w * scipy.stats.norm(loc=mu, scale=np.sqrt(var)).pdf(x)
                    for w, mu, var in cmps
                ]
            )

            x = np.linspace(minx, maxx, 200)
            y = sum(
                [
                    w * scipy.stats.norm(loc=mu, scale=np.sqrt(var)).pdf(x)
                    for w, mu, var in cmps
                ]
            )

            if len(ws) > 1:
                for w, mu, var in cmps:
                    ax.plot(
                        x, w * scipy.stats.norm(loc=mu, scale=np.sqrt(var)).pdf(x), lw=1
                    )

            ax.plot(x, y, lw=3, color="black")
            # ax.plot(x, [ scipy.stats.norm(loc=pars[0][i], scale=np.sqrt(covs[0][i,i])).pdf(xx) for xx in x])
            ax.set_title(title)

        fig.tight_layout()
        return fig, ax
