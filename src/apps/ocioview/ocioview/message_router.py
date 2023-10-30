# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

import logging
import re
import sys
from typing import Any, Optional
from queue import Empty, SimpleQueue

import PyOpenColorIO as ocio
from PySide6 import QtCore, QtGui, QtWidgets

from .utils import config_to_html, processor_to_ctf_html, processor_to_shader_html


# Global message queue
message_queue = SimpleQueue()


class MessageRunner(QtCore.QObject):
    """
    Object for routing OCIO and Python messages to listeners from a
    background thread.
    """

    info_logged = QtCore.Signal(str)
    warning_logged = QtCore.Signal(str)
    error_logged = QtCore.Signal(str)
    debug_logged = QtCore.Signal(str)

    cpu_processor_ready = QtCore.Signal(ocio.CPUProcessor)
    config_html_ready = QtCore.Signal(str)
    ctf_html_ready = QtCore.Signal(str, ocio.GroupTransform)
    shader_html_ready = QtCore.Signal(str, ocio.GPUProcessor)

    LOOP_INTERVAL = 0.5  # In seconds

    FMT_LOG = f"<span>{{html}}</span>"  # Just triggers Qt HTML detection
    FMT_ERROR = (
        f'<span style="color:'
        f"{QtGui.QColor.fromHsvF(0.0, 0.5, 1.0).name()}"
        f'">{{html}}</span>'
    )
    FMT_WARNING = (
        f'<span style="color:'
        f"{QtGui.QColor.fromHsvF(30 / 360, 0.5, 1.0).name()}"
        f'">{{html}}</span>'
    )

    RE_LOG_LEVEL = re.compile(r"\s*\[\w+ (?P<level>[a-zA-Z]+)]:")

    LOG_LEVEL_ERROR = "error"
    LOG_LEVEL_WARNING = "warning"
    LOG_LEVEL_INFO = "info"
    LOG_LEVEL_DEBUG = "debug"

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        self._is_routing = False
        self._end_routing = True
        self._gpu_language = ocio.GPU_LANGUAGE_GLSL_4_0

        self._prev_config = None
        self._prev_proc = None

        self._cpu_processor_updates_allowed = False
        self._config_updates_allowed = False
        self._ctf_updates_allowed = False
        self._shader_updates_allowed = False

    def get_gpu_language(self) -> ocio.GpuLanguage:
        return self._gpu_language

    def set_gpu_language(self, gpu_language: ocio.GpuLanguage) -> None:
        self._gpu_language = gpu_language
        if self._shader_updates_allowed and self._prev_proc is not None:
            # Rebroadcast last processor record
            message_queue.put_nowait(self._prev_proc)

    def config_updates_allowed(self) -> bool:
        return self._config_updates_allowed

    def set_config_updates_allowed(self, allowed: bool) -> None:
        self._config_updates_allowed = allowed
        if allowed and self._prev_config is not None:
            # Rebroadcast last config record
            message_queue.put_nowait(self._prev_config)

    def cpu_processor_updates_allowed(self) -> bool:
        return self._cpu_processor_updates_allowed

    def set_cpu_processor_updates_allowed(self, allowed: bool) -> None:
        self._cpu_processor_updates_allowed = allowed
        if allowed and self._prev_config is not None:
            # Rebroadcast last config record
            message_queue.put_nowait(self._prev_config)

    def ctf_updates_allowed(self) -> bool:
        return self._ctf_updates_allowed

    def set_ctf_updates_allowed(self, allowed: bool) -> None:
        self._ctf_updates_allowed = allowed
        if allowed and self._prev_proc is not None:
            # Rebroadcast last processor record
            message_queue.put_nowait(self._prev_proc)

    def shader_updates_allowed(self) -> bool:
        return self._shader_updates_allowed

    def set_shader_updates_allowed(self, allowed: bool) -> None:
        self._shader_updates_allowed = allowed
        if allowed and self._prev_proc is not None:
            # Rebroadcast last processor record
            message_queue.put_nowait(self._prev_proc)

    def is_routing(self) -> bool:
        """Whether runner is routing messages."""
        return self._is_routing

    def end_routing(self) -> None:
        """Instruct runner to exit routing loop ASAP."""
        self._end_routing = True

    def start_routing(self) -> None:
        """Instruct runner to start routing messages."""
        self._end_routing = False

        while not self._end_routing:
            self._is_routing = True

            try:
                msg_raw = message_queue.get(timeout=self.LOOP_INTERVAL)
            except Empty:
                continue

            # OCIO config
            if isinstance(msg_raw, ocio.Config):
                self._prev_config = msg_raw
                if self._config_updates_allowed:
                    self._handle_config_message(msg_raw)

            # OCIO processor
            elif isinstance(msg_raw, ocio.Processor):
                self._prev_proc = msg_raw
                if (
                    self._cpu_processor_updates_allowed
                    or self._ctf_updates_allowed
                    or self._shader_updates_allowed
                ):
                    self._handle_processor_message(msg_raw)

            # Python or OCIO log record
            else:
                self._handle_log_message(msg_raw)

        self._is_routing = False

    def _handle_config_message(self, config: ocio.Config) -> None:
        """
        Handle OCIO config received in the message queue.

        :config: OCIO config instance
        """
        try:
            config_html_data = config_to_html(config)
            self.config_html_ready.emit(config_html_data)
        except Exception as e:
            # Pass error to log
            self._handle_log_message(str(e), force_level=self.LOG_LEVEL_WARNING)

    def _handle_processor_message(self, processor: ocio.Processor) -> None:
        """
        Handle OCIO processor received in the message queue.

        :config: OCIO processor instance
        """
        try:
            if self._cpu_processor_updates_allowed:
                self.cpu_processor_ready.emit(processor.getDefaultCPUProcessor())

            if self._ctf_updates_allowed:
                ctf_html_data, group_tf = processor_to_ctf_html(processor)
                self.ctf_html_ready.emit(ctf_html_data, group_tf)

            if self._shader_updates_allowed:
                gpu_proc = processor.getDefaultGPUProcessor()
                shader_html_data = processor_to_shader_html(
                    gpu_proc, self._gpu_language
                )
                self.shader_html_ready.emit(shader_html_data, gpu_proc)

        except Exception as e:
            # Pass error to log
            self._handle_log_message(str(e), force_level=self.LOG_LEVEL_WARNING)

    def _handle_log_message(
        self, log_record: str, force_level: Optional[str] = None
    ) -> None:
        """
        Handle routing a Python or OCIO log record received in the
        message queue.

        :param log_record: Log record data
        :param force_level: Force a particular log level for this
            record.
        """
        # Python log record
        if isinstance(log_record, logging.LogRecord):
            level = log_record.levelname.lower()
            msg = log_record.msg

        # OCIO log record?
        else:
            level = self.LOG_LEVEL_INFO
            msg = str(log_record)

            record_match = self.RE_LOG_LEVEL.match(log_record)
            if record_match:
                level = record_match.group("level").lower()

            # Route non-debug OCIO messages to stdout/stderr also
            if level == self.LOG_LEVEL_ERROR:
                sys.stderr.write(msg)
            elif level in (self.LOG_LEVEL_WARNING, self.LOG_LEVEL_INFO):
                sys.stdout.write(msg)

        # Override inferred level?
        if force_level is not None:
            level = force_level

        # HTML conversion
        html_msg = msg.rstrip().replace(" ", "&nbsp;").replace("\n", "<br>")

        # Python and OCIO log output
        if level == self.LOG_LEVEL_ERROR:
            self.error_logged.emit(self.FMT_ERROR.format(html=html_msg))
        elif level == self.LOG_LEVEL_WARNING:
            self.warning_logged.emit(self.FMT_WARNING.format(html=html_msg))
        elif level == self.LOG_LEVEL_INFO:
            self.info_logged.emit(self.FMT_LOG.format(html=html_msg))
        elif level == self.LOG_LEVEL_DEBUG:
            self.debug_logged.emit(self.FMT_LOG.format(html=html_msg))


