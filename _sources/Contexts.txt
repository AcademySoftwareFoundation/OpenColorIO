Contexts
========

OCIO's allows different LUT's or grades to be applied based on the
current context.

These contexts are usually based on environment variables, but also
allows on-the-fly context switching in applications that operate on
multiple shots (such as playback tools)

Typically these would be used as part of the display transform, to
apply shot-specific looks (such as a CDL color correction, or a 1D
grade LUT)


.. _context_per_shot_luts:

A contrived example
*******************

The simplest way to explain this feature is with examples. Say we have
two shots, ``ab-123`` and ``sf-432``, and each shot requires a
different LUT to view. The current shot name is stored in the
environment variable SHOT.

In the OCIO config, you can use this SHOT environment variable to
construct the LUT's path/filename. This path can be absolute (e.g
``/example/path/${SHOT}.spi1d``), or relative to any directory on the
OCIO search path, which includes the resource path (e.g
``${SHOT}.spi1d``)

This is a simplified example, to demonstrate the context
feature. Typically this "contextual LUT" would be used in conjuction
with other LUT's (e.g before a scene-linear to log transform, followed
by a 3D film emulation LUT), this will be covered in
:ref:`context_per_shot_grade`

So, we have our empty OCIO config in ``~/showcfg``, and our two LUTs
in ``~/showcfg/luts`` which are named ``af-123.spi1d`` and
``sf-432.spi1d``::

    ~/showcfg/
        config.ocio
        luts/
            af-123.spi1d
            sf-432.spi1d

In the config, we first specify the config version, and the resource
path (usually this is relative to the directory containing
``config.ocio``, although can be an absolute path)::

    ocio_profile_version: 1
    resource_path: luts

Next, we define a colorspace that transforms from the show reference
space to the display colorspace:

.. code-block:: yaml

    colorspaces:
      - !<ColorSpace>
        name: srgb8
        family: srgb
        bitdepth: 8ui
        from_reference: !<FileTransform> {src: ${SHOT}.spi1d}

Then add a display alias for this transform:

.. code-block:: yaml

    displays:
      - !<Display> {device: sRGB, name: "Shot LUT", colorspace: srgb8}


Finally, we point the OCIO env-variable to the config, set the SHOT
env-variable to the shot being worked on, and launch Nuke (or any
other OCIO-enabled application)::

    export OCIO=~/showcfg/config.ocio
    export SHOT=af-123
    nuke

In Nuke, we create an OCIODisplay node, select our "sRGB" device with
the "Shot LUT" transform, and this will apply the ``af-123.spi1d``
LUT.

.. _context_per_shot_grade:

Per-shot grades
***************

Similarly to LUTs, we use a ``.cc`` file (an XML file containing a
single ASC CDL ``<ColorCorrection>``), or a ``.ccc`` file (an XML file
containing multiple ASC CDL color corrections, each with a unique ID)

The ``.cc`` file is applied identically to a regular LUT files, using
a ``FileTransform``. For example, if we have ``af-123.cc`` in the
``luts/`` directory:

.. code-block:: xml

    <ColorCorrection id="mygrade">
            <SOPNode>
                 <Slope>2 1 1</Slope>
                 <Offset>0 0 0</Offset>
                 <Power>1 1 1</Power>
            </SOPNode>
            <SATNode>
                 <Saturation>1</Saturation>
            </SATNode>
      </ColorCorrection>


We wish to apply this grade on the scene-linear image, then transform
into log and apply a 3D print emulation LUT. Since this requires
multiple transforms, instead of using a single ``FileTransform``, we
use a ``GroupTransform`` (which is is just a collection of other
transforms):

.. code-block:: yaml

    colorspaces:
      - !<ColorSpace>
        name: lnh
        family: ln
        bitdepth: 16f:
        isdata: false

      - !<ColorSpace>
        name: lg10
        family: lg
        bitdepth: 10ui
        isdata: false
        to_reference: !<FileTransform> {src: lg10.spi1d, interpolation: nearest}

      - !<ColorSpace>
        name: srgb8
        family: srgb
        bitdepth: 8ui
        isdata: false
        from_reference: !<GroupTransform>
          children:
            - !<FileTransform> {src: ${SHOT}.cc}
            - !<ColorSpaceTransform> {src: lnh, dst: lg10}
            - !<FileTransform> {src: film_emulation.spi3d, interpolation: linear}

A .ccc file is a collection of ``<ColorCorrection>``'s. The only
difference is when defining the ``FileTransform``, you must specify
the ``cccdid`` key, which you can also construct using the context's
environment variables. This means we could create a ``grades.ccc``
file containing the grade for all our shots:

.. code-block:: xml

    <ColorCorrectionCollection xmlns="urn:ASC:CDL:v1.2">
          <ColorCorrection id="af-123">
                  <SOPNode>
                       <Slope>2 1 1</Slope>
                       <Offset>0 0 0</Offset>
                       <Power>1 1 1</Power>
                  </SOPNode>
                  <SATNode>
                       <Saturation>1</Saturation>
                  </SATNode>
            </ColorCorrection>
            <ColorCorrection id="mygrade">
                    <SOPNode>
                         <Slope>0.9 0.7 0.9</Slope>
                         <Offset>0 0 0</Offset>
                         <Power>1 1 1</Power>
                    </SOPNode>
                    <SATNode>
                         <Saturation>1</Saturation>
                    </SATNode>
              </ColorCorrection>
    </ColorCorrectionCollection>

And the colorspace definition to utilise this:

