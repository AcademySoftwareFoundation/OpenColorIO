# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import numpy as np
import pygfx as gfx
from PySide6 import QtCore, QtGui, QtWidgets
from wgpu.gui.offscreen import WgpuCanvas
from wgpu.gui.qt import BUTTON_MAP, MODIFIERS_MAP


class WgpuCanvasOffScreenViewer(QtWidgets.QGraphicsView):
    def __init__(self):
        super().__init__()

        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)

        # WebGPU
        self._wgpu_canvas = WgpuCanvas(size=self._viewport_size)
        self._wgpu_renderer = gfx.renderers.WgpuRenderer(self._wgpu_canvas)
        self._wgpu_camera = gfx.PerspectiveCamera(50, 16 / 9)
        self._wgpu_controller = gfx.OrbitController(self._wgpu_camera)
        self._wgpu_controller.register_events(self._wgpu_renderer)

        self._wgpu_scene = gfx.Scene()

        self._wgpu_canvas.request_draw(
            lambda: self._wgpu_renderer.render(
                self._wgpu_scene, self._wgpu_camera
            )
        )

        self._wgpu_camera.local.position = np.array([0.5, 0.5, 2])
        self._wgpu_camera.show_pos(np.array([0.5, 0.5, 0.5]))

        # QGraphicsView
        self.setScene(QtWidgets.QGraphicsScene(self))
        self.setTransformationAnchor(self.ViewportAnchor.AnchorUnderMouse)
        self.image_plane = QtWidgets.QGraphicsPixmapItem(
            self._render_to_pixmap()
        )
        self.scene().addItem(self.image_plane)
        self.scale(0.5, 0.5)

    @property
    def wgpu_canvas(self):
        return self._wgpu_canvas

    @property
    def wgpu_renderer(self):
        return self._wgpu_renderer

    @property
    def wgpu_camera(self):
        return self._wgpu_camera

    @property
    def wgpu_controller(self):
        return self._wgpu_controller

    @property
    def wgpu_scene(self):
        return self._wgpu_scene

    @property
    def _viewport_size(self):
        return (
            self.viewport().size().width() * 2,
            self.viewport().size().height() * 2,
        )

    def resizeEvent(self, event: QtGui.QResizeEvent) -> None:
        super().resizeEvent(event)

        self._wgpu_canvas.set_logical_size(*self._viewport_size)

        self.render()

    def _mouse_event(self, event_type, event, touches=True):
        button = BUTTON_MAP.get(event.button(), 0)
        buttons = [
            BUTTON_MAP[button]
            for button in BUTTON_MAP.keys()
            if button & event.buttons()
        ]

        modifiers = [
            MODIFIERS_MAP[mod]
            for mod in MODIFIERS_MAP.keys()
            if mod & event.modifiers()
        ]

        wgpu_event = {
            "event_type": event_type,
            "x": event.pos().x(),
            "y": event.pos().y(),
            "button": button,
            "buttons": buttons,
            "modifiers": modifiers,
        }
        if touches:
            wgpu_event.update(
                {
                    "ntouches": 0,
                    "touches": {},
                }
            )

        self._wgpu_canvas.handle_event(wgpu_event)

        self.render()

    def mousePressEvent(self, event):
        self._mouse_event("pointer_down", event)

    def mouseMoveEvent(self, event):
        self._mouse_event("pointer_move", event)

    def mouseReleaseEvent(self, event):
        self._mouse_event("pointer_up", event)

    def mouseDoubleClickEvent(self, event):
        self._mouse_event("double_click", event, touches=False)

    def wheelEvent(self, event):
        modifiers = [
            MODIFIERS_MAP[mod]
            for mod in MODIFIERS_MAP.keys()
            if mod & event.modifiers()
        ]

        wgpu_event = {
            "event_type": "wheel",
            "dx": -event.angleDelta().x(),
            "dy": -event.angleDelta().y(),
            "x": event.position().x(),
            "y": event.position().y(),
            "modifiers": modifiers,
        }

        self._wgpu_canvas.handle_event(wgpu_event)

        self.render()

    def render(self):
        self.image_plane.setPixmap(self._render_to_pixmap())

    def _render_to_pixmap(self):
        render = np.array(self._wgpu_renderer.target.draw())[..., :3]

        height, width, _channel = render.shape
        return QtGui.QPixmap.fromImage(
            QtGui.QImage(
                np.ascontiguousarray(render),
                width,
                height,
                3 * width,
                QtGui.QImage.Format_RGB888,
            )
        )
