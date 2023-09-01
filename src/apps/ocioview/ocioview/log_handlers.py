# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import atexit
import logging
import sys
from logging.handlers import QueueHandler

import PyOpenColorIO as ocio

from .message_router import message_queue


# Queue handler
queue_handler = QueueHandler(message_queue)

# Route OCIO log through queue handler, but disconnect for a clean exit
ocio.SetLoggingFunction(message_queue.put_nowait)
atexit.register(lambda: ocio.SetLoggingFunction(None))


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


def set_logging_level(level: ocio.LoggingLevel) -> None:
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
