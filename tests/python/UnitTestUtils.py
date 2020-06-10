import os
import sys
from random import randint


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

SIMPLE_CONFIG = """ocio_profile_version: 1

search_path: luts
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw
  scene_linear: lnh

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
    description: |
      A raw color space. Conversions to and from this space are no-ops.

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
    description: |
      how many transforms can we use?

    isdata: false
    allocation: uniform
    to_reference: !<GroupTransform>
      children:
        - !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}
        - !<MatrixTransform> {matrix: [1, 2, 3, 4, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1], offset: [1, 2, 0, 0]}
        - !<CDLTransform> {slope: [0.9, 1, 1], offset: [0.1, 0.3, 0.4], power: [1.1, 1.1, 1.1], sat: 0.9}
"""
