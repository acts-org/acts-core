# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import ROOT
import argparse, math

from ROOT import TCanvas, TFile, TTree, TLine, TArrow, gStyle
from array import array

if "__main__" == __name__:
    p = argparse.ArgumentParser()

    p.add_argument(
        "-i",
        "--input",
        type=str,
        default="",
        help="Input file with material tracks used from the mapping",
    )
    p.add_argument(
        "-o",
        "--output",
        type=str,
        default="",
        help="Output file with material mapping analysis",
    )
    p.add_argument(
        "-d",
        "--distance",
        type=float,
        default=50,
        help="Maximum distance to the surface",
    )
    p.add_argument(
        "-e",
        "--entries",
        type=int,
        default=10000,
        help="Number of draw entries",
    )
    p.add_argument(
        "--z-range",
        type=float,
        nargs=2,
        default=[-3000, 3000],
        help="Drawing z range",
    )
    p.add_argument(
        "--r-range",
        type=float,
        nargs=2,
        default=[0, 1200],
        help="Drawing z range",
    )

    p.add_argument(
        "--arrows",
        action=argparse.BooleanOptionalAction,
        help="Draw arrows for the material projection",
        default=False,
    )

    p.add_argument(
        "--eta-lines",
        type=float,
        nargs="+",
        default=[
            -3.5,
            -3.25,
            -3,
            -2.75,
            -2.5,
            -2.25,
            -2,
            -1.75,
            -1.5,
            -1.25,
            -1,
            -0.75,
            -0.5,
            -0.25,
            0,
            0.25,
            0.5,
            0.75,
            1,
            1.25,
            1.5,
            1.75,
            2,
            2.25,
            2.5,
            2.75,
            3,
            3.25,
            3.5,
        ],
        help="Drawing z range",
    )

    args = p.parse_args()

    # Bailout
    if args.input == "":
        print("** ERROR ** The input file must be provided")
        exit(1)

    # Some basic style
    gStyle.SetOptStat(0)
    gStyle.SetOptTitle(0)

    # File handling
    tfile = TFile.Open(args.input, "READ")
    ttree = tfile.Get("material-tracks")

    # {\displaystyle \theta =2\arctan \left(e^{-\eta }\right).}
    def theta_from_eta(eta):
        return 2 * math.atan(math.exp(-eta))

    # Calculate the theta cut
    theta_cut = math.atan2(args.r_range[1], args.z_range[1])

    # The eta line
    def draw_eta_line(
        eta: float, theta_cut, z_max=args.z_range[1], r_max=args.r_range[1]
    ):
        theta = theta_from_eta(eta)
        # Inside barrel line
        if theta > theta_cut and theta < math.pi - theta_cut:
            r_line = r_max
            z_line = r_line / math.tan(theta)
        else:
            # Outside barrel line
            z_line = z_max
            if theta > 0.5 * math.pi:
                z_line = args.z_range[0]
            r_line = z_line * math.tan(theta)
        # Create the line
        line = TLine(0, 0, z_line, r_line)
        line.SetLineColor(ROOT.kGray)
        if not float(eta).is_integer():
            if float(eta) % 1 == 0.25 or float(eta) % 1 == 0.75:
                line.SetLineStyle(ROOT.kDotted)
            else:
                line.SetLineStyle(ROOT.kDashed)
        return line

    c = TCanvas("CheckCanvas", "Check Material Mapping", 1200, 1200)
    c.Divide(1, 2)

    lines_store = []

    # Overall plot
    c.cd(1)
    hcmd = f"mat_r:mat_z>>d1(100,{args.z_range[0]},{args.z_range[1]},100,{args.r_range[0]},{args.r_range[1]})"
    ttree.Draw(
        hcmd,
        "sur_distance>" + str(args.distance) + "; z [mm]; r [mm]",
        "colz",
        args.entries,
    )
    ttree.Draw("sur_r:sur_z", "", "same", args.entries)

    for eta in args.eta_lines:
        eta_line = draw_eta_line(eta, theta_cut)
        if eta_line:
            eta_line.Draw("same")
            lines_store.append(eta_line)
    c.Update()

    # Detailed plot
    c.cd(2)
    hcmd = f"mat_r:mat_z>>d2(100,{args.z_range[0]},{args.z_range[1]},100,{args.r_range[0]},{args.r_range[1]})"
    ttree.Draw(hcmd + "; z [mm]; r [mm]", "", "", args.entries)

    for ie in range(args.entries):
        ttree.GetEntry(ie)
        steps = len(ttree.sur_distance)
        for si, sd in enumerate(ttree.sur_distance):
            if sd > args.distance:
                if args.arrows:
                    line = TArrow(
                        ttree.mat_z[si],
                        ttree.mat_r[si],
                        ttree.sur_z[si],
                        ttree.sur_r[si],
                        0.01,
                        ">",
                    )
                    line.SetLineColor(ROOT.kRed)
                    line.Draw(">")
                else:
                    line = TLine(
                        ttree.sur_z[si],
                        ttree.sur_r[si],
                        ttree.mat_z[si],
                        ttree.mat_r[si],
                    )
                    line.SetLineColor(ROOT.kRed)
                    line.Draw("same")
                lines_store.append(line)

    for eta in args.eta_lines:
        eta_line = draw_eta_line(eta, theta_cut)
        if eta_line:
            eta_line.Draw("same")
            lines_store.append(eta_line)
