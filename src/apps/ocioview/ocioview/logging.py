# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from __future__ import annotations

import logging
import re
import sys
from logging.handlers import QueueHandler
from typing import Optional
from queue import SimpleQueue, Empty

import PyOpenColorIO as ocio
from pygments import highlight
from pygments.lexers import GLShaderLexer, XmlLexer, YamlLexer
from pygments.formatters import HtmlFormatter
from PySide6 import QtCore, QtGui, QtWidgets


# Queue handler
log_queue = SimpleQueue()
queue_handler = QueueHandler(log_queue)


# stdout handler
class StdoutFilter(logging.Filter):
    def filter(self, record: logging.LogRecord) -> bool:
        if record.levelno != logging.ERROR:
            return True
        else:
            return False


stdout_handler = logging.StreamHandler(sys.stdout)
stdout_handler.setLevel(logging.DEBUG)
stdout_handler.addFilter(StdoutFilter())


# stderr handler
class StderrFilter(logging.Filter):
    def filter(self, record: logging.LogRecord) -> bool:
        if record.levelno == logging.ERROR:
            return True
        else:
            return False


stderr_handler = logging.StreamHandler(sys.stderr)
stderr_handler.setLevel(logging.ERROR)
stderr_handler.addFilter(StderrFilter())


# Configure application-wide logging
logging.root.name = "ocioview"
logging.addLevelName(logging.ERROR, "Error")
logging.addLevelName(logging.WARNING, "Warning")
logging.addLevelName(logging.INFO, "Info")
logging.addLevelName(logging.DEBUG, "Debug")

logging.basicConfig(
    level=logging.DEBUG,
    handlers=[stdout_handler, stderr_handler, queue_handler],
    format="[%(name)s %(levelname)s]: %(message)s",
    force=True,
)


