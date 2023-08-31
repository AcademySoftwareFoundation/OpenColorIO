# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import PyOpenColorIO as ocio


def ravel_transform(transform: ocio.Transform) -> list[ocio.Transform]:
    """
    Recursively ravel group transform into flattened list of
    transforms. Other transform types are returned as the sole list
    item.

    :param transform: Transform to ravel
    :return: list of transforms
    """
    transforms = []

    def ravel_recursive(tf):
        if isinstance(tf, ocio.GroupTransform):
            for child in tf:
                ravel_recursive(child)
        else:
            transforms.append(tf)

    if transform is not None:
        ravel_recursive(transform)

    return transforms
