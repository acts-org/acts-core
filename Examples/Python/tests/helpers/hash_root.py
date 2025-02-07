#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import hashlib
from pathlib import Path
import argparse

import uproot
import numpy as np
import awkward as ak


def hash_root_file(path: Path, ordering_invariant: bool = True) -> str:
    rf = uproot.open(path)

    gh = hashlib.sha256()

    for tree_name in sorted(rf.keys(cycle=False)):
        gh.update(tree_name.encode("utf8"))

        try:
            tree = rf[tree_name]
            if not isinstance(tree, uproot.TTree):
                continue
        except NotImplementedError:
            continue
        keys = list(sorted(tree.keys()))

        branches = tree.arrays(library="ak")

        if not ordering_invariant:
            h = hashlib.sha256()
            for name in keys:
                h.update(name.encode("utf8"))
                arr = branches[name]
                arr = ak.flatten(arr, axis=None)
                arr = np.array(arr)
                h.update(arr.tobytes())
            gh.update(h.digest())

        else:
            items = np.array([])

            for row in zip(*[branches[b] for b in keys]):
                h = hashlib.md5()
                for obj in row:
                    if isinstance(obj, ak.highlevel.Array):
                        if obj.ndim == 1:
                            h.update(ak.to_numpy(obj).tobytes())
                        else:
                            arr = ak.to_numpy(ak.flatten(obj, axis=None))
                            h.update(arr.tobytes())
                    else:
                        h.update(np.array([obj]).tobytes())
                items = np.append(items, h.digest())

            items.sort()

            h = hashlib.sha256()
            h.update("".join(keys).encode("utf8"))
            h.update(items.tobytes())

            gh.update(h.digest())
    return gh.hexdigest()


if "__main__" == __name__:
    p = argparse.ArgumentParser(
        description="Calculate a hash of the numeric content of a root file"
    )

    p.add_argument(
        "input_file", type=Path, help="The input ROOT file to calculate a hash for"
    )
    p.add_argument(
        "--no-ordering-invariant",
        "-n",
        action="store_true",
        help="Calculate a hash that is not invariant under reordering of entries? (faster than invariant)",
    )

    args = p.parse_args()

    print(
        hash_root_file(
            path=args.input_file,
            ordering_invariant=not args.no_ordering_invariant,
        )
    )
