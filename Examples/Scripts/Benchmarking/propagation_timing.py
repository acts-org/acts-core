# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import ROOT
import csv
import matplotlib.pyplot as plt
import numpy as np

# Data preparation
dataDict = {}

# Open the output file
with open("output.log", mode="r") as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=",")
    # read lines and go for it
    for csv_row in csv_reader:
        if len(csv_row) > 1:
            # get the job id
            jobID = csv_row[0]
            # we need the exec time
            exectime = 0.0
            with open("timing_" + jobID + ".tsv") as tsv_file:
                tsv_reader = csv.reader(tsv_file, delimiter="\t")
                for tsv_row in tsv_reader:
                    if str(tsv_row[0]) == "Algorithm:PropagationAlgorithm":
                        exectime = float(tsv_row[2])
            # stepper and pt
            stepper = int(csv_row[1])
            ptvalue = float(csv_row[2])
            # now open the ROOT file and extract the numbers
            rfile = ROOT.TFile("propagation_steps_" + str(jobID) + ".root")
            stree = rfile.Get("propagation_steps")
            stree.Draw("@g_x->size()>>h_steps")
            h_steps = ROOT.gDirectory.Get("h_steps")
            steps = h_steps.GetMean()
            stepsSpread = h_steps.GetMeanError()

            # Make sure you have all the keys ready
            try:
                cdict = dataDict[ptvalue]
            except:
                dataDict[ptvalue] = {}
                cdict = dataDict[ptvalue]

            # Now fill the sub dictionary
            try:
                vdict = cdict[stepper]
            except:
                cdict[stepper] = []
                vdict = cdict[stepper]

            vdict += [steps, stepsSpread, exectime, exectime / steps]

# plot the dataDict
plt.figure(figsize=(16, 5))

ax = plt.subplot(131)
plt.loglog(
    dataDict.keys(),
    [i[0][0] for i in np.array(list(dataDict.values()))],
    "+",
    label="line",
)
plt.loglog(
    dataDict.keys(),
    [i[1][0] for i in np.array(list(dataDict.values()))],
    "*",
    label="eigen",
)
plt.loglog(
    dataDict.keys(),
    [i[2][0] for i in np.array(list(dataDict.values()))],
    "o",
    label="atlas",
)
ax.set_xlabel("$p_T$ [GeV]")
ax.set_ylabel("#steps")
ax.set_xlim((-10, 150))
plt.legend(numpoints=1)

ax = plt.subplot(132)
plt.loglog(dataDict.keys(), [i[0][2] for i in np.array(list(dataDict.values()))], "+")
plt.loglog(dataDict.keys(), [i[1][2] for i in np.array(list(dataDict.values()))], "*")
plt.loglog(dataDict.keys(), [i[2][2] for i in np.array(list(dataDict.values()))], "o")
ax.set_xlabel("$p_T$ [GeV]")
ax.set_ylabel("time [a.u.]")
ax.set_xlim((-10, 150))


ax = plt.subplot(133)
plt.loglog(dataDict.keys(), [i[0][3] for i in np.array(list(dataDict.values()))], "+")
plt.loglog(dataDict.keys(), [i[1][3] for i in np.array(list(dataDict.values()))], "*")
plt.loglog(dataDict.keys(), [i[2][3] for i in np.array(list(dataDict.values()))], "o")
ax.set_xlabel("$p_T$ [GeV]")
ax.set_ylabel("time/steps [a.u.]")
ax.set_xlim((-10, 150))


plt.suptitle("Stepper comparison: Constant Field")

plt.show()
