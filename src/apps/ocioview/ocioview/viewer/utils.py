# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from pathlib import Path

import numpy as np

try:
    import OpenImageIO as oiio

    HAS_OIIO = True
except (ImportError, ModuleNotFoundError):
    HAS_OIIO = False
    oiio = None

if not HAS_OIIO:
    # NOTE: This will raise if not available, since one of the supported image
    #       libraries is required.
    import imageio as iio
else:
    iio = None


def load_image(image_path: Path) -> np.ndarray:
    """
    Load RGB image data via an available image library.

    :param image_path: Path to image to load
    :return: NumPy array
    """
    if HAS_OIIO:
        return _load_oiio(image_path)
    else:
        return _load_iio(image_path)


def _load_oiio(image_path: Path) -> np.ndarray:
    """
    Load RGB image data via OpenImageIO.

    :param image_path: Path to image to load
    :return: NumPy array
    """
    image_buf = oiio.ImageBuf(image_path.as_posix())
    spec = image_buf.spec()

    # Convert to RGB, filling missing color channels with 0.0
    if spec.nchannels < 3:
        image_buf = oiio.ImageBufAlgo.channels(
            image_buf,
            tuple(list(range(spec.nchannels)) + ([0.0] * (3 - spec.nchannels))),
            newchannelnames=("R", "G", "B"),
        )
    elif spec.nchannels > 3:
        image_buf = oiio.ImageBufAlgo.channels(
            image_buf, (0, 1, 2), newchannelnames=("R", "G", "B")
        )

    # Get pixels as 32-bit float NumPy array
    return image_buf.get_pixels(oiio.FLOAT)


def _load_iio(image_path: Path) -> np.ndarray:
    """
    Load RGB image data via imageio.

    :param image_path: Path to image to load
    :return: NumPy array
    """
    data = iio.imread(image_path.as_posix())

    # Convert to 32-bit float
    if not np.issubdtype(data.dtype, np.floating):
        data = data.astype(np.float32) / np.iinfo(data.dtype).max
    if data.dtype != np.float32:
        data = data.astype(np.float32)

    # Convert to RGB, filling missing color channels with 0.0
    nchannels = 1
    if len(data.shape) == 3:
        nchannels = data.shape[-1]

    while nchannels < 3:
        data = np.dstack((data, np.zeros(data.shape[:2])))
        nchannels += 1
    if nchannels > 3:
        data = data[..., :3]

    return data