class MessageRouter(QtCore.QObject):
    """
    Singleton router which runs a background thread for routing a
    variety of messages and log records to listeners.
    """

    __instance: MessageRouter = None

    @classmethod
    def get_instance(cls) -> MessageRouter:
        """Get singleton MessageRouter instance."""
        if cls.__instance is None:
            cls.__instance = MessageRouter()
        return cls.__instance

    def __init__(self, parent: Optional[QtCore.QObject] = None):
        super().__init__(parent=parent)

        # Only allow __init__ to be called once
        if self.__instance is not None:
            raise RuntimeError(
                f"{self.__class__.__name__} is a singleton. Please call "
                f"'get_instance' to access this type."
            )
        else:
            self.__instance = self

        # Setup threading
        self._thread = QtCore.QThread()
        self._runner = MessageRunner()
        self._runner.moveToThread(self._thread)
        self._thread.started.connect(self._runner.start_routing)

        # Make sure thread stops and routing is cleaned up on app close
        app = QtWidgets.QApplication.instance()
        app.aboutToQuit.connect(self.end_routing)

    def __getattr__(self, item: str) -> Any:
        """Forward unknown attribute requests to internal runner."""
        return getattr(self._runner, item)

    def end_routing(self) -> None:
        """Stop message routing thread."""
        if not self._runner.is_routing():
            return

        self._runner.end_routing()
        self._thread.quit()

        # Wait twice as long as the routing loop interval for thread to stop. If
        # quitting the thread takes longer than this, the app will exit with a non-zero
        # exit code.
        self._thread.wait(int(MessageRunner.LOOP_INTERVAL * 1000))

    def start_routing(self) -> None:
        """Start message routing thread."""
        if self._runner.is_routing():
            return

        self._thread.start()
