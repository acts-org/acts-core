#!/usr/bin/env python3
import argparse
import os
import sys
import copy

from processors import (
    AverageTrackPlotter,
    ComponentsPlotter,
    MomentumGraph,
    BoundParametersProcessor,
)


def main():
    # fmt: off
    parser = argparse.ArgumentParser(description='GSF Debugger')
    parser.add_argument('--detector', '-d', help="detector description as csv file", type=str)
    parser.add_argument('--logfile', '-f', help="log file (if not given, stdin is read)", type=str)
    parser.add_argument('--nogui', action="store_true", default=False, help="for testing the log parsing (works without Qt)")
    parser.add_argument('--view', choices=["cylindrical", "telescope"], default="cylindrical", help="cylindrical=zr+xy, telescope=zx,xy")
    # fmt: on
    args = vars(parser.parse_args())

    # Try to be smart an load the detectors.csv from the current working dir if possible
    if args["detector"] is None and "detectors.csv" in os.listdir():
        args["detector"] = "detectors.csv"

    # If no logfile is given, read from stdin (for piping)
    if args["logfile"] is not None:
        with open(args["logfile"], "r") as f:
            lines = f.readlines()
    else:
        print("Read from standard input...")
        lines = sys.stdin.readlines()

    # Group the log into steps to forward it to the processors
    # TODO this is maybe not super reliable and also highly GSF specific
    steps = []
    current_step = []
    for line in lines:
        if line.count("at mean position") == 1:
            steps.append(copy.deepcopy(current_step))
            current_step = []

        current_step.append(line)

    # Initialize the drawers
    if args["nogui"]:
        drawers = None
    else:
        from drawers import CsvZRDrawer, CsvXYDrawer, CsvXZDrawer

        if args["view"] == "cylindrical":
            drawers = [CsvZRDrawer(args["detector"]), CsvXYDrawer(args["detector"])]
        elif args["view"] == "telescope":
            drawers = [
                CsvXZDrawer(args["detector"], assume_telescope=True),
                CsvXYDrawer(args["detector"], assume_telescope=True),
            ]

    # Initialize the processors
    processors = [
        AverageTrackPlotter(drawers),
        ComponentsPlotter(drawers),
        MomentumGraph(),
        BoundParametersProcessor("Predicted"),
        BoundParametersProcessor("Filtered"),
    ]

    # Parse the step in each processor
    for step in steps:
        for processor in processors:
            processor.parse_step(step)

    # Run the Qt app
    if not args["nogui"]:
        from PyQt5 import QtWidgets
        from widgets import MainWindow

        app = QtWidgets.QApplication(sys.argv)
        w = MainWindow(processors, steps)
        app.exec_()


if __name__ == "__main__":
    main()
