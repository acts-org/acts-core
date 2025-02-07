#!/usr/bin/env python

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# tgeo-response2json.py - convert TGeo response file options to ACTS v13.0.0 JSON format

import sys
import os
import re
import getopt
import json
import subprocess
from collections import OrderedDict


def usage():
    print(
        prog,
        """- convert TGeo response file options to ACTS v13.0.0 JSON format

USAGE:
  """
        + prog
        + """ [OPTIONS] tgeo.response

ACTS v13.0.0 (PR #884) changed the way the TGeo detector configuration is specified.
A JSON file is now used, instead of the previous method using Boost options,
which were often collected together in a response file (--response-file).
This script converts an old response file to the new JSON file.

To include all the required settings, this script needs to know the defaults for
any options not specified in the response file. These defaults can be obtained
by running a TGeo example with the --geo-tgeo-dump-jsonconfig=def.json option.
This script includes a hardcoded copy of these defaults (produced with ACTS v13.0.0).
These are used by default, but the latest defaults can be regenerated and used by
specifying the -d (or -c) option.

The JSON file is written to stdout.

OPTIONS:
  -h      display this help and exit
  -v      verbose running
  -d      use ActsExampleGeometryTGeo --geo-tgeo-dump-jsonconfig to get list of default options
  -c CMD  run CMD --geo-tgeo-dump-jsonconfig instead
  -f JSON read list of default options from JSON file
  -n      don't add any defaults

If none of -dcfn options is specified, then use hardcoded default options.

AUTHOR: Tim Adye <tim.adye@cern.ch>""",
    )


prog = os.path.basename(sys.argv[0])


def main():
    args = getopts()
    for filename in args:
        try:
            with open(filename) as f:
                process(f)
        except IOError as e:
            print(prog + ":", e, file=sys.stderr)


def getopts():
    global opt, verbose
    try:
        optlist, args = getopt.getopt(sys.argv[1:], "hvdc:f:n")
    except getopt.GetoptError as e:
        print(prog + ":", e, file=sys.stderr)
        exit(1)
    opt = dict(optlist)
    if "-h" in opt or len(sys.argv) <= 1:
        usage()
        sys.exit(0)
    verbose = "-v" in opt
    return args


def process(f):
    vols = []
    cfg = OrderedDict()
    vol = None
    iline = 0
    for line in f:
        iline += 1
        if verbose:
            print(str(iline) + ":" + line, end="", file=sys.stderr)
        line = re.sub(r"#.*", "", line).strip()
        if not line:
            continue

        if re.match(r"--geo-[\w-]+-loglevel\s\d+$", line):
            continue

        m = re.match(r"--(geo-tgeo-[\w-]+)\s+(.*)$", line)
        if not m:
            print(
                "%s:%d: unrecognised type of option: %s" % (f.name, iline, line),
                file=sys.stderr,
            )
            continue
        o, v = m.groups()

        if o == "geo-tgeo-filename" or o == "geo-tgeo-worldvolume":
            #      cfg[o] = v
            continue

        if o == "geo-tgeo-unit-scalor":
            cfg[o] = float(v)
            continue

        if o == "geo-tgeo-beampipe-parameters":
            cfg["geo-tgeo-build-beampipe"] = True
            cfg[o] = [float(x) for x in v.split(":")]
            continue

        if o == "geo-tgeo-volume":
            if vol is None:
                cfg["Volumes"] = vols
            vol = OrderedDict()
            vol["geo-tgeo-volume-name"] = v
            vols.append(vol)
            continue

        if vol is None:
            print(
                "%s:%d: unrecognised global option: %s" % (f.name, iline, line),
                file=sys.stderr,
            )
            continue

        if re.match("geo-tgeo-sfbin-(r|z|phi)-tolerance$", o):
            vv = [float(x) for x in v.split(":")]
            vol[o] = OrderedDict([("lower", vv[0]), ("upper", vv[1])])
            continue

        m = re.match("geo-tgeo-([ncp])(.*)$", o)
        if not m:
            print(
                "%s:%d: unrecognised option: %s" % (f.name, iline, line),
                file=sys.stderr,
            )
            continue

        side, oo = m.groups()
        side = {"n": "negative", "c": "central", "p": "positive"}[side]
        oo = "geo-tgeo-" + oo
        vv = v

        if oo == "geo-tgeo-layers":
            oo = "geo-tgeo-volume-layers"
            vv = bool(int(vv))

        elif oo == "geo-tgeo-volume-name":
            oo = "geo-tgeo-subvolume-names"

        elif oo == "geo-tgeo-module-name":
            oo = "geo-tgeo-sensitive-names"
            vv = vv.split("|")

        elif oo == "geo-tgeo-module-axes":
            oo = "geo-tgeo-sensitive-axes"

        elif re.match("geo-tgeo-layer-[rz]-split$", oo):
            vv = float(vv)

        elif re.match("geo-tgeo-layer-[rz]-range$", oo):
            oo += "s"
            vv = [float(x) for x in vv.split(":")]
            vv = OrderedDict([("lower", vv[0]), ("upper", vv[1])])

        else:
            print(
                "%s:%d: unrecognised option: %s" % (f.name, iline, line),
                file=sys.stderr,
            )
            continue

        if oo not in vol:
            vol[oo] = OrderedDict()
        vol[oo][side] = vv

    if "-n" not in opt:
        if "-d" in opt:
            empty = generate_empty_config("ActsExampleGeometryTGeo")
        elif "-c" in opt:
            empty = generate_empty_config(opt["-c"])
        elif "-f" in opt:
            with open(opt["-f"]) as ef:
                ecfg = ef.read()
            empty = json.loads(ecfg, object_pairs_hook=OrderedDict)
        else:
            empty = None

        if not empty:
            empty = empty_config()

        for o, v in empty.items():
            if o not in cfg:
                cfg[o] = v
        if len(empty.get("Volumes", [])):
            for vol in vols:
                for o, v in empty["Volumes"][0].items():
                    if o not in vol:
                        vol[o] = v

    print(json.dumps(cfg, indent=2))


