import os
import sys
from random import randint

import PyOpenColorIO as OCIO

def assertEqualRGBM(testCase, first, second):
    testCase.assertEqual(first.red, second.red)
    testCase.assertEqual(first.green, second.green)
    testCase.assertEqual(first.blue, second.blue)
    testCase.assertEqual(first.master, second.master)

def assertEqualPrimary(testCase, first, second):
    assertEqualRGBM(testCase, first.brightness, second.brightness)
    assertEqualRGBM(testCase, first.contrast, second.contrast)
    assertEqualRGBM(testCase, first.gamma, second.gamma)
    assertEqualRGBM(testCase, first.offset, second.offset)
    assertEqualRGBM(testCase, first.exposure, second.exposure)
    assertEqualRGBM(testCase, first.lift, second.lift)
    assertEqualRGBM(testCase, first.gain, second.gain)
    testCase.assertEqual(first.pivot, second.pivot)
    testCase.assertEqual(first.saturation, second.saturation)
    testCase.assertEqual(first.clampWhite, second.clampWhite)
    testCase.assertEqual(first.clampBlack, second.clampBlack)
    testCase.assertEqual(first.pivotWhite, second.pivotWhite)
    testCase.assertEqual(first.pivotBlack, second.pivotBlack)

def assertEqualRGBMSW(testCase, first, second):
    testCase.assertEqual(first.red, second.red)
    testCase.assertEqual(first.green, second.green)
    testCase.assertEqual(first.blue, second.blue)
    testCase.assertEqual(first.master, second.master)
    testCase.assertEqual(first.start, second.start)
    testCase.assertEqual(first.width, second.width)

def assertEqualTone(testCase, first, second):
    assertEqualRGBMSW(testCase, first.blacks, second.blacks)
    assertEqualRGBMSW(testCase, first.whites, second.whites)
    assertEqualRGBMSW(testCase, first.shadows, second.shadows)
    assertEqualRGBMSW(testCase, first.highlights, second.highlights)
    assertEqualRGBMSW(testCase, first.midtones, second.midtones)
    testCase.assertEqual(first.scontrast, second.scontrast)

def assertEqualControlPoint(testCase, first, second):
    testCase.assertEqual(first.x, second.x)
    testCase.assertEqual(first.y, second.y)

def assertEqualBSpline(testCase, first, second):
    pts1 = first.getControlPoints()
    pts2 = second.getControlPoints()
    for pt1 in pts1:
        try:
            pt2 = next(pts2)
            assertEqualControlPoint(testCase,pt1, pt2)
        except StopIteration:
            raise AssertionError("Different number of control points")
    try:
        pt2 = next(pts2)
    except StopIteration:
        pass
    else:
        raise AssertionError("Different number of control points")

def assertEqualRGBCurve(testCase, first, second):
    assertEqualBSpline(testCase, first.red, second.red)
    assertEqualBSpline(testCase, first.green, second.green)
    assertEqualBSpline(testCase, first.blue, second.blue)
    assertEqualBSpline(testCase, first.master, second.master)

class MuteLogging:
  def __init__(self):
    self.previous_function = None
  
  def __enter__(self):
    self.previous_function = OCIO.SetLoggingFunction(lambda message: None)
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    OCIO.ResetToDefaultLoggingFunction()

# -----------------------------------------------------------------------------
# Python 2/3 compatibility
# -----------------------------------------------------------------------------
if sys.version_info.major >= 3:
    STRING_TYPES = str
else:  # Python 2
    STRING_TYPES = basestring

# -----------------------------------------------------------------------------
# Test data
# -----------------------------------------------------------------------------
TEST_DATAFILES_DIR = os.getenv('TEST_DATAFILES_DIR')

TEST_NAMES = ['default_name', 'HelloWorld', 'Simple Colourspace', 'a1b2c3d4', 
              'RGBA.1&2*3#']

TEST_DESCS = [
    'These: &lt; &amp; &quot; &apos; &gt; are escape chars', 
    'this is a description',

    'The show reference space. This is a sensor referred linear representation '
    'of the scene with primaries that correspond to\nscanned film. 0.18 in '
    'this space corresponds to a properly\nexposed 18 % grey card.',

    'A raw color space. Conversions to and from this space are no-ops.',
    'how many transforms can we use?']

TEST_CATEGORIES = ['linear', 'rendering', 'log', '123', 'a1b.']

SIMPLE_CONFIG = """ocio_profile_version: 2

search_path: luts
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw
  scene_linear: lnh

viewing_rules:
  - !<Rule> {name: Rule_1, colorspaces: c1, custom: {key0: value0, key1: value1}}
  - !<Rule> {name: Rule_2, colorspaces: [c2, c3]}
  - !<Rule> {name: Rule_3, encodings: log}
  - !<Rule> {name: Rule_4, encodings: [log, scene-linear], custom: {key1: value2, key2: value0}}

displays:
  sRGB:
    - !<View> {name: Film1D, colorspace: vd8}
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: lnh
    family: ln
    equalitygroup: ""
    bitdepth: 16f
    description: |
      The show reference space. This is a sensor referred linear
      representation of the scene with primaries that correspond to
      scanned film. 0.18 in this space corresponds to a properly
      exposed 18% grey card.
    isdata: false
    allocation: lg2

  - !<ColorSpace>
    name: vd8
    family: vd8
    equalitygroup: ""
    bitdepth: 8ui
    description: how many transforms can we use?
    isdata: false
    allocation: uniform
    to_reference: !<GroupTransform>
      children:
        - !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}
        - !<MatrixTransform> {matrix: [1, 2, 3, 4, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1], offset: [1, 2, 0, 0]}
        - !<CDLTransform> {slope: [0.9, 1, 1], offset: [0.1, 0.3, 0.4], power: [1.1, 1.1, 1.1], sat: 0.9}

  - !<ColorSpace>
    name: c1
    encoding: scene-linear
    categories: [rendering, linear]

  - !<ColorSpace>
    name: c2
    categories: [rendering]

  - !<ColorSpace>
    name: c3
    categories: [sample]

  - !<ColorSpace>
    name: c4
    encoding: log
    categories: [sample]
"""

