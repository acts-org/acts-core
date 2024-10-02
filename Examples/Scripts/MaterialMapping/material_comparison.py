import matplotlib.pyplot as plt
import numpy as np
import uproot
import math
from collections import namedtuple

FileRecord = namedtuple("FileRecord", ["name", "tree", "label", "color", "marker_size"])
PlotRecord = namedtuple(
    "PlotRecord", ["x_axis", "y_axis", "x_range", "x_bins", "x_label", "saveAs"]
)

# The file records
fileRecords = [
    FileRecord("geant4_material_tracks.root", "material-tracks", "Geant4", "blue", 3),
    FileRecord("acts_material_tracks.root", "material-tracks", "Acts", "orange", 4),
]


# The plot records
plotRecords = [
    PlotRecord("v_eta", "t_X0", (-4.0, 4.0), 80, "η", "tX0_vs_eta.svg"),
    PlotRecord("v_phi", "t_X0", (-math.pi, math.pi), 72, "φ", "tX0_vs_phi.svg"),
]

# Different plot records
for pr in plotRecords:

    fig, axs = plt.subplots(2, 1, sharex=True)
    fig.subplots_adjust(hspace=0.05)

    # Prepare limit & ratios
    y_lim = 0
    y_ratio_values = []
    y_ratio_errors = [0.0 for i in range(pr.x_bins)]

    # Loop over the file records
    for ifr, fr in enumerate(fileRecords):

        # Load the three
        tree = uproot.open(fr.name + ":" + fr.tree)

        x_arr = tree[pr.x_axis].array(library="np")
        y_arr = tree[pr.y_axis].array(library="np")
        y_max = y_arr.max()
        y_lim = y_max if y_max > y_lim else y_lim

        # Generate the central bin values
        x_step = (pr.x_range[1] - pr.x_range[0]) / pr.x_bins
        x_vals = [pr.x_range[0] + (ix + 0.5) * x_step for ix in range(pr.x_bins)]

        # Prepare the min /max
        y_min_vals = [1000.0] * pr.x_bins
        y_max_vals = [0.0] * pr.x_bins
        y_vals_sorted = [np.array([])] * pr.x_bins

        for iv in range(len(x_arr)):
            x_b = int((x_arr[iv] - pr.x_range[0]) / x_step)
            y_v = y_arr[iv]
            # Take min / max
            y_min_vals[x_b] = y_v if y_v < y_min_vals[x_b] else y_min_vals[x_b]
            y_max_vals[x_b] = y_v if y_v > y_max_vals[x_b] else y_max_vals[x_b]
            # Regulate the x value
            y_vals_sorted[x_b] = np.append(y_vals_sorted[x_b], y_v)

        axs[0].fill_between(
            x=x_vals,
            y1=y_min_vals,
            y2=y_max_vals,
            alpha=0.1,
            label=fr.label + " spread",
            color=fr.color,
        )
        axs[0].grid(axis="x")
        y_vals_mean = [y_bin_vals.mean() for y_bin_vals in y_vals_sorted]

        y_ratio_values += [y_vals_mean]
        y_vals_mse = [
            y_bin_vals.std() ** 2 / len(y_bin_vals) for y_bin_vals in y_vals_sorted
        ]

        axs[0].errorbar(
            x=x_vals,
            y=y_vals_mean,
            yerr=y_vals_mse,
            markersize=fr.marker_size,
            marker="o",
            mfc=fr.color if ifr == 0 else "none",
            linestyle="none",
            label=fr.label + " mean",
            color=fr.color,
        )

        if ifr > 0:
            y_ratios = [
                y_ratio_values[ifr][ib] / y_ratio_values[0][ib]
                for ib in range(pr.x_bins)
            ]
            axs[1].errorbar(
                x=x_vals,
                y=y_ratios,
                yerr=y_ratio_errors,
                markersize=fr.marker_size,
                marker="o",
                mfc="none",
                linestyle="none",
                color=fr.color,
                label=fr.label,
            )
            axs[1].set_ylabel("Ratio to " + fileRecords[0].label)

    # Some final cosmetics
    axs[0].set_ylim(0.0, y_lim)
    axs[0].grid(axis="x", linestyle="dotted")

    axs[1].set_ylim(0.9, 1.1)
    axs[1].grid(axis="x", linestyle="dotted")
    axs[1].axhline(y=1.0, color="black", linestyle="-")

    axs[0].legend(loc="upper center")
    axs[1].legend(loc="upper center")

    # Set the range of x-axis
    plt.xlabel(pr.x_label)
    fig.savefig(pr.saveAs)


plt.show()
