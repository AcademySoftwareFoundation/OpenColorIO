..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _transforms:

.. _config-transforms:

Available transforms
********************

``AllocationTransform``
^^^^^^^^^^^^^^^^^^^^^^^

Transforms from reference space to the range specified by the
``vars:``

Keys:

* ``allocation``
* ``vars``
* ``direction``


``BuiltInTransform``
^^^^^^^^^^^^^^^^^^^^

Builds one of a known set of transforms, on demand

Keys:

* ``style``
* ``direction``


``CDLTransform``
^^^^^^^^^^^^^^^^

Applies an ASC CDL compliant grade

Keys:

* ``slope``
* ``offset``
* ``power``
* ``sat``
* ``style``
* ``name``
* ``direction``


``ColorSpaceTransform``
^^^^^^^^^^^^^^^^^^^^^^^

Transforms from ``src`` colorspace to ``dst`` colorspace.

Keys:

* ``src``
* ``dst``
* ``data_bypass``
* ``direction``


``DisplayViewTransform``
^^^^^^^^^^^^^^^^^^^^^^^^

Applies a View from one of the displays.

Keys:

* ``src``
* ``display``
* ``view``
* ``looks_bypass``
* ``data_bypass``
* ``direction``


``ExponentTransform``
^^^^^^^^^^^^^^^^^^^^^

Raises pixel values to a given power (often referred to as "gamma")

.. code-block:: yaml

    !<ExponentTransform> {value: [1.8, 1.8, 1.8, 1]}

Keys:

* ``value``
* ``style``
* ``name``
* ``direction``


``ExponentWithLinearTransform``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applies a power but with a linear section near black.  May be used to
implement sRGB, CIE L*, and the Rec.709 camera OETF (not the display!).


Keys:

* ``gamma``
* ``offset``
* ``style``
* ``name``
* ``direction``


``ExposureContrastTransform``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applies an exposure, contrast, or gamma adjustment. Uses dynamic properties
for optimal live adjustments (e.g., in viewports).

.. code-block:: yaml

    !<ExposureContrastTransform> {style: linear, exposure: -1.5, 
      contrast: 0.5, gamma: 1.1, pivot: 0.18}

Keys:

* ``exposure``
* ``contrast``
* ``pivot``
* ``gamma``
* ``style``
* ``log_exposure_step``
* ``log_midway_gray``
* ``name``
* ``direction``


``FileTransform``
^^^^^^^^^^^^^^^^^

Applies a lookup table (LUT)

Keys:

* ``src``
* ``cccid``
* ``cdl_style``
* ``interpolation``
* ``direction``


``FixedFunctionTransform``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Applies one of a set of fixed, special purpose, mathematical operators.

Keys:

* ``style``
* ``params``
* ``name``
* ``direction``


``GradingPrimaryTransform``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applies primary color correction.

Keys:

* ``style``
* ``brightness``
* ``contrast``
* ``pivot``
* ``offset``
* ``exposure``
* ``lift``
* ``gamma``
* ``gain``
* ``saturation``
* ``clamp``
* ``name``
* ``direction``


``GradingRGBCurveTransform``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applies a spline-based curve.

Keys:

* ``style``
* ``red``
* ``green``
* ``blue``
* ``master``
* ``lintolog_bypass``
* ``name``
* ``direction``


``GradingToneTransform``
^^^^^^^^^^^^^^^^^^^^^^^^

Applies an adjustment to various tonal ranges.

Keys:

* ``style``
* ``blacks``
* ``shadows``
* ``midtones``
* ``highlights``
* ``whites``
* ``s_contrast``
* ``name``
* ``direction``


``GroupTransform``
^^^^^^^^^^^^^^^^^^

Combines multiple transforms into one.

.. code-block:: yaml

    colorspaces:
    
      - !<ColorSpace>
        name: adx10

        [...]

        to_reference: !<GroupTransform>
          children:
            - !<FileTransform> {src: adx_adx10_to_cdd.spimtx}
            - !<FileTransform> {src: adx_cdd_to_cid.spimtx}

A group transform is accepted anywhere a "regular" transform is.

Keys:

* ``children``
* ``name``
* ``direction``


``LogAffineTransform``
^^^^^^^^^^^^^^^^^^^^^^

Applies a logarithm as well as a scale and offset on both the linear and 
log sides.  May be used to implement Cineon or Pivoted (Josh Pines) style 
lin-to-log transforms.

Keys:

* ``base``
* ``lin_side_offset``
* ``lin_side_slope``
* ``log_side_offset``
* ``log_side_slope``
* ``name``
* ``direction``


``LogCameraTransform``
^^^^^^^^^^^^^^^^^^^^^^

Similar to LogAffineTransform but also allows a linear section near black.
May be used to implement the ACEScct non-linearity as well as many camera
vendor lin-to-log transforms.

Keys:

* ``base``
* ``lin_side_offset``
* ``lin_side_slope``
* ``log_side_offset``
* ``log_side_slope``
* ``lin_side_break``
* ``linear_slope``
* ``name``
* ``direction``


``LogTransform``
^^^^^^^^^^^^^^^^

Applies a mathematical logarithm with a given base to the pixel values.

Keys:

* ``base``
* ``name``
* ``direction``


``LookTransform``
^^^^^^^^^^^^^^^^^

Applies a named look

Keys:

* ``src``
* ``dst``
* ``looks``
* ``direction``


``MatrixTransform``
^^^^^^^^^^^^^^^^^^^

Applies a matrix transform to the pixel values

Keys:

* ``matrix``
* ``offset``
* ``name``
* ``direction``


``RangeTransform``
^^^^^^^^^^^^^^^^^^

Applies an affine transform (scale & offset) and clamps values to min/max bounds.

Keys:

* ``min_in_value``
* ``max_in_value``
* ``min_out_value``
* ``max_out_value``
* ``style``
* ``name``
* ``direction``

.. note::

    If a min_in_value is present, then min_out_value must also be present and the result 
    is clamped at the low end. Similarly, if max_in_value is present, then max_out_value 
    must also be present and the result is clamped at the high end.


.. _config-named-transforms:

Named Transforms
****************

Sometimes it is helpful to include one or more transforms in a config that are essentially
stand-alone transforms that do not have a fixed relationship to a reference space or a
process space.  An example would be a "utility curve" transform where the intent is to
simply apply a LUT1D without any conversion to a reference space.  In these cases, a
``named_transforms`` section may be added to the config with one or more named transforms.

Note that named transforms do not show up in color space menus by default, so the 
application developer must implement support to make them available to users.

This feature may be used to emulate older methods of color management that ignored the 
RGB primaries and simply applied one-dimensional transformations.  However, config authors 
are encouraged to implement transforms as normal OCIO color spaces wherever possible.

Named transforms support the keys:

* ``name``
* ``aliases``
* ``description``
* ``family``
* ``categories``
* ``encoding``
* ``transform``
* ``inverse_transform``

.. code-block:: yaml

    named_transforms:
      - !<NamedTransform>
        name: Utility Curve -- Cineon Log to Lin
        transform: !<FileTransform> {src: logtolin_curve.spi1d}
