# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import pytest


@pytest.fixture(scope="session")
def qapp(qapp):
    """
    pytest-qt qapp fixture override, with ocioview-specific setup
    steps.
    """
    from ocioview.setup import setup_app

    setup_app(qapp)
    return qapp


@pytest.fixture
def qtbot(qapp, qtbot):
    """
    pytest-qt qtbot fixture override, injecting the overridden qapp
    fixture before qtbot can initialize the default implementation.
    """
    return qtbot


@pytest.fixture(scope="session")
def ocio():
    import PyOpenColorIO as ocio

    return ocio


@pytest.fixture
def ocio_view(ocio, qtbot):
    from ocioview.main_window import OCIOView

    ocio_view = OCIOView(transient=True)
    ocio_view.show()
    qtbot.addWidget(ocio_view)

    return ocio_view


@pytest.fixture
def ocio_config(ocio):
    """
    .. note::
        This fixture should be used AFTER the `ocio_view` fixture,
        since `OCIOView` instantiation resets the current config,
        invalidating any existing references.
    """
    return ocio.GetCurrentConfig()
