from nose.tools import eq_ as eq


def _get_gamma2_conf():
    """Get boring config with lnf space and gamma2
    """

    import PyOpenColorIO as OCIO

    config = OCIO.Config()

    # lnf space
    cs = OCIO.ColorSpace(family='ln',name='lnf')
    cs.setDescription("linear light float")
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
    cs.setAllocationVars([-15.0,6.0])
    cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
    config.addColorSpace(cs)

    config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, "lnf")

    # gamma2 space
    cs = OCIO.ColorSpace(family='test',name='gamma2')
    cs.setDescription("simple gamma 2 space")
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
    cs.setTransform(OCIO.ExponentTransform([2,2,2,1.0]), OCIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)
    config.addColorSpace(cs)

    return config

def test_transform_rgb():
    """Create a config, transform a few pixels with it
    """

    config = _get_gamma2_conf()

    # Get processor
    processor = config.getProcessor("lnf", "gamma2")

    # Apply the color transform to the existing RGB pixel data
    img_in = [0, 0, 0,
              0.5, 0.5, 0.5,
              1.0, 1.0, 1.0]
    img_out = processor.applyRGB(img_in)

    # Check results
    expected = [0, 0, 0,
                0.5**2, 0.5**2, 0.5**2,
                1, 1, 1]

    eq(img_out, expected)


def test_transform_rgba():
    """Transform RGBA pixels
    """

    config = _get_gamma2_conf()

    # Get processor
    processor = config.getProcessor("lnf", "gamma2")

    # Apply the color transform to the existing RGB pixel data
    img_in = [0, 0, 0, 1,
              0.5, 0.5, 0.5, 1,
              1, 1, 1, 0.75]
    img_out = processor.applyRGBA(img_in)

    # Check results
    expected = [0, 0, 0, 1,
                0.5**2, 0.5**2, 0.5**2, 1,
                1, 1, 1, 0.75]

    eq(img_out, expected)


def test_transform_incorrect_number_pixels():
    """Can only transform n*3 or n*4 pixel values
    """

    config = _get_gamma2_conf()
    processor = config.getProcessor("lnf", "gamma2")

    # Ensure RGB/RGBA with single value fails
    try:
        processor.applyRGB([1])
    except TypeError:
        pass

    try:
        processor.applyRGBA([1])
    except TypeError:
        pass

    # Ensure RGB/RGBA with strange number of values
    try:
        processor.applyRGB([1, 1, 1, 1])
    except TypeError:
        pass

    try:
        processor.applyRGBA([1, 1, 1])
    except TypeError:
        pass


def test_transform_generator():
    """Ensure applyRGB works on a generator
    """

    def gen_pixels(num):
        for x in range(num):
            yield 1.0

    config = _get_gamma2_conf()
    processor = config.getProcessor("lnf", "gamma2")

    g = gen_pixels(6)
    img_out = processor.applyRGB(g)

    eq(img_out, [1, 1, 1,  1, 1, 1])
