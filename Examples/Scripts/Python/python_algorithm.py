#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import acts
import acts.examples


s = acts.examples.Sequencer(events=1000, numThreads=10, logLevel=acts.logging.VERBOSE)


class PyAlg(acts.examples.IAlgorithm):
    def __init__(self, name, level):
        acts.examples.IAlgorithm.__init__(self, name, level)

    # def name(self):
    #     return "PyAlg"

    def execute(self, context):
        return acts.examples.ProcessCode.SUCCESS


class PyAlg1(acts.examples.IAlgorithm):
    def name(self):
        return "PyAlg1"

    def execute(self, context):
        print(context.eventStore.exists("blubb"))
        return acts.examples.ProcessCode.SUCCESS


s.addAlgorithm(PyAlg(name="blubb", level=acts.logging.INFO))

print("alg go")
s.run()
