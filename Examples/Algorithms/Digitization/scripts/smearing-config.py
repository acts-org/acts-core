# This file is part of the ACTS project.
#
# Copyright (C) 2016 CERN for the benefit of the ACTS project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.


# each volume configuration is one logical block
#
#   --digi-smear-volume=8
#   --digi-smear-indices=0:1:5 # loc0, loc1, and time
#   --digi-smear-types=0:0:3   # loc{0,1} uses gaussian, time uses uniform
#   # parameter 0: loc0 gaussian width
#   # parameter 1: loc1 gaussian width
#   # parameter 2-4: time pitch,min,max
#   --digi-smear-parameters=10:20:2.5:-25:25
#
# which can be repeated as often as needed
#
#   --digi-smear-volume=11
#   --digi-smear-indices=1       # loc1
#   --digi-smear-types=0         # loc1 uses gaussian
#   --digi-smear-parameters=12.5 # loc1 gaussian width
#


import argparse
import json
import sys


def add_switch(i, argv, current):
    fields = argv[i].split("=")

    if len(fields) == 1:
        # --foo bar
        current.append(argv[i])
        current.append(argv[i + 1])
        i += 2

    elif len(fields) == 2:
        # --foo=bar
        current.append(argv[i])
        i += 1

    else:
        raise RuntimeError(f"Invalid argument: {argv[i]}")

    return i


def get_args_blocks():
    argv = sys.argv[1:]
    blocks = []
    current = []

    i = 0
    while i < len(argv):
        if argv[i].startswith("--digi-smear-volume"):
            if current:
                blocks.append(current)
                current = []
        i = add_switch(i, argv, current)
    if current:
        blocks.append(current)

    return blocks


def arg_parser():
    argp = argparse.ArgumentParser()
    argp.add_argument(
        "--digi-smear-volume", help="Sensitive volume identifiers", required=True
    )
    argp.add_argument(
        "--digi-smear-indices",
        help="Smear parameter indices for this volume",
        required=True,
    )
    argp.add_argument(
        "--digi-smear-types",
        help="Smear function types as 0 (gauss), 1 (truncated gauss), 2 (clipped gauss), 3 (uniform), 4 (digital)",
        required=True,
    )
    argp.add_argument(
        "--digi-smear-parameters",
        help="Smear parameters depending on the smearing type, 1 parameter for simple gauss, 3 for all others (1 parameter, 2 range values)",
        required=True,
    )
    return argp


def get_args():
    return [arg_parser().parse_args(block) for block in get_args_blocks()]


def get_n_params(type_id):
    if type_id == 0:
        return 1
    return 3


def get_param_blocks(types_ids, params):
    blocks = []
    icur = 0
    for x in types_ids:
        n = get_n_params(x)
        blocks.append(params[icur : icur + n])
        icur += n
    return blocks


def block_to_json(args):
    top_data = {"volume": int(args.digi_smear_volume), "value": {"smearing": []}}

    indices = [int(x) for x in args.digi_smear_indices.split(":")]
    types = [int(x) for x in args.digi_smear_types.split(":")]
    params = [float(x) for x in args.digi_smear_parameters.split(":")]
    param_blocks = get_param_blocks(types, params)

    for i, t, ps in zip(indices, types, param_blocks):
        data = {"index": i}
        if t == 0:
            data["mean"] = 0.0
            data["stddev"] = ps[0]
            data["type"] = "Gauss"
        elif t == 1:
            data["mean"] = 0.0
            data["stddev"] = ps[0]
            data["range"] = ps[1:]
            data["type"] = "GaussTrunc"
        elif t == 2:
            data["mean"] = 0.0
            data["stddev"] = ps[0]
            data["range"] = ps[1:]
            data["type"] = "GaussClipped"
        elif t in [3, 4]:
            data["type"] = "Uniform" if t == 3 else "Digitial"

            pitch = ps[0]
            low = ps[1]
            high = ps[2]

            data["bindata"] = [
                0,  # Acts::Open,
                0,  # Acts::BinningValue::binX,
                (high - low) / pitch,
                low,
                high,
            ]
        else:
            raise RuntimeError(f"Unrecognized type: {t}")

        top_data["value"]["smearing"].append(data)

    return top_data


def get_json_data():
    return {
        "acts-geometry-hierarchy-map": {
            "format-version": 0,
            "value-identifier": "digitization-configuration",
        },
        "entries": [block_to_json(x) for x in get_args()],
    }


def main():
    print(json.dumps(get_json_data(), indent=4))


if __name__ == "__main__":
    main()
