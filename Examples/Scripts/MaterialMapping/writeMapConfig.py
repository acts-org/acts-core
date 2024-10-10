# This file is part of the ACTS project.
#
# Copyright (C) 2016 CERN for the benefit of the ACTS project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import sys

# Should be run with Python 3 if possible
# Script that parse a Json surfaces map to create an easy to use json config file for the mapping
# Take two arguments in input : The path to the surfaces map and the path of the json config file
# By default the input is : 'geometry-map.json' and the output is : 'config-map.json'
# The config file can be used to define a binning for all the surfaces in a given volume
# It can also be used to define the binning for volume mapping


def getSurfaceMaterial(mat):
    outputmat = {}
    value = {}
    material = {}
    bound = {}
    outputmat["volume"] = mat["volume"]
    if "boundary" in mat:
        outputmat["boundary"] = mat["boundary"]
    if "layer" in mat:
        if "approach" not in entry:
            if "sensitive" not in entry:
                outputmat["layer"] = "X"
    if "approach" in mat:
        outputmat["approach"] = mat["approach"]
    if "sensitive" in mat:
        outputmat["layer"] = mat["layer"]
        outputmat["sensitive"] = "X"
    material["binUtility"] = mat["value"]["material"]["binUtility"]
    material["mapMaterial"] = False
    material["mappingType"] = mat["value"]["material"]["mappingType"]
    bound["type"] = mat["value"]["bounds"]["type"]
    value["material"] = material
    value["bounds"] = bound
    outputmat["value"] = value
    return outputmat


if sys.version_info[0] < 3:
    print("Using Python 2")
    print("To obtain the proper ordering in the Json files Python 3 is recommended")

if len(sys.argv) < 2:
    inFileName = "geometry-map.json"
else:
    inFileName = sys.argv[1]


with open(inFileName, "r") as json_file:
    config = {}
    config["Surfaces"] = {}
    data = json.load(json_file)
    lastVol = -1
    for entry in data["Surfaces"]["entries"]:
        if lastVol != entry["volume"]:
            if lastVol != -1:
                config["Surfaces"][lastVol] = vconfig
            vconfig = []
            lastVol = entry["volume"]
            typeLayer = []
            createdApproach1 = False
            createdApproach2 = False
            typeSensitive = {}

        if "type" not in entry["value"]["bounds"]:
            entry["value"]["bounds"]["type"] = ""

        if "layer" in entry:
            if "approach" not in entry:
                if "sensitive" not in entry:
                    if entry["value"]["bounds"]["type"] not in typeLayer:
                        typeLayer.append(entry["value"]["bounds"]["type"])
                        surface = getSurfaceMaterial(entry)
                        vconfig.append(surface)
                        continue

        if "boundary" in entry:
            if "layer" not in entry:
                surface = getSurfaceMaterial(entry)
                vconfig.append(surface)
                continue

        if "approach" in entry:
            if "sensitive" not in entry:
                if entry["approach"] == 1 and createdApproach1 == False:
                    createdApproach1 = True
                    surface = getSurfaceMaterial(entry)
                    vconfig.append(surface)
                    continue
                if entry["approach"] == 2 and createdApproach2 == False:
                    createdApproach2 = True
                    surface = getSurfaceMaterial(entry)
                    vconfig.append(surface)
                    continue

        if "sensitive" in entry:
            if "approach" not in entry:
                if entry["value"]["material"]["binUtility"]["binningdata"] != None:
                    if not entry["layer"] in typeSensitive:
                        typeSensitive[entry["layer"]] = []
                    if (
                        entry["value"]["bounds"]["type"]
                        not in typeSensitive[entry["layer"]]
                    ):
                        typeSensitive[entry["layer"]].append(
                            entry["value"]["bounds"]["type"]
                        )
                        surface = getSurfaceMaterial(entry)
                        vconfig.append(surface)
                        continue

    if lastVol != -1:
        config["Surfaces"][lastVol] = vconfig
    config["Volumes"] = data["Volumes"]

if len(sys.argv) < 3:
    outFileName = "config-map.json"
else:
    outFileName = sys.argv[2]

with open(outFileName, "w") as outfile:
    json.dump(config, outfile, indent=4)
