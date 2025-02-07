# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import matplotlib

matplotlib.use("QtAgg")

from PyQt5 import QtCore, QtWidgets, QtGui

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg, NavigationToolbar2QT
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

from PyQt5 import QtCore, QtWidgets


class MatplotlibWidget(QtWidgets.QWidget):
    """
    This widget is supposed to support a processor that renders its output to
    a matplotlib figure.
    For performance reasons, it only renders the content if the widget is_active.
    """

    def __init__(self, processor, parent=None):
        super(MatplotlibWidget, self).__init__()
        self.processor = processor
        self.fig, self.axes = processor.get_figure_axes()

        self.canvas = FigureCanvasQTAgg(self.fig)
        toolbar = NavigationToolbar2QT(self.canvas, self)

        self.layout = QtWidgets.QVBoxLayout()
        self.layout.addWidget(toolbar)
        self.layout.addWidget(self.canvas)
        self.setLayout(self.layout)

        self.is_active = False

    def change_step(self, step):
        try:
            for ax in self.axes.flatten():
                ax.cla()
        except:
            self.axes.cla()

        if self.is_active:
            self.processor.draw(self.fig, self.axes, step)
            self.canvas.draw()


class LogWidget(QtWidgets.QTextEdit):
    """
    This is a widget dedicated to rendering the log
    of a certain step.
    """

    def __init__(self, steps, parent=None):
        super(LogWidget, self).__init__()
        self.steps = steps

        font = QtGui.QFont()
        font.setFamily("monospace [Consolas]")
        font.setFixedPitch(True)
        font.setStyleHint(QtGui.QFont.TypeWriter)

        self.setFont(font)
        self.setReadOnly(True)
        self.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)

    def change_step(self, step):
        if step >= len(self.steps):
            self.setText(f"Error: cannot display log for step {step}")
        else:
            self.setText("".join(self.steps[step]))


class MainWindow(QtWidgets.QWidget):
    """
    This is the main window. It sets the gui up etc.
    Most importantly, it calls .step_changed() on the active tab-widget
    if the step-slider has changed
    """

    def __init__(self, processors, steps, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)
        self.setWindowTitle("GSF Debugger")

        layout = QtWidgets.QVBoxLayout()

        # Step label
        self.label = QtWidgets.QLabel("step: 0", self)
        layout.addWidget(self.label)

        # Slider
        self.slider = QtWidgets.QSlider(QtCore.Qt.Horizontal, self)
        self.slider.setMinimum(0)
        self.slider.setMaximum(len(steps))
        self.slider.valueChanged.connect(self.step_changed)

        def disable():
            for p in self.processor_widgets:
                if hasattr(p, "is_active"):
                    p.is_active = False

        self.slider.sliderPressed.connect(disable)

        def switch(i=None):
            if i is None:
                i = self.tabs.currentIndex()
            for p in self.processor_widgets:
                if hasattr(p, "is_active"):
                    p.is_active = False
            if hasattr(self.processor_widgets[i], "is_active"):
                self.processor_widgets[i].is_active = True
            self.step_changed()

        self.slider.sliderReleased.connect(switch)

        def bwd():
            self.slider.setValue(max(self.slider.value() - 1, 0))
            self.step_changed()

        bwdBtn = QtWidgets.QPushButton("-")
        bwdBtn.pressed.connect(bwd)

        def fwd():
            self.slider.setValue(min(self.slider.value() + 1, len(steps)))
            self.step_changed()

        fwdBtn = QtWidgets.QPushButton("+")
        fwdBtn.pressed.connect(fwd)

        hlayout = QtWidgets.QHBoxLayout()
        hlayout.addWidget(bwdBtn, 1)
        hlayout.addWidget(self.slider, 18)
        hlayout.addWidget(fwdBtn, 1)

        layout.addLayout(hlayout)

        # Tabs
        self.processor_widgets = []
        self.tabs = QtWidgets.QTabWidget(self)
        for p in processors:
            self.processor_widgets.append(MatplotlibWidget(p))
            self.tabs.addTab(self.processor_widgets[-1], p.name())

        self.processor_widgets.append(LogWidget(steps))
        self.tabs.addTab(self.processor_widgets[-1], "Log")
        self.tabs.currentChanged.connect(switch)

        layout.addWidget(self.tabs)

        # init
        switch(0)
        self.step_changed()

        # Finalize
        self.setLayout(layout)
        self.show()

    def step_changed(self):
        self.label.setText(f"step: {self.slider.value()}")
        for w in self.processor_widgets:
            w.change_step(self.slider.value())
