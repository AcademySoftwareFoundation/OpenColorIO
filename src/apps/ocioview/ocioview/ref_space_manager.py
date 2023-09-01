# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from typing import Callable, Optional

import PyOpenColorIO as ocio


class ReferenceSpaceManager:
    """Interface for managing config reference spaces."""

    _ref_scene_name: Optional[str] = None
    _ref_display_name: Optional[str] = None
    _ref_subscribers: list[Callable] = []

    @classmethod
    def subscribe_to_reference_spaces(cls, ref_callback: Callable) -> None:
        """
        Subscribe to reference space updates.

        :param ref_callback: Reference space callback, which will be
            called when any reference space is created, notifying the
            application of an external config change.
        """
        if ref_callback not in cls._ref_subscribers:
            cls._ref_subscribers.append(ref_callback)

    @classmethod
    def scene_reference_space(cls) -> ocio.ColorSpace:
        """
        Return a color space from the current config which is
        representative of its scene reference space. If no such color
        space exists, one will be created.

        :return: Scene reference color space
        """
        cls._update_scene_reference_space()

        config = ocio.GetCurrentConfig()
        return config.getColorSpace(cls._ref_scene_name)

    @classmethod
    def display_reference_space(cls) -> ocio.ColorSpace:
        """
        Return a color space from the current config which is
        representative of its display reference space. If no such color
        space exists, one will be created.

        :return: Display reference color space
        """
        cls._update_display_reference_space()

        config = ocio.GetCurrentConfig()
        return config.getColorSpace(cls._ref_display_name)

    @classmethod
    def _update_scene_reference_space(cls) -> None:
        """
        Find or create a color space which is representative of the
        current config's scene reference space. This color space will
        have no transforms and not be a data space.
        """
        config = ocio.GetCurrentConfig()

        # Verify existing scene reference space
        if cls._ref_scene_name:
            scene_ref_color_space = config.getColorSpace(cls._ref_scene_name)
            if (
                not scene_ref_color_space
                or scene_ref_color_space.getReferenceSpaceType()
                != ocio.REFERENCE_SPACE_SCENE
                or scene_ref_color_space.isData()
                or scene_ref_color_space.getTransform(
                    ocio.COLORSPACE_DIR_FROM_REFERENCE
                )
                or scene_ref_color_space.getTransform(ocio.COLORSPACE_DIR_TO_REFERENCE)
            ):
                cls._ref_scene_name = None

        if not cls._ref_scene_name:
            # Find first candidate scene reference space
            for color_space in config.getColorSpaces(
                ocio.SEARCH_REFERENCE_SPACE_SCENE, ocio.COLORSPACE_ALL
            ):
                if (
                    not color_space.isData()
                    and not color_space.getTransform(ocio.COLORSPACE_DIR_FROM_REFERENCE)
                    and not color_space.getTransform(ocio.COLORSPACE_DIR_TO_REFERENCE)
                ):
                    cls._ref_scene_name = color_space.getName()
                    break

            # Make a scene reference space if not found
            if not cls._ref_scene_name:
                scene_ref_color_space = ocio.ColorSpace(
                    ocio.REFERENCE_SPACE_SCENE,
                    "scene_reference",
                    bitDepth=ocio.BIT_DEPTH_F32,
                    isData=False,
                )
                config.addColorSpace(scene_ref_color_space)
                cls._ref_scene_name = scene_ref_color_space.getName()

                # Notify subscribers
                for callback in cls._ref_subscribers:
                    callback()

    @classmethod
    def _update_display_reference_space(cls) -> None:
        """
        Find or create a color space which is representative of the
        current config's display reference space. This display color
        space will have no transforms and not be a data space.
        """
        config = ocio.GetCurrentConfig()

        # Verify existing display reference space
        if cls._ref_display_name:
            display_ref_color_space = config.getColorSpace(cls._ref_display_name)
            if (
                not display_ref_color_space
                or display_ref_color_space.getReferenceSpaceType()
                != ocio.REFERENCE_SPACE_DISPLAY
                or display_ref_color_space.isData()
                or display_ref_color_space.getTransform(
                    ocio.COLORSPACE_DIR_FROM_REFERENCE
                )
                or display_ref_color_space.getTransform(
                    ocio.COLORSPACE_DIR_TO_REFERENCE
                )
            ):
                cls._ref_display_name = None

        if not cls._ref_display_name:
            # Find first candidate display reference space
            for color_space in config.getColorSpaces(
                ocio.SEARCH_REFERENCE_SPACE_DISPLAY, ocio.COLORSPACE_ALL
            ):
                if (
                    not color_space.isData()
                    and not color_space.getTransform(ocio.COLORSPACE_DIR_FROM_REFERENCE)
                    and not color_space.getTransform(ocio.COLORSPACE_DIR_TO_REFERENCE)
                ):
                    cls._ref_display_name = color_space.getName()
                    break

            # Make a display reference space if not found
            if not cls._ref_display_name:
                display_ref_color_space = ocio.ColorSpace(
                    ocio.REFERENCE_SPACE_DISPLAY,
                    "display_reference",
                    bitDepth=ocio.BIT_DEPTH_F32,
                    isData=False,
                )
                config.addColorSpace(display_ref_color_space)
                cls._ref_display_name = display_ref_color_space.getName()

                # Notify subscribers
                for callback in cls._ref_subscribers:
                    callback()
