# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


def test_add_color_space(ocio, ocio_view):
    config = ocio.GetCurrentConfig()
    color_space = ocio.ColorSpace(name="test")
    assert config.getColorSpace("test") is None

    model = ocio_view.config_dock.color_space_edit.model
    color_space_names = model.get_item_names()
    assert "test" not in color_space_names

    color_space_count = len(config.getColorSpaces())
    assert len(color_space_names) == color_space_count

    model._add_item(color_space)

    assert config.getColorSpace("test") is not None
    assert model.get_item_names() == color_space_names + ["test"]
    assert len(config.getColorSpaces()) == color_space_count + 1