SAMPLE_CONFIG = """ocio_profile_version: 2

environment:
  {}

search_path: luts
strictparsing: true
family_separator: /
luma: [0.2126, 0.7152, 0.0722]

roles:
  reference: lin_1
  scene_linear: lin_1
  rendering: lin_1
  default: raw

view_transforms:
  - !<ViewTransform>
    name: view_transform
    from_scene_reference: !<MatrixTransform> {}

displays:
  DISP_1:
    - !<View> {name: VIEW_1, colorspace: view_1}
    - !<View> {name: VIEW_2, colorspace: view_2}
    - !<View> {name: VIEW_3, colorspace: view_3}
  DISP_2:
    - !<View> {name: VIEW_4, colorspace: view_3, looks: look_4}
    - !<View> {name: VIEW_3, colorspace: view_3, looks: look_3}
    - !<View> {name: VIEW_2, colorspace: view_2, looks: "look_3, -look_4"}
    - !<View> {name: VIEW_1, colorspace: view_1}

active_displays: []
active_views: []

looks:
  - !<Look>
    name: look_3
    process_space: log_1
    transform: !<CDLTransform> {slope: [1, 2, 1]}

  - !<Look>
    name: look_4
    process_space: log_1
    transform: !<CDLTransform> {slope: [1.2, 2.2, 1.2]}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: Raw
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: lin_1
    categories: [ working-space, basic-2d ]
    encoding: scene-linear

  - !<ColorSpace>
    name: lin_2
    categories: [ working-space ]
    encoding: scene-linear

  - !<ColorSpace>
    name: log_1
    categories: [ working-space ]
    encoding: log
    to_reference: !<LogTransform> {base: 2, direction: inverse}

  - !<ColorSpace>
    name: in_1
    family: Input / Camera/Acme
    description: |
      An input color space.
      For the Acme camera.
    categories: [ file-io, basic-2d ]
    to_reference: !<ExponentTransform> {value: [2.6, 2.6, 2.6, 1]}
    encoding: sdr-video

  - !<ColorSpace>
    name: in_2
    categories: [ file-io, advanced-2d ]
    to_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1]}
    encoding: sdr-video

  - !<ColorSpace>
    name: in_3
    categories: [ file-io, working-space, advanced-3d ]
    encoding: scene-linear
    to_reference: !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}

  - !<ColorSpace>
    name: view_1
    from_reference: !<ExponentTransform> {value: [2.6, 2.6, 2.6, 1], direction: inverse}

  - !<ColorSpace>
    name: view_2
    from_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}

  - !<ColorSpace>
    name: view_3
    from_reference: !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1], direction: inverse}

  - !<ColorSpace>
    name: lut_input_1
    categories: [ look-process-space, LUT-connection-space ]
    from_reference: !<ExponentTransform> {value: [2.6, 2.6, 2.6, 1], direction: inverse}

  - !<ColorSpace>
    name: lut_input_2
    categories: [ look-process-space, advanced-2d ]
    from_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}

  - !<ColorSpace>
    name: lut_input_3
    categories: [ file-io, look-process-space, advanced-2d ]
    from_reference: !<ExponentTransform> {value: [2.4, 2.4, 2.4, 1], direction: inverse}

display_colorspaces:
  - !<ColorSpace>
    name: display_lin_1
    categories: [ working-space, basic-2d ]
    encoding: scene-linear

  - !<ColorSpace>
    name: display_lin_2
    categories: [ working-space ]
    encoding: sdr-video

  - !<ColorSpace>
    name: display_log_1
    categories: [ working-space, advanced-2d ]
    encoding: log
    to_display_reference: !<LogTransform> {base: 2, direction: inverse}

named_transforms:
  - !<NamedTransform>
    name: nt1
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: nt2
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

  - !<NamedTransform>
    name: nt3
    categories: [ file-io, basic-2d ]
    encoding: sdr-video
    family: Raw 
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
"""

SIMPLE_CONFIG_VIRTUAL_DISPLAY = """ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}
  - !<View> {name: sview2, colorspace: raw}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}
    - !<Views> [sview1]

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<View> {name: Film, view_transform: display_vt, display_colorspace: <USE_DISPLAY_NAME>}
  - !<Views> [sview2]

active_displays: []
active_views: []

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: true
    allocation: uniform
"""

SIMPLE_CONFIG_VIRTUAL_DISPLAY_ACTIVE_DISPLAY = """ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<Views> [sview1]

active_displays: [sRGB]
active_views: [view]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
"""

SIMPLE_CONFIG_VIRTUAL_DISPLAY_V1 = """ocio_profile_version: 1

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}

colorspaces:
  - !<ColorSpace>
    name: raw
"""

SIMPLE_CONFIG_VIRTUAL_DISPLAY_EXCEPTION = """ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<Views> [sview1]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
"""