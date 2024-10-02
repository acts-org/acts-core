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
# Script that use the json config file to configure the Json surfaces map for the material mapping
# Take two arguments in input : The path to the surfaces map and the path of the json config file
# By default the inputs are : 'geometry-map.json' and 'config-map.json'
# The config file can be used to define a binning for all the surfaces in a given volume
# It can also be used to define the binning for volume mapping

if sys.version_info[0] < 3:
    print("Using Python 2")
    print("To obtain the proper ordering in the Json files Python 3 is recomanded")

if len(sys.argv) < 2:
    inFileName = "geometry-map.json"
    confFileName = "config-map.json"

if len(sys.argv) < 3:
    confFileName = "config-map.json"

else:
    inFileName = sys.argv[1]
    confFileName = sys.argv[2]


with open(inFileName, "r+") as json_file:
    with open(confFileName, "r") as config_file:
        config = json.load(config_file)
        data = json.load(json_file)

        for entry in data["Surfaces"]["entries"]:
            if "type" not in entry["value"]["bounds"]:
                entry["value"]["bounds"]["type"] = ""

            if "layer" in entry:
                if "approach" not in entry:
                    if "sensitive" not in entry:
                        for conf in config["Surfaces"][str(entry["volume"])]:
                            if (
                                "layer" in conf
                                and conf["layer"] == "X"
                                and conf["value"]["bounds"]["type"]
                                == entry["value"]["bounds"]["type"]
                            ):
                                entry["value"]["material"]["mapMaterial"] = conf[
                                    "value"
                                ]["material"]["mapMaterial"]
                                entry["value"]["material"]["mappingType"] = conf[
                                    "value"
                                ]["material"]["mappingType"]
                                ibin = 0
                                for bin in entry["value"]["material"]["binUtility"][
                                    "binningdata"
                                ]:
                                    bin["bins"] = conf["value"]["material"][
                                        "binUtility"
                                    ]["binningdata"][ibin]["bins"]
                                    ibin = ibin + 1
                                continue
                        continue

            if "boundary" in entry:
                if "layer" not in entry:
                    for conf in config["Surfaces"][str(entry["volume"])]:
                        if (
                            "boundary" in conf
                            and conf["boundary"] == entry["boundary"]
                            and conf["value"]["bounds"]["type"]
                            == entry["value"]["bounds"]["type"]
                        ):
                            entry["value"]["material"]["mapMaterial"] = conf["value"][
                                "material"
                            ]["mapMaterial"]
                            entry["value"]["material"]["mappingType"] = conf["value"][
                                "material"
                            ]["mappingType"]
                            ibin = 0
                            for bin in entry["value"]["material"]["binUtility"][
                                "binningdata"
                            ]:
                                bin["bins"] = conf["value"]["material"]["binUtility"][
                                    "binningdata"
                                ][ibin]["bins"]
                                ibin = ibin + 1
                            continue
                    continue

            if "approach" in entry:
                if "sensitive" not in entry:
                    for conf in config["Surfaces"][str(entry["volume"])]:
                        if (
                            "approach" in conf
                            and conf["approach"] == entry["approach"]
                            and conf["value"]["bounds"]["type"]
                            == entry["value"]["bounds"]["type"]
                        ):
                            entry["value"]["material"]["mapMaterial"] = conf["value"][
                                "material"
                            ]["mapMaterial"]
                            entry["value"]["material"]["mappingType"] = conf["value"][
                                "material"
                            ]["mappingType"]
                            ibin = 0
                            for bin in entry["value"]["material"]["binUtility"][
                                "binningdata"
                            ]:
                                bin["bins"] = conf["value"]["material"]["binUtility"][
                                    "binningdata"
                                ][ibin]["bins"]
                                ibin = ibin + 1
                            continue
                    continue

            if "sensitive" in entry:
                if "approach" not in entry:
                    for conf in config["Surfaces"][str(entry["volume"])]:
                        if (
                            "sensitive" in conf
                            and conf["sensitive"] == "X"
                            and conf["layer"] == entry["layer"]
                            and conf["value"]["bounds"]["type"]
                            == entry["value"]["bounds"]["type"]
                        ):
                            entry["value"]["material"]["mapMaterial"] = conf["value"][
                                "material"
                            ]["mapMaterial"]
                            entry["value"]["material"]["mappingType"] = conf["value"][
                                "material"
                            ]["mappingType"]
                            ibin = 0
                            for bin in entry["value"]["material"]["binUtility"][
                                "binningdata"
                            ]:
                                bin["bins"] = conf["value"]["material"]["binUtility"][
                                    "binningdata"
                                ][ibin]["bins"]
                                ibin = ibin + 1
                            continue
                    continue
        data["Volumes"] = config["Volumes"]
    json_file.seek(0)
    json.dump(data, json_file, indent=4)
    json_file.truncate()