.. code-block:: yaml

    - !<ColorSpace>
      name: srgb8
      family: srgb
      bitdepth: 8ui
      isdata: false
      from_reference: !<GroupTransform>
        children:
          - !<FileTransform> {src: grades.ccc, cccid: ${SHOT}}
          - !<ColorSpaceTransform> {src: lnh, dst: lg10}
          - !<FileTransform> {src: film_emulation.spi3d, interpolation: linear}


.. _context_complete_example:

A complete example
******************

.. warning::
    This is incomplete, the lnh_graded space is likely wrong

The context feature can be used to accommodate complex grading
pipelines. In this example, we have a "neutral grade" for each shot,
to neutralise color casts and exposure variations, keeping plates
consistent throughout a sequence.

To view a shot, we reverse this neutral grade, apply a "beauty grade",
then apply the display transform (the usual lin-to-log and a film
emulation LUT)

We will use the same two example shots from before, af-123 (which is
in the af sequence) and sg-432 (in the sg sequence). Imagine we have
many shots in each sequence, so we wish to put the grades for each
sequence in a separate file.

Using the same directory structure as above, in ``~/showcfg/luts`` we
first create two grade files, ``grades_af.ccc``:

.. code-block:: xml

    <ColorCorrectionCollection xmlns="urn:ASC:CDL:v1.2">
          <ColorCorrection id="af/af-123/neutral">
                  <SOPNode>
                       <Slope>2 1 1</Slope>
                       <Offset>0 0 0</Offset>
                       <Power>1 1 1</Power>
                  </SOPNode>
                  <SATNode>
                       <Saturation>1</Saturation>
                  </SATNode>
            </ColorCorrection>
            
          <ColorCorrection id="af/af-123/beauty">
                  <SOPNode>
                       <Slope>1.5 1.2 0.9</Slope>
                       <Offset>0 0 0</Offset>
                       <Power>1 1 1</Power>
                  </SOPNode>
                  <SATNode>
                       <Saturation>0.8</Saturation>
                  </SATNode>
            </ColorCorrection>

            <!-- More ColorCorrection's... -->
    </ColorCorrectionCollection>


And ``grades_sg.ccc``:

.. code-block:: xml

    <ColorCorrectionCollection xmlns="urn:ASC:CDL:v1.2">
            <ColorCorrection id="sg/sg-432/neutral">
                    <SOPNode>
                         <Slope>0.9 0.7 0.9</Slope>
                         <Offset>0 0 0</Offset>
                         <Power>1 1 1</Power>
                    </SOPNode>
                    <SATNode>
                         <Saturation>1</Saturation>
                    </SATNode>
              </ColorCorrection>
              
            <ColorCorrection id="sg/sg-432/beauty">
                    <SOPNode>
                         <Slope>1.1 0.9 0.8</Slope>
                         <Offset>0 0 0</Offset>
                         <Power>1.2 0.9 1.5</Power>
                    </SOPNode>
                    <SATNode>
                         <Saturation>1</Saturation>
                    </SATNode>
              </ColorCorrection>

              <!-- More ColorCorrection's.. -->
    </ColorCorrectionCollection>
    

Next, we create the ``config.ocio`` file, containing a colorspace to
define several colorspaces:

* ``lnh``, the scene-linear, 16-bit half-float space in which
  compositing will happen

* ``lg10``, the 10-bit log space in which material will be received
  (e.g in .dpx format)

* ``srgb8``, the display colorspace, for viewing the neutrally graded
  footage on an sRGB display

* ``srgb8graded``, another display colorspace, for viewing the final
  "beauty grade"


.. code-block:: yaml

    ocio_profile_version: 1

    # The directory relative to the location of this config
    resource_path: "luts"

    roles:
      scene_linear: lnh
      compositing_log: lgf

    displays:
      # Reference to display transform, without reversing the working grade
      - !<Display> {device: sRGB, name: Film1D, colorspace: srgb8}

      # Reference to display, reversing the working grade, and applying
      # the beauty grade
      - !<Display> {device: sRGB, name: Film1DGraded, colorspace: srgb8graded}

    colorspaces:

      # The source space, containing a log to scene-linear LUT
      - !<ColorSpace>
        name: lg10
        family: lg
        bitdepth: 10ui
        isdata: false
        to_reference: !<FileTransform> {src: lg10.spi1d, interpolation: nearest}

      # Our scene-linear space (reference space)
      - !<ColorSpace>
        name: lnh
        family: ln
        bitdepth: 16f
        isdata: false

      # Neutrally graded scene-linear
      - !<ColorSpace>
        name: lnh_graded
        family: ln
        bitdepth: 16f
        isdata: false
        to_reference: !<FileTransform> {src: "grades_${SEQ}.ccc", cccid: "${SEQ}/${SHOT}/neutral"}
          

      # The display colorspace - how to get from scene-linear to sRGB
      - !<ColorSpace>
        name: srgb8
        family: srgb
        bitdepth: 8ui
        isdata: false
        from_reference: !<GroupTransform>
          children:
            - !<ColorSpaceTransform> {src: lnh, dst: lg10}
            - !<FileTransform> {src: lg_to_srgb.spi3d, interpolation: linear}

      # Display color, with neutral grade reversed, and beauty grade applied
      - !<ColorSpace>
        name: srgb8graded
        family: srgb
        bitdepth: 8ui
        isdata: false
        from_reference: !<GroupTransform>
          children:
            - !<FileTransform> {src: "grades_${SEQ}.ccc", cccid: "${SEQ}/${SHOT}/neutral", direction: inverse}
            - !<FileTransform> {src: "grades_${SEQ}.ccc", cccid: "${SEQ}/${SHOT}/beauty", direction: forward}
            - !<ColorSpaceTransform> {src: lnh, dst: srgb8}
