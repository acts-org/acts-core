#!/usr/bin/python3

import json
import sys
import os


def generate():
    format_file_version = 0

    # Assume charge smearing and threshold to be the same for the full detector
    charge_smearing = {"mean": 0.0, "stddev": 0.001, "type": "Gauss"}
    threshold = 0.001

    # Assemble some generic information for detector regions
    p_pitch = 0.05
    ss_pitchx = 0.075
    ss_pitchy = 0.5
    ls_pitchx = 0.15

    pixel_cfg = {
        "thickness": 0.125,
        "digital": False,
        "pitch": [p_pitch, p_pitch],
    }
    sstrip_cfg = {
        "thickness": 0.200,
        "digital": True,
        "pitch": [ss_pitchx, ss_pitchy],
    }
    lstrip_cfg = {
        "thickness": 0.250,
        "digital": True,
        "pitch": [
            ls_pitchx,
        ],
    }

    # Assemble all simplified configurations of the different geometry hierarchy entries
    cfgs = [
        {"desc": "pixel barrel", "vol": 17, "size": [16.8, 72.0], **pixel_cfg},
    ]
    for ecvol in [16, 18]:
        cfgs += [
            {
                "desc": "pixel endcap (ring 0)",
                "vol": ecvol,
                "extra": 1,
                "size": [32.2, 78.0],
                **pixel_cfg,
            },
            {
                "desc": "pixel endcap (ring 1)",
                "vol": ecvol,
                "extra": 2,
                "size": [16.8, 78.0],
                **pixel_cfg,
            },
        ]

    cfgs += [
        {"desc": "sstrip barrel", "vol": 24, "size": [48.0, 108.0], **sstrip_cfg},
    ]
    for ecvol in [23, 25]:
        cfgs += [
            {
                "desc": "sstrip endcap (ring 0)",
                "vol": ecvol,
                "extra": 1,
                "size": [32.2, 78.0],
                **sstrip_cfg,
            },
            {
                "desc": "sstrip endcap (ring 1)",
                "vol": ecvol,
                "extra": 2,
                "size": [44.0, 78.0],
                **sstrip_cfg,
            },
            {
                "desc": "sstrip endcap (ring 2)",
                "vol": ecvol,
                "extra": 3,
                "size": [56.4, 78.0],
                **sstrip_cfg,
            },
        ]

    cfgs += [
        {
            "desc": "sstrip barrel",
            "vol": 29,
            "size": [
                96.0,
            ],
            **lstrip_cfg,
        },
    ]
    for ecvol in [28, 30]:
        cfgs += [
            {
                "desc": "sstrip endcap (ring 0)",
                "vol": ecvol,
                "extra": 1,
                "size": [
                    58.6,
                ],
                **lstrip_cfg,
            },
            {
                "desc": "sstrip endcap (ring 1)",
                "vol": ecvol,
                "extra": 2,
                "size": [
                    70.0,
                ],
                **lstrip_cfg,
            },
        ]

    # Assemble the entries of the geometry hierarchy map
    entries = []

    for cfg in cfgs:
        binning_data = []
        for pitch, size, binDim in zip(cfg["pitch"], cfg["size"], ["binX", "binY"]):
            binning_data.append(
                {
                    "bins": int(size / pitch),
                    "max": 0.5 * size,
                    "min": -0.5 * size,
                    "option": "open",
                    "type": "equidistant",
                    "value": binDim,
                }
            )

        segmentation = {}
        segmentation["binningdata"] = binning_data

        geometric = {}
        geometric["digital"] = cfg["digital"]
        geometric["indices"] = list(range(len(cfg["pitch"])))
        geometric["segmentation"] = segmentation
        geometric["thickness"] = cfg["thickness"]
        geometric["threshold"] = threshold
        geometric["charge-smearing"] = charge_smearing

        value = {}
        value["geometric"] = geometric

        entry = {}
        entry["description"] = cfg["desc"]
        entry["volume"] = cfg["vol"]
        if "extra" in cfg:
            entry["extra"] = cfg["extra"]
        entry["value"] = value

        entries.append(entry)

    # Assemble and write the final configuration
    final_dict = {}
    final_dict["acts-geometry-hierarchy-map"] = {
        "format-version": 0,
        "value-identifier": "digitization-configuration",
    }
    final_dict["entries"] = entries

    return final_dict


if __name__ == "__main__":
    final_dict = generate()

    try:
        output_file = sys.argv[1]
    except:
        output_file = "odd-digi-geometric-config.json"

    with open(output_file, "w") as f:
        json.dump(final_dict, f, indent=4)
