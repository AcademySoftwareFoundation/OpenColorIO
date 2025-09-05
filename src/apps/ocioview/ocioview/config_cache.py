# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import re
import uuid
import warnings
from typing import Callable, Optional, Union

import PyOpenColorIO as ocio


class ConfigCache:
    """
    Helper function result cache, tied to the current Config cache ID.
    """

    _cache_id: str = None
    _callbacks: list[Callable] = []

    _active_displays: Optional[list[str]] = None
    _active_views: Optional[list[str]] = None
    _all_names: Optional[list[str]] = None
    _categories: Optional[list[str]] = None
    _color_spaces: dict[
        tuple[bool, ocio.SearchReferenceSpaceType, ocio.ColorSpaceVisibility],
        Union[list[ocio.ColorSpace], ocio.ColorSpaceSet],
    ] = {}
    _color_space_names: dict[ocio.SearchReferenceSpaceType, list[str]] = {}
    _default_color_space_name: Optional[str] = None
    _default_view_transform_name: Optional[str] = None
    _displays: Optional[list[str]] = None
    _encodings: Optional[list[str]] = None
    _equality_groups: Optional[list[str]] = None
    _families: Optional[list[str]] = None
    _looks: Optional[list[ocio.Look]] = None
    _named_transforms: Optional[list[ocio.NamedTransform]] = None
    _shared_views: Optional[list[str]] = None
    _views: dict[
        tuple[Optional[str], Optional[str], Optional[ocio.ViewType]], list[str]
    ] = {}
    _view_transforms: Optional[list[ocio.ViewTransform]] = None
    _view_transform_names: Optional[list[str]] = None
    _viewing_rule_names: Optional[list[str]] = None

    @classmethod
    def get_cache_id(cls) -> tuple[str, bool]:
        """
        Try to get the cache ID for the current config. If the config
        is in an invalid state this will fail and a random config ID
        will be generated (which will be invalidated on the next config
        cache validation).

        :return: Tuple of cache ID, and whether the config is valid
        """
        config = ocio.GetCurrentConfig()

        try:
            return config.getCacheID(), True
        except Exception as e:
            # Invalid config state; generate random cache ID
            warnings.warn(str(e))
            return uuid.uuid4().hex, False

    @classmethod
    def register_reset_callback(cls, callback: Callable) -> None:
        """
        :param callback: Callable to call whenever the cache is
            cleared, which should reset external caches tied to this
            class' cache lifetime.
        """
        cls._callbacks.append(callback)

    @classmethod
    def validate(cls) -> bool:
        """
        Check cache validity, resetting all caches if the config has
        changed.

        :return: Whether cache is still valid. If False, the calling
            function should re-pull data from the current config and
            update the cache.
        """
        cache_id, is_valid = cls.get_cache_id()

        if cache_id != cls._cache_id:
            cls._active_displays = None
            cls._active_views = None
            cls._all_names = None
            cls._categories = None
            cls._color_spaces.clear()
            cls._color_space_names.clear()
            cls._default_color_space_name = None
            cls._default_view_transform_name = None
            cls._displays = None
            cls._encodings = None
            cls._equality_groups = None
            cls._families = None
            cls._looks = None
            cls._named_transforms = None
            cls._shared_views = None
            cls._views.clear()
            cls._view_transforms = None
            cls._view_transform_names = None
            cls._viewing_rule_names = None

            for callback in cls._callbacks:
                callback()

            cls._cache_id = cache_id
            return False

        return True

    @classmethod
    def get_active_displays(cls) -> list[str]:
        """
        :return: List of active displays from the current config
        """
        if not cls.validate() or cls._active_displays is None:
            cls._active_displays = list(ocio.GetCurrentConfig().getActiveDisplays())

        return cls._active_displays

    @classmethod
    def get_active_views(cls) -> list[str]:
        """
        :return: List of active views from the current config
        """
        if not cls.validate() or cls._active_views is None:
            cls._active_views = list(ocio.GetCurrentConfig().getActiveViews())

        return cls._active_views

    @classmethod
    def get_all_names(cls) -> list[str]:
        """
        :return: All unique names from the current config. When creating
            any new config object or adding aliases or roles, there should
            be no intersection with the returned names.
        """
        if not cls.validate() or cls._all_names is None:
            config = ocio.GetCurrentConfig()
            color_spaces = cls.get_color_spaces()
            named_transforms = cls.get_named_transforms()

            cls._all_names = (
                [c.getName() for c in color_spaces]
                + [a for c in color_spaces for a in c.getAliases()]
                + [t.getName() for t in named_transforms]
                + [a for t in named_transforms for a in t.getAliases()]
                + list(config.getLookNames())
                + list(config.getRoleNames())
                + cls.get_view_transform_names()
            )

        return cls._all_names

    @classmethod
    def get_builtin_color_space_roles(
        cls, include_deprecated: bool = False
    ) -> list[str]:
        """
        Get role names which are defined by the core OCIO library.

        :param include_deprecated: By default, deprecated roles are omitted
            from the returned role list. Set to True to return all builtin
            roles.
        :return: list of role names
        """
        roles = [
            ocio.ROLE_DATA,
            ocio.ROLE_DEFAULT,
            ocio.ROLE_COLOR_PICKING,
            ocio.ROLE_COLOR_TIMING,
            ocio.ROLE_COMPOSITING_LOG,
            ocio.ROLE_INTERCHANGE_DISPLAY,
            ocio.ROLE_INTERCHANGE_SCENE,
            ocio.ROLE_MATTE_PAINT,
            ocio.ROLE_RENDERING,
            ocio.ROLE_SCENE_LINEAR,
            ocio.ROLE_TEXTURE_PAINT,
        ]
        if include_deprecated:
            roles.extend([ocio.ROLE_REFERENCE])
        return roles

    @classmethod
    def get_categories(cls) -> list[str]:
        """
        :return: All color space/view transform/named transform categories
            from the current config.
        """
        if not cls.validate() or cls._categories is None:
            categories = set()

            for color_space in cls.get_color_spaces():
                categories.update(color_space.getCategories())
            for view_tf in cls.get_view_transforms():
                categories.update(view_tf.getCategories())
            for named_tf in cls.get_named_transforms():
                categories.update(named_tf.getCategories())

            cls._categories = sorted(filter(None, categories))

        return cls._categories

    @classmethod
    def get_color_spaces(
        cls,
        reference_space_type: Optional[ocio.SearchReferenceSpaceType] = None,
        visibility: Optional[ocio.ColorSpaceVisibility] = None,
        as_set: bool = False,
    ) -> Union[list[ocio.ColorSpace], ocio.ColorSpaceSet]:
        """
        Get all (all reference space types and visibility states) color
        spaces from the current config.

        :param reference_space_type: Optionally filter by reference
            space type.
        :param visibility: Optional filter by visibility
        :param as_set: If True, put returned color spaces into a
            ColorSpaceSet, which copies the spaces to insulate from config
            changes.
        :return: list or color space set of color spaces
        """
        if reference_space_type is None:
            reference_space_type = ocio.SEARCH_REFERENCE_SPACE_ALL
        if visibility is None:
            visibility = ocio.COLORSPACE_ALL

        cache_key = (as_set, reference_space_type, visibility)

        if not cls.validate() or cache_key not in cls._color_spaces:
            config = ocio.GetCurrentConfig()
            color_spaces = config.getColorSpaces(
                reference_space_type, visibility
            )
            if as_set:
                color_space_set = ocio.ColorSpaceSet()
                for color_space in color_spaces:
                    color_space_set.addColorSpace(color_space)
                cls._color_spaces[cache_key] = color_space_set
            else:
                cls._color_spaces[cache_key] = list(color_spaces)

        return cls._color_spaces[cache_key]

    @classmethod
    def get_color_space_names(
        cls,
        reference_space_type: ocio.SearchReferenceSpaceType = ocio.SEARCH_REFERENCE_SPACE_ALL,
    ) -> list[str]:
        """
        :param reference_space_type: Optional reference space search type
            to limit results. Searches all reference spaces by default.
        :return: Requested color space names from the current config
        """
        cache_key = reference_space_type

        if (
            not cls.validate()
            or reference_space_type not in cls._color_space_names
        ):
            cls._color_space_names[cache_key] = list(
                ocio.GetCurrentConfig().getColorSpaceNames(
                    reference_space_type, ocio.COLORSPACE_ALL
                )
            )

        return cls._color_space_names[cache_key]

    @classmethod
    def get_default_color_space_name(cls) -> Optional[str]:
        """
        Choose a reasonable default color space from the current config.

        :return: Color space name, or None if there are no color spaces
        """
        if not cls.validate() or cls._default_color_space_name is None:
            config = ocio.GetCurrentConfig()

            # Check common roles
            for role in (ocio.ROLE_DEFAULT, ocio.ROLE_SCENE_LINEAR):
                color_space = config.getColorSpace(role)
                if color_space is not None:
                    break

            if color_space is None:
                # Get first active color space
                for color_space in config.getColorSpaces():
                    break

            if color_space is None:
                # Get first color space, active or not
                for color_space in config.getColorSpaces(
                    ocio.SEARCH_REFERENCE_SPACE_ALL, ocio.COLORSPACE_ALL
                ):
                    break

            if color_space is not None:
                cls._default_color_space_name = color_space.getName()
            else:
                cls._default_color_space_name = None

        return cls._default_color_space_name

    @classmethod
    def get_default_view_transform_name(cls) -> Optional[str]:
        """
        :return: Default view transform name from the current config
        """
        if not cls.validate() or cls._default_view_transform_name is None:
            config = ocio.GetCurrentConfig()
            default_view_transform_name = config.getDefaultViewTransformName()

            if not default_view_transform_name:
                view_transform_names = cls.get_view_transform_names()
                if view_transform_names:
                    default_view_transform_name = view_transform_names[0]

            cls._default_view_transform_name = default_view_transform_name

        return cls._default_view_transform_name

    @classmethod
    def get_displays(cls) -> list[str]:
        """
        :return: Sorted list of OCIO displays from the current config
        """
        if not cls.validate() or cls._displays is None:
            cls._displays = list(ocio.GetCurrentConfig().getDisplaysAll())

        return cls._displays

    @classmethod
    def get_encodings(cls) -> list[str]:
        """
        :return: All color space/named transform encodings in current
            config.
        """
        if not cls.validate() or cls._encodings is None:
            # Pre-defined standard encodings from the OCIO docs
            encodings = {
                "scene-linear",
                "display-linear",
                "log",
                "sdr-video",
                "hdr-video",
                "data",
            }
            for color_space in cls.get_color_spaces():
                encodings.add(color_space.getEncoding())
            for named_tf in cls.get_named_transforms():
                encodings.add(named_tf.getEncoding())

            cls._encodings = sorted(filter(None, encodings))

        return cls._encodings

    @classmethod
    def get_equality_groups(cls) -> list[str]:
        """
        :return: All color space families in current config
        """
        if not cls.validate() or cls._equality_groups is None:
            equality_groups = set()

            for color_space in cls.get_color_spaces():
                equality_groups.add(color_space.getEqualityGroup())

            cls._equality_groups = sorted(filter(None, equality_groups))

        return cls._equality_groups

    @classmethod
    def get_families(cls) -> list[str]:
        """
        :return: All color space/view transform/named transform families
            from the current config.
        """
        if not cls.validate() or cls._families is None:
            families = set()

            for color_space in cls.get_color_spaces():
                families.add(color_space.getFamily())
            for view_tf in cls.get_view_transforms():
                families.add(view_tf.getFamily())
            for named_tf in cls.get_named_transforms():
                families.add(named_tf.getFamily())

            cls._families = sorted(filter(None, families))

        return cls._families

    @classmethod
    def get_looks(cls) -> list[ocio.Look]:
        """
        :return: All looks from the current config
        """
        if not cls.validate() or cls._looks is None:
            cls._looks = list(ocio.GetCurrentConfig().getLooks())

        return cls._looks

    @classmethod
    def get_named_transforms(cls) -> list[ocio.NamedTransform]:
        """
        :return: All named transforms from the current config
        """
        if not cls.validate() or cls._named_transforms is None:
            cls._named_transforms = list(
                ocio.GetCurrentConfig().getNamedTransforms(
                    ocio.NAMEDTRANSFORM_ALL
                )
            )

        return cls._named_transforms

    @classmethod
    def get_shared_views(cls) -> list[str]:
        """
        :return: All shared views for the current config
        """
        if not cls.validate() or cls._shared_views is None:
            cls._shared_views = list(ocio.GetCurrentConfig().getSharedViews())

        return cls._shared_views

    @classmethod
    def get_views(
        cls,
        display: Optional[str] = None,
        color_space_name: Optional[str] = None,
        view_type: Optional[ocio.ViewType] = None,
    ) -> list[str]:
        """
        :param display: OCIO display to get views for
        :param color_space_name: Contextual input color space name (for
            viewing rules evaluation)
        :param view_type: Optionally request ONLY shared views or
            display-defined views for the requested display(s). When unset,
            all view types are returned. Ignored when a color space name is
            provided.
        :return: Sorted list of matching OCIO views from the current config
        """
        cache_key = (display, color_space_name, view_type)

        if not cls.validate() or cache_key not in cls._views:
            config = ocio.GetCurrentConfig()

            if display is not None:
                if color_space_name is not None:
                    views = config.getViews(display, color_space_name)
                elif view_type is not None:
                    views = config.getViews(view_type, display)
                else:
                    # Ignore active views by getting all views from each view type
                    views = list(
                        config.getViews(ocio.VIEW_DISPLAY_DEFINED, display)
                    ) + list(config.getViews(ocio.VIEW_SHARED, display))
            else:
                # NOTE: Controlled recursion into this function
                views = set()
                for display in cls.get_displays():
                    views.update(
                        cls.get_views(
                            display,
                            color_space_name=color_space_name,
                            view_type=view_type,
                        )
                    )

            cls._views[cache_key] = list(views)

        return cls._views[cache_key]

    @classmethod
    def get_view_transforms(cls) -> list[ocio.ViewTransform]:
        """
        :return: List of view transforms from the current config
        """
        if not cls.validate() or cls._view_transforms is None:
            cls._view_transforms = list(
                ocio.GetCurrentConfig().getViewTransforms()
            )

        return cls._view_transforms

    @classmethod
    def get_view_transform_names(cls) -> list[str]:
        """
        :return: Sorted list of view transform names from the current
            config.
        """
        if not cls.validate() or cls._view_transform_names is None:
            cls._view_transform_names = sorted(
                ocio.GetCurrentConfig().getViewTransformNames()
            )

        return cls._view_transform_names

    @classmethod
    def get_viewing_rule_names(cls) -> list[str]:
        """
        :return: Sorted list of viewing rule names from the current config
        """
        if not cls.validate() or cls._viewing_rule_names is None:
            viewing_rules = ocio.GetCurrentConfig().getViewingRules()
            cls._viewing_rule_names = sorted(
                [
                    viewing_rules.getName(i)
                    for i in range(viewing_rules.getNumEntries())
                ]
            )

        return cls._viewing_rule_names