class LogRouter(QtCore.QThread):
    """
    Thread for routing OCIO and Python log records to listeners.
    """

    info_logged = QtCore.Signal(str)
    warning_logged = QtCore.Signal(str)
    error_logged = QtCore.Signal(str)
    debug_logged = QtCore.Signal(str)
    config_logged = QtCore.Signal(str)
    ctf_logged = QtCore.Signal(str)
    shader_logged = QtCore.Signal(str)

    RE_LOG_LEVEL = re.compile(r"\s*\[\w+ (?P<level>[a-zA-Z]+)]:")

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

    LEVEL_ERROR = "error"
    LEVEL_WARNING = "warning"
    LEVEL_INFO = "info"
    LEVEL_DEBUG = "debug"

    __instance: LogRouter = None

    @classmethod
    def get_instance(cls) -> LogRouter:
        """Get singleton log agent instance."""
        if cls.__instance is None:
            cls.__instance = LogRouter()
        return cls.__instance

    @classmethod
    def setup_ocio_logging(cls) -> None:
        """Route OCIO logging through queue handler."""
        ocio.SetLoggingFunction(log_queue.put_nowait)

    @classmethod
    def teardown_ocio_logging(cls) -> None:
        """
        Remove reference to log queue or Python will hang on close.
        """
        ocio.SetLoggingFunction(None)

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

        self._stopped = True

        # Make sure thread stops and logging is cleaned up on app close
        app = QtWidgets.QApplication.instance()
        app.aboutToQuit.connect(self.stop)

    def stop(self) -> None:
        """Stop logging thread."""
        if self._stopped:
            return

        self._stopped = True

        # Wait longer than the log loop interval for thread to stop
        self.wait(1000)

        # Clean up OCIO logging
        self.teardown_ocio_logging()

    def run(self) -> None:
        """Logging thread loop implementation."""
        self._stopped = False

        while not self._stopped:
            try:
                record = log_queue.get(timeout=0.25)
            except Empty:
                continue

            # OCIO config record
            if isinstance(record, ocio.Config):
                logged, level, msg = self._handle_config_record(record)
                if logged:
                    continue

            # OCIO processor record
            elif isinstance(record, ocio.Processor):
                logged, level, msg = self._handle_processor_record(record)
                if logged:
                    continue

            # Python log record
            elif isinstance(record, logging.LogRecord):
                level = record.levelname.lower()
                msg = record.msg

            # OCIO log record
            else:
                level, msg = self._handle_ocio_record(record)

            # HTML conversion
            html_msg = msg.rstrip().replace(" ", "&nbsp;").replace("\n", "<br>")

            if level == self.LEVEL_ERROR:
                self.error_logged.emit(self.FMT_ERROR.format(html=html_msg))
            elif level == self.LEVEL_WARNING:
                self.warning_logged.emit(self.FMT_WARNING.format(html=html_msg))
            elif level == self.LEVEL_INFO:
                self.info_logged.emit(self.FMT_LOG.format(html=html_msg))
            elif level == self.LEVEL_DEBUG:
                self.debug_logged.emit(self.FMT_LOG.format(html=html_msg))

    def _handle_ocio_record(self, record: str) -> tuple[str, str]:
        """
        Handle logging an OCIO log record received in the logging
        queue.

        :record: String log record
        :return: Logging level and message
        """
        level = self.LEVEL_INFO
        msg = str(record)

        record_match = self.RE_LOG_LEVEL.match(msg)
        if record_match:
            level = record_match.group("level").lower()

        # Route non-debug OCIO messages to stdout/stderr also
        if level == self.LEVEL_ERROR:
            sys.stderr.write(msg)
        elif level in (self.LEVEL_WARNING, self.LEVEL_INFO):
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
            yaml_data = str(config)
            yaml_html_data = self._increase_html_lineno_padding(
                highlight(yaml_data, YamlLexer(), HtmlFormatter(linenos="inline"))
            )
            self.config_logged.emit(yaml_html_data)

            # End logging
            return True, "", ""

        except Exception as e:
            # Pass error to main log
            return False, self.LEVEL_WARNING, str(e)

    def _handle_processor_record(
        self, processor: ocio.Processor
    ) -> tuple[bool, str, str]:
        """
        Handle logging an OCIO processor received in the logging queue.

        :config: OCIO processor instance
        :return: Whether record was already logged, and if not, logging
            level and message to pass through to the main log.
        """
        config = ocio.GetCurrentConfig()

        try:
            # Build and broadcast CTF
            group_tf = processor.createGroupTransform()
            ctf_data = group_tf.write("Color Transform Format", config)
            ctf_html_data = self._increase_html_lineno_padding(
                highlight(ctf_data, XmlLexer(), HtmlFormatter(linenos="inline"))
            )
            self.ctf_logged.emit(ctf_html_data)

            # Build and broadcast shader
            gpu_shader_desc = ocio.GpuShaderDesc.CreateShaderDesc(
                language=ocio.GPU_LANGUAGE_GLSL_4_0
            )
            gpu_proc = processor.getDefaultGPUProcessor()
            gpu_proc.extractGpuShaderInfo(gpu_shader_desc)
            glsl_data = gpu_shader_desc.getShaderText()
            glsl_html_data = self._increase_html_lineno_padding(
                highlight(glsl_data, GLShaderLexer(), HtmlFormatter(linenos="inline"))
            )
            self.shader_logged.emit(glsl_html_data)

            # End logging
            return True, "", ""

        except Exception as e:
            # Pass error to main log
            return False, self.LEVEL_WARNING, str(e)

    def _increase_html_lineno_padding(self, html: str) -> str:
        """
        Adds two non-breaking spaces to the right of all line numbers
        for some breathing room around the code.
        """
        # This works with inline and table linenos
        return re.sub(
            r"(<span class=\"(?:normal|special|linenos)\">\s*)([0-9]+)(</span>)",
            r"\1\2&nbsp;&nbsp;\3",
            html,
        )


# Start putting OCIO log record into queue before the router's thread begins consuming
# them. This results in OCIO initialization logs being captured.
LogRouter.setup_ocio_logging()
