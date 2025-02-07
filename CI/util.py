# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import contextlib
import sys

try:
    from halo import Halo
except:
    Halo = None


@contextlib.contextmanager
def Spinner(text, persist=True, *args, **kwargs):
    stream = kwargs.get("stream", sys.stdout)
    if stream.isatty() and Halo is not None:
        spinner = Halo(text, *args, **kwargs)
        spinner.start()
        try:
            yield
            if persist:
                spinner.succeed()
        except:
            if persist:
                spinner.fail()
            raise
        finally:
            if not persist:
                spinner.stop()
    else:
        stream.write(text + "\n")
        yield
