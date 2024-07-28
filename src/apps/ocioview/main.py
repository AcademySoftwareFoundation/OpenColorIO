# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import sys

from ocioview.main_window import OCIOView
from ocioview.setup import setup_app


if __name__ == "__main__":
    app = setup_app()

    # Start ocioview
    ocio_view = OCIOView()
    ocio_view.show()

    sys.exit(app.exec_())
