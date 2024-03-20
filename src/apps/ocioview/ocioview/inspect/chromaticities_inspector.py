# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import numpy as np
import pygfx as gfx
import PyOpenColorIO as ocio
from colour_visuals import *
from PySide6 import QtCore, QtGui, QtWidgets
from typing import Optional

from ..viewer import WgpuCanvasOffScreenViewer
from ..message_router import MessageRouter
from ..utils import get_glyph_icon, subsampling_factor


class ChromaticitiesInspector(QtWidgets.QWidget):
    @classmethod
    def label(cls) -> str:
        return "Chromaticities"

    @classmethod
    def icon(cls) -> QtGui.QIcon:
        return get_glyph_icon("mdi6.grain")

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._cpu_proc = None
        self._image_array = None

        self._wgpu_viewer = WgpuCanvasOffScreenViewer()
        self._root = None
        self._visuals = {}
        self._setup_scene()

        # Layout
        layout = QtWidgets.QHBoxLayout()
        self.setLayout(layout)
        layout.addWidget(self._wgpu_viewer)

        msg_router = MessageRouter.get_instance()
        msg_router.processor_ready.connect(self._on_processor_ready)
        msg_router.image_ready.connect(self._on_image_ready)

    @property
    def wgpu_viewer(self):
        return self._wgpu_viewer

    def _setup_scene(self):
        self._wgpu_viewer.wgpu_scene.add(
            gfx.Background(
                None, gfx.BackgroundMaterial(np.array([0.18, 0.18, 0.18]))
            )
        )
        self._visuals = {
            "grid": VisualGrid(size=2),
            "chromaticity_diagram": VisualChromaticityDiagramCIE1931(
                kwargs_visual_chromaticity_diagram={"opacity": 0.25}
            ),
            "rgb_scatter_3d": VisualRGBScatter3D(np.zeros(3), "ACES2065-1", size=4),
        }

        self._root = gfx.Group()
        for visual in self._visuals.values():
            self._root.add(visual)
        self._wgpu_viewer.wgpu_scene.add(self._root)

    def reset(self) -> None:
        pass

    def showEvent(self, event: QtGui.QShowEvent) -> None:
        """Start listening for processor updates, if visible."""
        super().showEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_processor_updates_allowed(True)
        msg_router.set_image_updates_allowed(True)

    def hideEvent(self, event: QtGui.QHideEvent) -> None:
        """Stop listening for processor updates, if not visible."""
        super().hideEvent(event)

        msg_router = MessageRouter.get_instance()
        msg_router.set_processor_updates_allowed(False)
        msg_router.set_image_updates_allowed(False)

    @QtCore.Slot(ocio.CPUProcessor)
    def _on_processor_ready(self, cpu_proc: ocio.CPUProcessor) -> None:
        print("ChromaticitiesInspector._on_processor_ready")

        self._cpu_proc = cpu_proc
        # group_transform = self._cpu_proc.createGroupTransform()
        print(dir(self._cpu_proc))

        image_array = np.copy(self._image_array)
        self._cpu_proc.applyRGB(image_array)
        self._visuals["rgb_scatter_3d"].RGB = image_array
        self._wgpu_viewer.render()

    @QtCore.Slot(np.ndarray)
    def _on_image_ready(self, image_array: np.ndarray) -> None:
        print("ChromaticitiesInspector._on_image_ready")

        sub_sampling_factor = int(
            np.sqrt(subsampling_factor(image_array, 1e6)))
        self._image_array = image_array[
                ::sub_sampling_factor, ::sub_sampling_factor, ...
            ]

        if self._cpu_proc is None:
            self._visuals["rgb_scatter_3d"].RGB = self._image_array
            self._wgpu_viewer.render()



if __name__ == "__main__":
    application = QtWidgets.QApplication([])
    chromaticity_inspector = ChromaticitiesInspector()
    chromaticity_inspector.resize(800, 600)
    chromaticity_inspector.show()

    application.exec()
