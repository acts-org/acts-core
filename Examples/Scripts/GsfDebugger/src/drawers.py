# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import numpy as np
import pandas as pd

detector_color = "grey"


class ViewDrawer:
    """
    Base class for a drawer that encapsulates drawing the state with a certain view (x-y, z-r, ...)
    """

    def draw_detector(self, ax):
        ax = self.draw_detector_impl(ax)
        ax.set_title("{}-{} plot".format(self.coor0, self.coor1))
        ax.set_ylabel(self.coor1)
        ax.set_xlabel(self.coor0)
        return ax

    def plot(self, ax, values, title_appendix="", **kwargs):
        ax.plot(self.get_coor0(values), self.get_coor1(values), **kwargs)
        ax.set_title("{}-{} plot: {}".format(self.coor0, self.coor1, title_appendix))
        return ax

    def scatter(self, ax, values, title_appendix="", **kwargs):
        ax.scatter(self.get_coor0(values), self.get_coor1(values), **kwargs)
        return ax

    def annotate(self, ax, position_3d, text):
        l = np.array([position_3d])
        position_2d = (self.get_coor0(l), self.get_coor1(l))
        ax.annotate(text, position_2d)
        return ax

    def set_title(self, ax, title_appendix=""):
        ax.set_title("{}-{} plot: {}".format(self.coor0, self.coor1, title_appendix))
        return ax


class ZRDrawer(ViewDrawer):
    """
    Base class for a z-r drawer
    """

    def __init__(self):
        self.coor0 = "z"
        self.coor1 = "r"

    def get_coor0(self, values):
        return values[:, 2]

    def get_coor1(self, values):
        return np.hypot(values[:, 0], values[:, 1])


class XYDrawer(ViewDrawer):
    """
    Base class for a x-y drawer
    """

    def __init__(self):
        self.coor0 = "x"
        self.coor1 = "y"

    def get_coor0(self, values):
        return values[:, 0]

    def get_coor1(self, values):
        return values[:, 1]


class XZDrawer(ViewDrawer):
    """
    Base class for a x-z drawer
    """

    def __init__(self):
        self.coor0 = "x"
        self.coor1 = "z"

    def get_coor0(self, values):
        return values[:, 0]

    def get_coor1(self, values):
        return values[:, 2]


# Helper to unify code for drawers not involve radius
class CsvCartesianDrawer:
    def __init__(self, detector_csv, centers, bounds=None):
        super().__init__()
        d = pd.read_csv(detector_csv)
        d["cr"] = np.hypot(d["cx"], d["cy"])

        l = len(d)

        # millimeter precision should be enough
        for c in centers:
            d[c] = d[c].astype(int)

        d = d.drop_duplicates(centers).copy()
        print("INFO CsvCartesianDrawer", centers, "dup drop", l, "->", len(d))
        self.points = d[centers].to_numpy()

        # assumes x axis is telescope axis
        self.simple_telescope = bounds is not None
        if self.simple_telescope:
            self.bounds = d[bounds].to_numpy()

    def draw_detector_impl(self, ax):
        if not self.simple_telescope:
            ax.scatter(self.points[:, 0], self.points[:, 1], color=detector_color, s=1)
        else:
            for point, bound in zip(self.points, self.bounds):
                ax.plot(
                    [point[0], point[0]],
                    [point[1] + bound[0], point[1] + bound[1]],
                    color=detector_color,
                    zorder=0.5,
                )

        return ax


class CsvXYDrawer(XYDrawer):
    def __init__(self, detector_csv, assume_telescope=False):
        super().__init__()
        if assume_telescope:
            self.drawer = CsvCartesianDrawer(
                detector_csv, ["cx", "cy"], ["bound_param1", "bound_param3"]
            )
        else:
            self.drawer = CsvCartesianDrawer(detector_csv, ["cx", "cy"])

    def draw_detector_impl(self, ax):
        return self.drawer.draw_detector_impl(ax)


class CsvXZDrawer(XZDrawer):
    def __init__(self, detector_csv, assume_telescope=False):
        super().__init__()
        if assume_telescope:
            self.drawer = CsvCartesianDrawer(
                detector_csv, ["cx", "cz"], ["bound_param0", "bound_param2"]
            )
        else:
            self.drawer = CsvCartesianDrawer(detector_csv, ["cx", "cz"])

    def draw_detector_impl(self, ax):
        return self.drawer.draw_detector_impl(ax)


class CsvZRDrawer(ZRDrawer):
    def __init__(self, detector_csv):
        super().__init__()
        self.drawer = CsvCartesianDrawer(detector_csv, ["cz", "cr"])

    def draw_detector_impl(self, ax):
        return self.drawer.draw_detector_impl(ax)
