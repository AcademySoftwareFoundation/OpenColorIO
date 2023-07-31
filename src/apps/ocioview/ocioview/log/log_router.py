# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

import logging
import re
import sys
from typing import Any, Optional
from queue import Empty

import PyOpenColorIO as ocio
from PySide2 import QtCore, QtGui, QtWidgets

from ..log_handlers import log_queue
from .utils import config_to_html, processor_to_ctf_html, processor_to_shader_html


class LogRunner(QtCore.QObject):
    """
    Object for routing OCIO and Python log records to listeners from a
    background thread.
    """

    info_logged = QtCore.Signal(str)
    warning_logged = QtCore.Signal(str)
    error_logged = QtCore.Signal(str)
    debug_logged = QtCore.Signal(str)
    config_logged = QtCore.Signal(str)
    ctf_logged = QtCore.Signal(str, ocio.GroupTransform)
    shader_logged = QtCore.Signal(str, ocio.GPUProcessor)

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

        self._prev_config_record = None
        self._prev_proc_record = None

        self._config_updates_allowed = False
        self._ctf_updates_allowed = False
        self._shader_updates_allowed = False

    def get_gpu_language(self) -> ocio.GpuLanguage:
        return self._gpu_language

    def set_gpu_language(self, gpu_language: ocio.GpuLanguage) -> None:
        self._gpu_language = gpu_language
        if self._shader_updates_allowed and self._prev_proc_record is not None:
            # Rebroadcast last processor record
            log_queue.put_nowait(self._prev_proc_record)

    def config_updates_allowed(self) -> bool:
        return self._config_updates_allowed

    def set_config_updates_allowed(self, allowed: bool) -> None:
        self._config_updates_allowed = allowed
        if allowed and self._prev_config_record is not None:
            # Rebroadcast last config record
            log_queue.put_nowait(self._prev_config_record)

    def ctf_updates_allowed(self) -> bool:
        return self._ctf_updates_allowed

    def set_ctf_updates_allowed(self, allowed: bool) -> None:
        self._ctf_updates_allowed = allowed
        if allowed and self._prev_proc_record is not None:
            # Rebroadcast last processor record
            log_queue.put_nowait(self._prev_proc_record)

    def shader_updates_allowed(self) -> bool:
        return self._shader_updates_allowed

    def set_shader_updates_allowed(self, allowed: bool) -> None:
        self._shader_updates_allowed = allowed
        if allowed and self._prev_proc_record is not None:
            # Rebroadcast last processor record
            log_queue.put_nowait(self._prev_proc_record)

    def is_routing(self) -> bool:
        """Whether runner is routing log records."""
        return self._is_routing

    def end_routing(self) -> None:
        """Instruct runner to ext routing loop ASAP."""
        self._end_routing = True

    def start_routing(self) -> None:
        """Instruct runner to start routing log records."""
        self._end_routing = False

        while not self._end_routing:
            self._is_routing = True

            try:
                record = log_queue.get(timeout=self.LOOP_INTERVAL)
            except Empty:
                continue

            # OCIO config record
            if isinstance(record, ocio.Config):
                self._prev_config_record = record
                if self._config_updates_allowed:
                    logged, level, msg = self._handle_config_record(record)
                    if logged:
                        continue
                else:
                    # Skip record
                    continue

            # OCIO processor record
            elif isinstance(record, ocio.Processor):
                self._prev_proc_record = record
                if self._ctf_updates_allowed or self._shader_updates_allowed:
                    logged, level, msg = self._handle_processor_record(record)
                    if logged:
                        continue
                else:
                    # Skip record
                    continue

            # Python log record
            if isinstance(record, logging.LogRecord):
                level = record.levelname.lower()
                msg = record.msg

            # OCIO log record
            else:
                level, msg = self._handle_ocio_record(record)

            # HTML conversion
            html_msg = msg.rstrip().replace(" ", "&nbsp;").replace("\n", "<br>")

            if level == self.LOG_LEVEL_ERROR:
                self.error_logged.emit(self.FMT_ERROR.format(html=html_msg))
            elif level == self.LOG_LEVEL_WARNING:
                self.warning_logged.emit(self.FMT_WARNING.format(html=html_msg))
            elif level == self.LOG_LEVEL_INFO:
                self.info_logged.emit(self.FMT_LOG.format(html=html_msg))
            elif level == self.LOG_LEVEL_DEBUG:
                self.debug_logged.emit(self.FMT_LOG.format(html=html_msg))

        self._is_routing = False

    def _handle_ocio_record(self, record: str) -> tuple[str, str]:
        """
        Handle logging an OCIO log record received in the logging
        queue.

        :record: String log record
        :return: Logging level and message
        """
        level = self.LOG_LEVEL_INFO
        msg = str(record)

        record_match = self.RE_LOG_LEVEL.match(msg)
        if record_match:
            level = record_match.group("level").lower()

        # Route non-debug OCIO messages to stdout/stderr also
        if level == self.LOG_LEVEL_ERROR:
            sys.stderr.write(msg)
        elif level in (self.LOG_LEVEL_WARNING, self.LOG_LEVEL_INFO):
            sys.stdout.write(msg)

        return level, msg

    def _handle_config_record(self, config: ocio.Config) -> tuple[bool, str, str]:
        """
        Handle logging an OCIO config received in the logging queue.

        :config: OCIO config instance
        :return: Whether record was already logged, and if not, logging
            level and message to pass through to the main log.
        """
        try:
            config_html_data = config_to_html(config)
            self.config_logged.emit(config_html_data)

            # End logging
            return True, "", ""

        except Exception as e:
            # Pass error to main log
            return False, self.LOG_LEVEL_WARNING, str(e)

    def _handle_processor_record(
        self, processor: ocio.Processor
    ) -> tuple[bool, str, str]:
        """
        Handle logging an OCIO processor received in the logging queue.

        :config: OCIO processor instance
        :return: Whether record was already logged, and if not, logging
            level and message to pass through to the main log.
        """
        try:
            if self._ctf_updates_allowed:
                ctf_html_data, group_tf = processor_to_ctf_html(processor)
                self.ctf_logged.emit(ctf_html_data, group_tf)

            if self._shader_updates_allowed:
                gpu_proc = processor.getDefaultGPUProcessor()
                shader_html_data = processor_to_shader_html(
                    gpu_proc, self._gpu_language
                )
                self.shader_logged.emit(shader_html_data, gpu_proc)

            # End logging
            return True, "", ""

        except Exception as e:
            # Pass error to main log
            return False, self.LOG_LEVEL_WARNING, str(e)