def generate_empty_config(cmd):
    cmd += " --geo-tgeo-dump-jsonconfig=/dev/stdout"
    if verbose:
        print("+", cmd, file=sys.stderr)
    cfg = subprocess.check_output(cmd, shell=True)
    if not cfg:
        print(prog + ": command failed: " + cmd, file=sys.stderr)
        return None
    if verbose:
        print(cfg, file=sys.stderr)
    return json.loads(cfg, object_pairs_hook=OrderedDict)


def empty_config():
    return json.loads(
        """
{
  "geo-tgeo-unit-scalor": 1.0,
  "geo-tgeo-build-beampipe": false,
  "geo-tgeo-beampipe-parameters": [
    0.0,
    0.0,
    0.0
  ],
  "Volumes": [
    {
      "geo-tgeo-volume-name": "",
      "geo-tgeo-sfbin-r-tolerance": {
        "lower": 0.0,
        "upper": 0.0
      },
      "geo-tgeo-sfbin-z-tolerance": {
        "lower": 0.0,
        "upper": 0.0
      },
      "geo-tgeo-sfbin-phi-tolerance": {
        "lower": 0.0,
        "upper": 0.0
      },
      "geo-tgeo-volume-layers": {
        "negative": false,
        "central": false,
        "positive": false
      },
      "geo-tgeo-subvolume-names": {
        "negative": "",
        "central": "",
        "positive": ""
      },
      "geo-tgeo-sensitive-names": {
        "negative": [],
        "central": [],
        "positive": []
      },
      "geo-tgeo-sensitive-axes": {
        "negative": "",
        "central": "",
        "positive": ""
      },
      "geo-tgeo-layer-r-ranges": {
        "negative": {
          "lower": 0.0,
          "upper": 0.0
        },
        "central": {
          "lower": 0.0,
          "upper": 0.0
        },
        "positive": {
          "lower": 0.0,
          "upper": 0.0
        }
      },
      "geo-tgeo-layer-z-ranges": {
        "negative": {
          "lower": 0.0,
          "upper": 0.0
        },
        "central": {
          "lower": 0.0,
          "upper": 0.0
        },
        "positive": {
          "lower": 0.0,
          "upper": 0.0
        }
      },
      "geo-tgeo-layer-r-split": {
        "negative": 0.0,
        "central": 0.0,
        "positive": 0.0
      },
      "geo-tgeo-layer-z-split": {
        "negative": 0.0,
        "central": 0.0,
        "positive": 0.0
      },
      "geo-tgeo-cyl-disc-split": false,
      "Splitters": {
        "CylinderDisk": {
          "geo-tgeo-cyl-nphi-segs": 0,
          "geo-tgeo-cyl-nz-segs": 0,
          "geo-tgeo-disc-nphi-segs": 0,
          "geo-tgeo-disc-nr-segs": 0
        }
      }
    }
  ]
}
""",
        object_pairs_hook=OrderedDict,
    )


if __name__ == "__main__":
    sys.exit(main())
