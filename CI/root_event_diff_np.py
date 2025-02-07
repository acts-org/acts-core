# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import sys
import argparse
import numpy as np
import uproot


parser = argparse.ArgumentParser()
parser.add_argument("a")
parser.add_argument("b")
parser.add_argument("--event-nr", default="event_nr")
parser.add_argument("--fail-fast", action="store_true")
args = parser.parse_args()

a_data = uproot.open(args.a)
b_data = uproot.open(args.b)
event_nr = args.event_nr
fail_fast = args.fail_fast

a_sort_index = np.argsort(a_data[event_nr].array(library="np"), kind="stable")
b_sort_index = np.argsort(b_data[event_nr].array(library="np"), kind="stable")

np.set_printoptions(linewidth=np.inf)


def cmp(a, b):
    if np.isscalar(a):
        if a != b:
            return False
    elif type(a) == np.ndarray:
        if not np.array_equal(a, b, equal_nan=True):
            return False
    else:
        for aa, bb in zip(a, b):
            if not cmp(aa, bb):
                return False
    return True


failed_events = set()
failed_keys = set()
for key in a_data.keys():
    if key == event_nr:
        continue

    a_vals = a_data[key].array(library="np")
    a_vals = a_vals[a_sort_index]

    b_vals = b_data[key].array(library="np")
    b_vals = b_vals[b_sort_index]

    for event, (a, b) in enumerate(zip(a_vals, b_vals)):
        if not cmp(a, b):
            print(f"event {event} failed for {key}")
            print(f"a {a}")
            print(f"b {b}")
            print()

            if fail_fast:
                sys.exit(1)
            failed_events.add(event)
            failed_keys.add(key)

if failed_events or failed_keys:
    print("summary")
    print("failed events: " + " ".join(map(str, sorted(failed_events))))
    print("failed keys: " + " ".join(failed_keys))
    sys.exit(1)

sys.exit(0)
