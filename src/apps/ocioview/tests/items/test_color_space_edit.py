# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


def test_add_and_remove_color_space(ocio_view, ocio_config):
    color_space_count = len(ocio_config.getColorSpaces())
    assert ocio_config.getColorSpace("ColorSpace_1") is None

    edit = ocio_view.config_dock.color_space_edit
    edit.list.add_button.click()

    assert ocio_config.getColorSpace("ColorSpace_1") is not None
    assert len(ocio_config.getColorSpaces()) == color_space_count + 1

    edit.list.remove_button.click()

    assert ocio_config.getColorSpace("ColorSpace_1") is None
    assert len(ocio_config.getColorSpaces()) == color_space_count


def test_rename_color_space(ocio_view, ocio_config):
    edit = ocio_view.config_dock.color_space_edit
    edit.list.add_button.click()
    color_space_count = len(ocio_config.getColorSpaces())

    assert ocio_config.getColorSpace("ColorSpace_1") is not None
    assert ocio_config.getColorSpace("test") is None

    edit.param_edit.name_edit.set_value("test")
    edit.mapper.submit()

    assert ocio_config.getColorSpace("ColorSpace_1") is None
    assert ocio_config.getColorSpace("test") is not None
    assert len(ocio_config.getColorSpaces()) == color_space_count


def test_edit_color_space_reference_space_type(ocio, ocio_view, ocio_config):
    edit = ocio_view.config_dock.color_space_edit
    edit.list.add_button.click()

    assert (
        ocio_config.getColorSpace("ColorSpace_1").getReferenceSpaceType()
        == ocio.REFERENCE_SPACE_SCENE
    )

    edit.param_edit.reference_space_type_combo.set_member(
        ocio.REFERENCE_SPACE_DISPLAY
    )
    edit.mapper.submit()

    assert (
        ocio_config.getColorSpace("ColorSpace_1").getReferenceSpaceType()
        == ocio.REFERENCE_SPACE_DISPLAY
    )
