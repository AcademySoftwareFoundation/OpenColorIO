# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import enum
from typing import Optional

import PyOpenColorIO as ocio


class ViewType(str, enum.Enum):
    """Enum of view types."""

    VIEW_SHARED = "Shared View"
    VIEW_DISPLAY = "View (Display Reference Space)"
    VIEW_SCENE = "View (Scene Reference Space)"


def get_view_type(display: str, view: str) -> ViewType:
    """
    Get the view type from a display and view.

    :param display: Display name. An empty string indicates a shared
        display.
    :param view: View name
    :return: View type
    """
    if not display:
        return ViewType.VIEW_SHARED

    config = ocio.GetCurrentConfig()

    color_space_name = config.getDisplayViewColorSpaceName(display, view)
    view_transform_name = config.getDisplayViewTransformName(display, view)

    color_space = config.getColorSpace(color_space_name)
    if color_space is not None:
        if color_space.getReferenceSpaceType() == ocio.REFERENCE_SPACE_DISPLAY:
            return ViewType.VIEW_DISPLAY
        else:
            return ViewType.VIEW_SCENE
    elif view_transform_name:
        return ViewType.VIEW_DISPLAY
    else:
        return ViewType.VIEW_SCENE


def adapt_splitter_sizes(from_sizes: list[int], to_sizes: list[int]) -> list[int]:
    """
    Given source and destination splitter size lists, adapt the
    destination sizes to match the source sizes. Supports between two
    and three splitter sections.

    :param from_sizes: Sizes to adapt to
    :param to_sizes: Sizes to adjust
    :return: Adapted sizes to apply to destination
    """
    from_count = len(from_sizes)
    to_count = len(to_sizes)

    # Assumes 2-3 splitter sections
    if from_count == 3 and to_count == 2:
        return [from_sizes[0], sum(to_sizes) - from_sizes[0]]
    elif from_count == 2 and to_count == 3:
        return [
            from_sizes[0],
            to_sizes[1],
            sum(to_sizes) - from_sizes[0] - to_sizes[1],
        ]
    elif from_count == to_count:
        return from_sizes
    else:
        return to_sizes


def get_scene_to_display_transform(
    view_transform: ocio.ViewTransform,
) -> Optional[ocio.Transform]:
    """
    Extract a scene-to-display transform from a view transform
    instance.

    :param view_transform: View transform instance
    :return: Scene to display transform, if available
    """
    # REFERENCE_SPACE_SCENE
    if view_transform.getReferenceSpaceType() == ocio.REFERENCE_SPACE_SCENE:
        scene_to_display_ref_tf = view_transform.getTransform(
            ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
        )
        if not scene_to_display_ref_tf:
            display_to_scene_ref_tf = view_transform.getTransform(
                ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
            )
            if display_to_scene_ref_tf:
                scene_to_display_ref_tf = ocio.GroupTransform(
                    [display_to_scene_ref_tf], ocio.TRANSFORM_DIR_INVERSE
                )

    # REFERENCE_SPACE_DISPLAY
    else:
        scene_to_display_ref_tf = view_transform.getTransform(
            ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
        )
        if not scene_to_display_ref_tf:
            display_to_scene_ref_tf = view_transform.getTransform(
                ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
            )
            if display_to_scene_ref_tf:
                scene_to_display_ref_tf = ocio.GroupTransform(
                    [display_to_scene_ref_tf], ocio.TRANSFORM_DIR_INVERSE
                )

    # Has transform?
    if scene_to_display_ref_tf:
        return scene_to_display_ref_tf
    else:
        return None


def get_display_to_scene_transform(
    view_transform: ocio.ViewTransform,
) -> Optional[ocio.Transform]:
    """
    Extract a display-to-scene transform from a view transform
    instance.

    :param view_transform: View transform instance
    :return: Display to scene transform, if available
    """
    # REFERENCE_SPACE_DISPLAY
    if view_transform.getReferenceSpaceType() == ocio.REFERENCE_SPACE_DISPLAY:
        display_to_scene_ref_tf = view_transform.getTransform(
            ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
        )
        if not display_to_scene_ref_tf:
            scene_to_display_ref_tf = view_transform.getTransform(
                ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
            )
            if scene_to_display_ref_tf:
                display_to_scene_ref_tf = ocio.GroupTransform(
                    [scene_to_display_ref_tf], ocio.TRANSFORM_DIR_INVERSE
                )

    # REFERENCE_SPACE_SCENE
    else:
        display_to_scene_ref_tf = view_transform.getTransform(
            ocio.VIEWTRANSFORM_DIR_TO_REFERENCE
        )
        if not display_to_scene_ref_tf:
            scene_to_display_ref_tf = view_transform.getTransform(
                ocio.VIEWTRANSFORM_DIR_FROM_REFERENCE
            )
            if scene_to_display_ref_tf:
                display_to_scene_ref_tf = ocio.GroupTransform(
                    [scene_to_display_ref_tf], ocio.TRANSFORM_DIR_INVERSE
                )

    # Has transform?
    if display_to_scene_ref_tf:
        return display_to_scene_ref_tf
    else:
        return None
