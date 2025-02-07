# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import csv
import matplotlib.pyplot as plt
import numpy as np

# Data preparation
muDict = {}

# Open the output file
with open("output.log", mode="r") as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=",")
    # read lines and go for it
    for csv_row in csv_reader:
        if len(csv_row) > 1:
            # get the job id
            jobID = csv_row[0]
            # mu, mode, exec time
            mode = float(csv_row[1])
            mu = int(csv_row[2])
            exectime = float(csv_row[3])

            # Make sure you have all the keys ready
            try:
                mdict = muDict[mu]
            except:
                muDict[mu] = {}
                mdict = muDict[mu]

            # Now fill the sub dictionary
            try:
                vmdict = mdict[mode]
            except:
                mdict[mode] = []
                vmdict = mdict[mode]

            vmdict += [exectime]

# plot the muDict
plt.figure(figsize=(7, 5))

ax = plt.subplot(111)
plt.plot(
    muDict.keys(),
    [i[0][0] for i in np.array(list(muDict.values()))],
    "-+",
    label="$\chi^{2}$<15, $n_{source link}	<=10$",
)
plt.plot(
    muDict.keys(),
    [i[1][0] for i in np.array(list(muDict.values()))],
    "-*",
    label="$\chi^{2}$<10, $n_{source link}=1$",
)
ax.set_xlabel("$<\mu>$")
ax.set_ylabel("time/event [Sec]")
ax.set_xlim((0, 250))
plt.legend(numpoints=1)

plt.suptitle("CKF timing vs. $<\mu>$")

plt.show()
