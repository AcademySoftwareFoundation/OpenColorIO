..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _looks:


.. _config-looks:

Looks
*****

``looks``
^^^^^^^^^

Optional.

This section defines a list of "looks". A look is a color transform
defined similarly to a colorspace, with a few important differences.

For example, a look could be defined for a "first pass DI beauty
grade", which is used to view shots with a rough approximation of the
final grade.

When the look is defined in the config, you must specify a name, the
color transform, and the colorspace in which the grade is performed
(the "process space"). You can optionally specify an inverse transform
for when the look transform is not trivially invertable (e.g. it
applies a 3D LUT)

When an application applies a look, OCIO ensures the grade is applied
in the correct colorspace (by converting from the input colorspace to
the process space, applies the look's transform, and converts the
image to the output colorspace)

Here is a simple ``looks:`` section, which defines two looks:

.. code-block:: yaml

    looks:
      - !<Look>
        name: beauty
        process_space: lnf
        transform: !<CDLTransform> {slope: [1, 2, 1]}

      - !<Look>
        name: neutral
        process_space: lg10
        transform: !<FileTransform> {src: 'neutral-${SHOT}-${SEQ}.csp', interpolation: linear }
        inverse_transform: !<FileTransform> {src: 'neutral-${SHOT}-${SEQ}-reverse.csp', interpolation: linear }


Here, the "beauty" look applies a simple, static ASC CDL grade, making
the image very green (for some artistic reason!). The beauty look is
applied in the scene-linear ``lnf`` colorspace (this colorspace is
defined elsewhere in the config.

Next is a definition for a "neutral" look, which applies a
shot-specific CSP LUT, dynamically finding the correct LUT based on
the SEQ and SHOT :ref:`context variables <userguide-looks>`.

For example if ``SEQ=ab`` and ``SHOT=1234``, this look will search for
a LUT named ``neutral-ab-1234.csp`` in locations specified in
``search_path``.

The ``process_space`` here is ``lg10``. This means when the look is
applied, OCIO will perform the following steps:

* Transform the image from it's current colorspace to the ``lg10`` process space
* Apply apply the FileTransform (which applies the grade LUT)
* Transform the graded image from the process space to the output colorspace

The "beauty" look specifies the optional ``inverse_transform``,
because in this example the neutral CSP files contain a 3D LUT. For
many transforms, OCIO will automatically calculate the inverse
transform (as with the "beauty" look), however with a 3D LUT the
inverse transform needs to be defined.

If the look was applied in reverse, and ``inverse_transform`` as not
specified, then OCIO would give a helpful error message. This is
commonly done for non-invertable looks


As in colorspace definitions, the transform can be specified as a
series of transforms using the ``GroupTransform``, for example:

.. code-block:: yaml

    looks:
      - !<Look>
        name: beauty
        process_space: lnf
        transform: !<GroupTransform>
          children:
            - !<CDLTransform> {slope: [1, 2, 1]}
            - !<FileTransform> {src: beauty.spi1d, interpolation: nearest}
