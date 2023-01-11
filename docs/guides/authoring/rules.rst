..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _rules:


.. _config-rules:

File & Viewing rules
********************

.. _config-file-rules:


``file_rules``
^^^^^^^^^^^^^^

.. warning::
    Either the file_rules section or the default role are Required for configs of
    version 2 or higher.

Use the File Rules to assign a default color space to files based on their path.

Here is example showing the various types of rules that may be defined:

.. code-block:: yaml

  file_rules:
    - !<Rule> {name: LogC, extension: "*", pattern: "*LogC*", colorspace: ARRI LogC}
    - !<Rule> {name: OpenEXR, extension: "exr", pattern: "*", colorspace: ACEScg}
    - !<Rule> {name: TIFF, regex: ".*\\.TIF?F", colorspace: sRGB}
    - !<Rule> {name: ColorSpaceNamePathSearch}
    - !<Rule> {name: Default, colorspace: default}

The File Rules are a set of mappings that are evaluated from the top down. The 
first rule to match is what determines which ColorSpace is returned.

There are four types of rules available. Each rule type has a name key that may 
be used by applications to refer to that rule. Name values must be unique. The 
other keys depend on the rule type.

1. Basic Rules -- 
This is the basic rule type that uses Unix glob style pattern matching and is 
thus very easy to use. It contains the keys:

* ``name``: Name of the rule.
* ``pattern``: Glob pattern to be used for the main part of the name/path.
  This is case-sensitive.  It must be in double-quotes.  Set it to "*" if you only 
  want the rule to consider the extension.
* ``extension``: Glob pattern or string to be used for the file extension. Note that
  if glob tokens are not used, the extension will be used in a non-case-sensitive 
  way by default.  For example the simple string "exr" would match "exr" and "EXR".  
  If you only want to match "exr", use the glob pattern "[e][x][r]".  It must be 
  in double-quotes.  Set it to "*" if you only want the rule to consider the pattern.
* ``colorspace``: ColorSpace name to be returned.

2. RegEx Rules -- 
This is similar to the basic rule but allows additional capabilities for 
power-users. It contains the keys:

* ``name``: Name of the rule.
* ``regex``: Regular expression to be evaluated.  It must be in double-quotes.
* ``colorspace``: ColorSpace name to be returned.

Note that a backslash character in a RegEx expression needs to be doubled up as ``\\``
(as shown in the example above for the TIFF rule) to make it through the Yaml parsing.

3. OCIO v1 style Rule -- 
This rule allows the use of the OCIO v1 style, where the string is searched for 
ColorSpace names from the config. This rule may occur 0 or 1 times in the list. 
The position in the list prioritizes it with respect to the other rules. It has 
the key:

* ``name``: Must be ``ColorSpaceNamePathSearch``.

4. Default Rule -- 
The file_rules section must always end with this rule. If no prior rules match, this 
rule specifies the ColorSpace applications will use. It has the keys:

* ``name``: must be ``Default``.
* ``colorspace``: ColorSpace name to be returned.

Note: OCIO v1 defined a ``default`` role intended to specify a default color space
when reading files. However, given the confusion over the usage of this role, the
Default file rule is now the preferred way to indicate this mapping.  However, if 
the file_rules section is not present in the config, the new API will try to use 
the default role in place of the Default Rule. If both are present, the Default 
Rule takes precedence. If both the file_rules section and the default role are 
missing, an exception will be thrown when loading the config.

Note that the strictparsing token does not affect the behavior of the File Rules 
API. In other words, evaluating the rules will always result in a ColorSpace being 
available to an application. However, the API also allows the application to know 
which rule was the matching one. So apps that want to work in "strict" mode should 
first check if strictparsing is true and if so check to see if the matching rule 
was the Default Rule. If so, it could then notify the user and take whatever action 
is appropriate.  (As an alternative to checking which rule number was matched, the 
API call ``filepathOnlyMatchesDefaultRule`` may be used instead.)

Roles may be used rather than ColorSpace names in the rules.

It is also legal for rules to have additional key:value pairs where the value 
may be an arbitrary string. The API provides access to getting/setting these 
additional pairs and will preserve them on a Config read/write.  These may be
used to define application-specific behavior.

Note to developers: The older ``parseColorSpaceFromString`` API call is now deprecated
and should be replaced with ``getColorSpaceFromFilepath``.


``strictparsing``
^^^^^^^^^^^^^^^^^

