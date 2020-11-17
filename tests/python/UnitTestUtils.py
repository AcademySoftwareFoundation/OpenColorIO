import os
import sys
from random import randint

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
TEST_DATAFILES_DIR = os.path.join(os.getenv('BUILD_LOCATION'), 'testdata')

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

  - !<ColorSpace>
    name: c2

  - !<ColorSpace>
    name: c3
"""
