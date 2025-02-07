#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from datetime import datetime
import uproot
import pandas as pd
import numpy as np
import logging
import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import json
import os

from pathlib import Path


def run_error_parametriation(
    rfile,
    digi_cfg,
    volumes,
    output_dir=Path.cwd(),
    json_out="rms_out.json",
    break_min_stat=5000,
    break_rms_change=0.05,
    break_cluster_size=5,
    view_colors=["deepskyblue", "gold"],
    view_rms_range=5,
):
    # Create a figure directory
    output_dir.mkdir(parents=True, exist_ok=True)
    output_html_dir = output_dir / "html"
    output_html_dir.mkdir(parents=True, exist_ok=True)
    output_fig_dir = output_html_dir / "plots"
    output_fig_dir.mkdir(parents=True, exist_ok=True)

    volume_links = ""

    logging.info(f"Hit error parameterisation for {len(volumes)} volumes")

    var_dict = {}
    header_dict = {}
    header_dict["format-version"] = 0
    header_dict["value-identifier"] = "hit-error-parametrisation"
    var_dict["acts-geometry-hierarchy-map"] = header_dict

    var_entries = []

    # loop over the volumes
    for iv, v_id_n in enumerate(volumes):
        v_id, v_name = v_id_n

        logging.info(f"Processing volume {v_name} with ID: {v_id}")

        # previous and next volume
        prev_id = volumes[iv - 1] if iv > 0 else volumes[-1]
        next_id = volumes[iv + 1] if iv < len(volumes) - 1 else volumes[0]

        # Get the volume
        v_id_str = "vol" + str(v_id)
        vol = rfile[v_id_str].arrays(library="pd")

        # RMS matrix
        max_size_0 = 1
        max_size_1 = 1

        # We should be able to get this from the volume
        local_values = []
        if "clus_size_loc0" in vol.columns and vol["clus_size_loc0"].any():
            logging.info(f" - local 0 coorindate found")
            local_values.append(0)
        if "clus_size_loc1" in vol.columns and vol["clus_size_loc1"].any():
            local_values.append(1)
            logging.info(f" - local 1 coorindate found")

        var_matrix = np.zeros((2, break_cluster_size))
        var_entry = {"volume": v_id}
        var_data = []

        # write html content
        plots = []
        # Loop over the local variables
        for l in local_values:
            # Local var_data
            rms_local_values = {"index": l}
            rms_local_data = []
            # The plots per column
            lplots = []
            # Overview plot
            plt.hist(
                vol["clus_size_loc" + str(l)],
                bins=range(1, max(vol["clus_size_loc" + str(l)]) + 3),
                histtype="step",
                fill=True,
                color=view_colors[l],
            )
            plt.xlabel("Cluster size local " + str(l))
            plt.ylabel("Entries")
            # Create the svg path
            svg_path = output_fig_dir / f"{v_id_str}_clus_size_loc{l}.svg"
            plt.savefig(svg_path)
            lplots.append(svg_path)
            plt.clf()
            # Resolution plot, break
            max_clus_size = max(vol["clus_size_loc" + str(l)]) + 1
            if max_clus_size > break_cluster_size:
                max_clus_size = break_cluster_size
            # loop over the cluster sizes
            for c_size in range(1, max_clus_size):
                # Break conditions: not enough change, not enough statistics
                break_condition = False
                # Select the cluster size
                vol_sel = vol[vol["clus_size_loc" + str(l)] == c_size]
                # Plot the resolution
                res = vol_sel["rec_loc" + str(l)] - vol_sel["true_loc" + str(l)]
                rms = np.std(res)
                var_matrix[l, c_size] = rms * rms
                rms_local_data.append(float(rms * rms))
                c_size_flag = str(c_size)
                # Peak into next selection
                next_sel = vol[vol["clus_size_loc" + str(l)] == c_size + 1]
                if not next_sel.empty:
                    # Check if enough statistics
                    next_res = (
                        next_sel["rec_loc" + str(l)] - next_sel["true_loc" + str(l)]
                    )
                    if (
                        len(next_sel) < break_min_stat
                        or abs(rms - np.std(next_res)) / rms < break_rms_change
                    ):
                        # Recaluate with rest
                        vol_sel = vol[vol["clus_size_loc" + str(l)] >= c_size]
                        res = vol_sel["rec_loc" + str(l)] - vol_sel["true_loc" + str(l)]
                        # Set the new cluster size
                        c_size_flag = "N"
                        # Set the break condition
                        break_condition = True

                # Plot the resolution within +/- n rms
                plt.hist(
                    res,
                    bins=100,
                    range=(-view_rms_range * rms, view_rms_range * rms),
                    histtype="step",
                    fill=True,
                    color=view_colors[l],
                )
                plt.text(
                    0.05,
                    0.95,
                    "rms = " + str(round(rms, 3)),
                    transform=plt.gca().transAxes,
                    fontsize=14,
                    verticalalignment="top",
                )
                plt.xlabel(
                    "Resolution - local " + str(l) + ", cluster size " + c_size_flag
                )
                # Save the figure
                svg_path = (
                    output_fig_dir / f"{v_id_str}_res_loc{l}_clus_size{c_size_flag}.svg"
                )
                plt.savefig(svg_path)
                lplots.append(svg_path)
                plt.clf()
                if break_condition:
                    break
            # Add the rms data
            rms_local_values["rms"] = rms_local_data
            var_data.append(rms_local_values)
            # Add the plots to the column
            plots.append(lplots)

        # Add the rms data to the dictionary
        var_entry["value"] = var_data
        var_entries.append(var_entry)
        var_dict["entries"] = var_entries

        # Write the rms dictionary
        if digi_cfg is not None:
            # Update the json
            digi_cfg_entries = digi_cfg["entries"]
            for entry in digi_cfg_entries:
                if entry["volume"] == v_id:
                    entry["value"]["geometric"]["variances"] = var_data

            with open(json_out, "w") as outfile:
                json.dump(digi_cfg, outfile, indent=4)
        else:
            with open(json_out, "w") as outfile:
                json.dump(var_dict, outfile, indent=4)

        # The matrix plot
        fig, ax = plt.subplots(ncols=1, nrows=1)
        pos = ax.matshow(var_matrix, cmap="Blues")
        plt.xlabel("Cluster size")
        plt.ylabel("Local coordinate")
        plt.title(v_name)
        fig.colorbar(pos, ax=ax)
        svg_path = output_fig_dir / f"{v_id_str}_summary.svg"
        plt.savefig(svg_path)
        plt.clf()

        # Create the html content
        plot_content = ""

        for ip in range(max([len(p) for p in plots])):
            for ic in range(len(plots)):
                if ip < len(plots[ic]):
                    plot_content += f"<div>{plots[ic][ip].read_text()}</div>"
                else:
                    plot_content += f"<div></div>"

        volume_links += f'<div><a href="html/volume_{v_id}.html"><div>{svg_path.read_text()}</div></a></div>'

        volume_file = output_html_dir / f"volume_{v_id}.html"
        previous_file = output_html_dir / f"volume_{prev_id[0]}.html"
        next_file = output_html_dir / f"volume_{next_id[0]}.html"

        volume_file.write_text(
            """<!DOCTYPE html>
<html>
<head>
    <title>Error Parameterisation</title>
    <style>
    .wrapper {{
        max-width: 1500px;
        margin: 0 auto;
    }}
    .grid {{
        display: grid;
        grid-template-columns: repeat(2, 50%);
    }}
    .grid svg {{
        width:100%;
        height:auto;
    }}
    </style>
</head>
<body>
<div class="wrapper">
    <a href="{previous}">Previous volume</a> |
    <a href="../index.html">Back to index</a> |
    <a href="{next}">Next volume</a><br>
    <h1>Error Parameterisation : volume {vid} </h1>
    Generated: {date}<br>
    <div class="grid">
    {content}
    </div>
</div>
</body>
</html>
    """.format(
                vid=v_id,
                previous=str(previous_file),
                next=str(next_file),
                content=plot_content,
                date=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            )
        )

        # Write the index file
        index_file = output_dir / "index.html"
        index_file.write_text(
            """<!DOCTYPE html>
<html>
<body>
<div class="wrapper">
<h1>Error Parameterisation</h1>
Generated: {date}<br>
<div class="grid">
{volume_content}
</div>
</div>
</body>
</html>
        """.format(
                date=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                volume_content=volume_links,
            )
        )


# Main function
if "__main__" == __name__:
    # Parse the command line arguments
    p = argparse.ArgumentParser(description="Hit parameterisation")
    p.add_argument("--root")
    p.add_argument("--json-in")
    p.add_argument("--json-out")
    args = p.parse_args()

    # Open the root file
    rfile = uproot.open(args.root)
    volumes = [
        (16, "Pixel NEC"),
        (17, "Pixel Barrel"),
        (18, "Pixel PEC"),
        (23, "SStrips NEC"),
        (24, "SStrips Barrel"),
        (25, "SStrips PEC"),
        (28, "LStrips NEC"),
        (29, "LStrips Barrel"),
        (30, "LStrips PEC"),
    ]

    # Open the json to be updated
    digi_cfg = None
    if (
        args.json_in is not None
        and os.path.isfile(args.json_in)
        and os.access(args.json_in, os.R_OK)
    ):
        jfile = open(args.json_in, "r")
        digi_cfg = json.load(jfile)

    logging.basicConfig(encoding="utf-8", level=logging.INFO)

    run_error_parametriation(
        rfile, digi_cfg, volumes, Path.cwd() / "output", args.json_out
    )