Optional. Valid values are ``true`` and ``false``. Default is ``true``
(assuming a config is present):

.. code-block:: yaml

    strictparsing: true

.. warning::
    This attribute is from OCIO v1.  In OCIO v2, the FileRules system was
    introduced and so the ``strictparsing`` attribute is less relevant now.
    The ``parseColorSpaceFromString`` API call is now deprecated and the
    proper way to obtain this information is ``getColorSpaceFromFilepath``.
    The FileRules always return a default color space but the API
    ``filepathOnlyMatchesDefaultRule`` may be used by applications that
    want to take some special action if ``strictparsing`` is true.

OCIO v1 provided a mechanism for applications to extract the colorspace
from a filename (the ``parseColorSpaceFromString`` API method).

So for a file like ``example_render_v001_lnf.0001.exr`` it will
determine the colorspace ``lnf`` (it being the right-most substring
containing a colorspace name).

However, if the colorspace cannot be determined and ``strictparsing:
true``, it will return an empty string.

If the colorspace cannot be determined and ``strictparsing: false``,
the default role will be used. This allows unhandled images to operate
in "non-color managed" mode.

Application authors should note: when no config is present (e.g. via
``$OCIO``), the default internal profile specifies
``strictparsing=false``, and the default color space role is
``raw``. This means that ANY string passed to OCIO will be parsed as
the default ``raw``. This is nice because in the absence of a config,
the behavior from your application perspective is that the library
essentially falls back to "non-color managed".

.. _config-viewing-rules:

``viewing_rules``
^^^^^^^^^^^^^^^^^

Optional. 

Use the Viewing Rules to assign default views to color spaces.

The Viewing Rules allow config authors to help applications provide a better
user experience by specifying the views appropriate to use for a given color
space.  For example, applications may use the default view when making 
thumbnail images for its user interface.  Likewise, an application could select 
the default view the first time it displays an image in a viewport.

Here is an example:

.. code-block:: yaml

  viewing_rules:
    - !<Rule> {name: video-spaces, colorspaces: [sRGB, Rec709]}
    - !<Rule> {name: data-spaces, colorspaces: [alpha, normals]}

  displays:
    sRGB:
      - !<View> {name: Video, view_transform: colorimetry, display_colorspace: sRGB, rule: video-spaces}
      - !<View> {name: Raw, colorspace: nc10, rule: data-spaces}

This is helpful for situations where a given view is intended for use with just 
a few specific color spaces. However in other situations, it would be helpful to 
be able to define rules to be used with a broader set of color spaces.

Color spaces may now have an "encoding" attribute to allow grouping color spaces 
into groups such as "scene-linear", "log", "sdr-video", and "data". The Viewing
Rules makes it possible to define a rule based on the encoding attribute of a 
color space rather than a set of named color spaces. For example:

.. code-block:: yaml

  viewing_rules:
    - !<Rule> {name: scene-linear-or-log, encodings: [scene-linear, log]}

  displays:
    sRGB:
      - !<View> {name: ACES, view_transform: ACES-sdr-video, display_colorspace: sRGB, rule: scene-linear-or-log}

The colorspaces and encodings attributes may contain a single value or a list of 
values. It is illegal for a rule to define both a list of colorspaces and encodings 
simultaneously.

Also, similar to the file_rules, it is allowed for a rule to define a set of custom 
key/value pairs like this:

.. code-block:: yaml

  - !<Rule> { name: scene-linear-rule, encodings: scene-linear, custom: {key1: value1, key2: value2} }

The key names and values are arbitrary strings. This may be useful to control 
application-specific behavior.

A Viewing Rule may contain the following keys:

* ``name``: The name of the rule (must be unique).
* ``encodings``: The color space encodings used by the rule (may be a list).
* ``colorspaces``: The color space names used by the rule (may be a list).
* ``custom``: A set of arbitrary key/value string pairs.

The API allows applications to request the list of views for a given color space.
This uses the viewing rules to filter the views for the given display based on the 
color space name and encoding. Views that do not have a rules attribute are always 
returned (so if no rules are present, the results are the same as the unfiltered API 
call).

Note that the active_views may be used to remove views that are not appropriate for
a given user or workstation.  If the active_views list is non-empty, any views that 
are not in that list will not appear in the results provided to the application
(regardless of whether the view appears in a rule).  

Furthermore, active_views will continue to sort (that is, determine the index order) 
the list of views in all of the API calls.

The first allowed view for a color space is the default.  
