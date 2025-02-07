# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import csv
import matplotlib.pyplot as plt
import numpy as np

# Data preparation
ptDict = {}

# Open the output file
with open("output.log", mode="r") as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=",")
    # read lines and go for it
    for csv_row in csv_reader:
        if len(csv_row) > 1:
            # get the job id
            jobID = csv_row[0]
            # etabin, pt value, exec time
            etabin = float(csv_row[1])
            ptvalue = float(csv_row[2])
            exectime = float(csv_row[3])

            # Make sure you have all the keys ready
            try:
                pdict = ptDict[ptvalue]
            except:
                ptDict[ptvalue] = {}
                pdict = ptDict[ptvalue]

            # Now fill the sub dictionary
            try:
                vpdict = pdict[etabin]
            except:
                pdict[etabin] = []
                vpdict = pdict[etabin]

            vpdict += [exectime]

# plot the ptDict
plt.figure(figsize=(7, 5))

ax = plt.subplot(111)
plt.loglog(
    ptDict.keys(),
    [i[0][0] for i in np.array(list(ptDict.values()))],
    ".-",
    label="0<$\eta$<0.5",
)
plt.loglog(
    ptDict.keys(),
    [i[1][0] for i in np.array(list(ptDict.values()))],
    ".-",
    label="0.5<$\eta$<1.0",
)
plt.loglog(
    ptDict.keys(),
    [i[2][0] for i in np.array(list(ptDict.values()))],
    ".-",
    label="1.0<$\eta$<1.5",
)
plt.loglog(
    ptDict.keys(),
    [i[3][0] for i in np.array(list(ptDict.values()))],
    ".-",
    label="1.5<$\eta$<2.0",
)
plt.loglog(
    ptDict.keys(),
    [i[4][0] for i in np.array(list(ptDict.values()))],
    ".-",
    label="2.0<$\eta$<2.5",
)
ax.set_xlabel("$p_T$ [GeV/c]")
ax.set_ylabel("time/track [sec]")
plt.yscale("log")
ax.set_xlim((0.09, 105))
plt.legend(numpoints=1)

plt.suptitle("KF timing vs. $p_T$")

plt.show()
