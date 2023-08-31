# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import PyOpenColorIO as ocio

from .transform_edit import BaseTransformEdit
from .utils import ravel_transform


class TransformEditFactory:
    """
    Factory interface for registering and creating transform edits for
    implemented transform types.
    """

    _registry = {}

    @classmethod
    def transform_edit_type(cls, transform_type: type) -> type:
        """
        Get the transform edit type for the specified transform type.

        :param transform_type: Transform type
        :return: Transform edit type
        """
        if transform_type not in cls._registry:
            raise TypeError(f"Unsupported transform type: {transform_type.__name__}")

        return cls._registry[transform_type]

    @classmethod
    def from_transform_type(cls, transform_type: type) -> BaseTransformEdit:
        """
        Create a transform edit for the specified transform type.

        :param transform_type: Transform type
        :return: Transform edit widget
        """
        return cls.transform_edit_type(transform_type)()

    @classmethod
    def from_transform(cls, transform: ocio.Transform) -> BaseTransformEdit:
        """
        Create a transform edit from an existing transform instance.

        :param transform: Transform instance
        :return: Transform edit widget
        """
        tf_type = transform.__class__
        return cls.transform_edit_type(tf_type).from_transform(transform)

    @classmethod
    def from_transform_recursive(
        cls, transform: ocio.GroupTransform
    ) -> list[BaseTransformEdit]:
        """
        Recursively ravel group transform instance into flattened list
        of transform edits.

        :param transform: Group transform instance
        :return: list of transform edit widgets
        """
        return [cls.from_transform(tf) for tf in ravel_transform(transform)]

    @classmethod
    def transform_types(cls) -> list[type]:
        """
        list all registered transform types.

        :return: list of transform types
        """
        return sorted(cls._registry.keys(), key=lambda t: t.__name__)

    @classmethod
    def register(cls, tf_type: type, tf_edit_type: type) -> None:
        """
        All subclasses of BaseTransformEdit must be registered with
        this method.

        :param tf_type: ocio.Transform subclass
        :param tf_edit_type: TransformEdit type
        """
        if tf_type != ocio.Transform:
            # Store transform type on the edit class
            tf_edit_type.__tf_type__ = tf_type
            cls._registry[tf_type] = tf_edit_type