class LogRouter(QtCore.QObject):
    """
    Singleton router which runs a background thread for routing OCIO
    and Python log records to listeners.
    """

    __instance: LogRouter = None

    @classmethod
    def get_instance(cls) -> LogRouter:
        """Get singleton log agent instance."""
        if cls.__instance is None:
            cls.__instance = LogRouter()
        return cls.__instance

    @classmethod
    def set_logging_level(cls, level: ocio.LoggingLevel) -> None:
        """
        Change the OCIO and Python logging level.

        :param level: OCIO logging level
        """
        ocio.SetLoggingLevel(level)

        if level == ocio.LOGGING_LEVEL_WARNING:
            logging.root.setLevel(logging.WARNING)
        elif level == ocio.LOGGING_LEVEL_INFO:
            logging.root.setLevel(logging.INFO)
        elif level == ocio.LOGGING_LEVEL_DEBUG:
            logging.root.setLevel(logging.DEBUG)

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
        self._runner = LogRunner()
        self._runner.moveToThread(self._thread)
        self._thread.started.connect(self._runner.start_routing)

        # Make sure thread stops and logging is cleaned up on app close
        app = QtWidgets.QApplication.instance()
        app.aboutToQuit.connect(self.end_routing)

    def __getattr__(self, item: str) -> Any:
        """Forward unknown attribute requests to internal runner."""
        return getattr(self._runner, item)

    def end_routing(self) -> None:
        """Stop log record routing thread."""
        if not self._runner.is_routing():
            return

        self._runner.end_routing()
        self._thread.quit()

        # Wait twice as long as the routing loop interval for thread to stop. If
        # quitting the thread takes longer than this, the app will exit with a non-zero
        # exit code.
        self._thread.wait(int(LogRunner.LOOP_INTERVAL * 1000))

    def start_routing(self) -> None:
        """Start log record routing thread."""
        if self._runner.is_routing():
            return

        self._thread.start()
