.. _userguide-looks:

Looks
=====

A "look" is a named color transform, intended to modify the look of an
image in a "creative" manner (as opposed to a colorspace definition
which tends to be technically/mathematically defined). An OCIO look typically
exists as a flexible addendum to a defined viewing transform. 

Examples of looks may be a neutral grade, to be applied to film scans
prior to VFX work, or a per-shot DI grade decided on by the director,
to be applied just before the viewing transform.

Looks are defined similarly to colorspaces, you specify a name and a
transform (possibly a GroupTransform containing several other
transforms), and optionally an inverse transform.

Where looks differ from colorspace definions are in how they are
applied. With a look, you also specify the "process space" - the
colorspace in which the transform is applied.

Example configuration
*********************

**Step 1: Setup a Look**

A look is a top-level OCIO configuration object. Conceptually, it's a
named transform which gets applied in a specific color space. All of the
changes below to the .ocio configs can be done manually by editing the
text, or using the Python API.

Example look definition in a OCIO config:

.. code-block:: yaml

    looks:
    - !<Look>
      name: di
      process_space: rclg16
      transform: !<FileTransform> {src: look_di.cc, interpolation: linear}

Example Python API call:

.. code-block:: python

    look = OCIO.Look(name='di', processSpace='rclg16')
    t = OCIO.FileTransform('look_di.cc', interpolation=OCIO.Constants.INTERP_LINEAR)
    look.setTransform(t)
    config.addLook(look)

The src file can be any LUT type that OCIO supports (in this case, it's a
file that contains the ``<ColorCorrection>`` element from a CDL file.) You
could also specify a .3dl, etc.

Once you define a look in your configuration, you'll see that the
``OCIOLookTransform`` node in Nuke will provide the named option. In
this example, the 'DI' look conceptually represents a look that will
be applied in DI. Other look names we often used are 'onset',
'editorial', etc.  The ``process_space`` specifies that the transform
should be applied in that space. In this example, if you provide
linear input to the ``OCIOLookTransform`` node, the pixels will be
converted to ``rclg16`` before applying the ``look_di.cc``
file-transform.

**Step 2: Update the Display to use a look.**

You can specify an optional 'looks' tag in the View tag, which will
apply the specified look(s). This lets application in the viewer
provide options which use the looks.

Example YAML config:

.. code-block:: yaml

    displays:
      DLP:
        - !<View> {name: Raw, colorspace: nc10}
        - !<View> {name: Log, colorspace: rclg10}
        - !<View> {name: Film, colorspace: p3dci16}
        - !<View> {name: Film DI, colorspace: p3dci16, looks: di}
      sRGB:
        - !<View> {name: Raw, colorspace: nc10}
        - !<View> {name: Log, colorspace: rclg10}
        - !<View> {name: Film, colorspace: srgb10}
        - !<View> {name: Film DI, colorspace: srgb10, looks: di}

Example Python API call:

.. code-block:: python

    for name,colorspace,look in [ ['Raw','nc10',''], ['Log','rclg10',''], 
            ['Film','p3dci16',''], ['Film DI','p3dci16','di'] ]:
        config.addDisplay('DLP',name,colorspace,look)

    for name,colorspace,look in [ ['Raw','nc10',''], ['Log','rclg10',''], 
            ['Film','srgb10',''], ['Film DI','srgb10','di'] ]:
        config.addDisplay('sRGB',name,colorspace,look)

Option for advanced users: The looks tag is actually a comma-delimited
list supporting +/- modifiers. So if you you wanted to specify a View
that undoes DI, and then adds Onset, you could do "-di,+onset".

**Step 3: Get per-shot looks supported.**

In the top example, look_di.cc, being a relative path location, will check
each location in the config's search_path. The first file that's found
will be used.

So if your config contains::

    search_path: luts

... then only the 'luts' subdir relative to the OCIO config will be
checked.

However if you specify::

    search_path: /shots/show/$SHOT/cc/data:luts

...the directory '/shots/show/$SHOT/cc/data/' will be evaluated first,
and only if not found will the 'luts' directory be checked.

env-vars, absolute, and relative paths can be used both in the config's
``search_path``, as well as the View's src specification.

Example:

.. code-block:: yaml

    - !<Look>
      name: di
      process_space: rclg16
      transform: !<FileTransform> {src: looks/$SHOT_di/current/look_$SHOT_di.cc, interpolation: linear}


Note that if the per-shot lut is not found, you can control whether a
fallback LUT succeeds based on if it's in the master location. You can
also use this for multiple levels (show, shot, etc).

Advanced option: If some shots use .cc files, and some use 3d-luts
currently there's no simple way to handle this. What we'd recommend as a
work around is to label all of your files with the same extension (such as
.cc), and then rely on OCIO's resiliance to misnamed lut files to just load
them anyways. Caveat: this only works in 1.0.1+ (commit sha-1: 6da3411ced)

Advanced option: In the Nuke OCIO nodes, you often want to preview
looks 'across shots' (often for reference, same-as, etc). You can
override the env-vars in each node, using the 'Context' menu. For
example, if you know that $SHOT is being used, in the context key1 you
should specify 'SHOT', and the in value1 specify the shot to use (such
as dev.lookdev). You can also use expressions, to say parse a shot
name out of ``[metadata "input/filename"]``

Advanced option: If you are writing your own OCIO integration code,
``getProcessor`` will fail if the per-shot lut is not found, and you
may want to distinguish this error from other OCIO errors. For this
reason, we provide OCIO::ExceptionMissingFile, which can be explicitly
caught (this can then handled using
``OCIO::DisplayTransform::setLooksOverride()``). I'd expect image
flipbook applications to use this approach.
