#! /usr/bin/python

# Use OpenImageIO's oiiotool to compare the reference image set to the actual image set produced
# by a CLF implementation.  The result of each image comparison is printed to the shell.  If none
# of the comparisons result in an error and they all say "0  > .002,.002,.002", which means that
# there are zero pixels that exceed the error metric threshold, then the test is a success.
#
# Usage:
# > python compare_clf_test_frames.py <PATH-TO-REFERENCE-IMAGES> <PATH-TO-ACTUAL-IMAGES>

# This script is python 2 and python 3 compatible.

import os
import tempfile

def process_frames( ref_path, act_path ):

    # Check the arguments are as expected.
    if not os.path.isdir( ref_path ):
        raise ValueError( "Reference image directory must exist: " + ref_path )
    if not os.path.isdir( act_path ):
        raise ValueError( "Actual image directory must exist: " + act_path )

    # Take the aim and actual images and compute abs(aim - act) / max(abs(aim), 0.1).
    base_cmd = 'oiiotool %s --dup %s --absdiff --swap --abs --maxc 0.1 --div '

    # Mask out the Inf and NaN values in the CLF test kit target image.
    box_cmd = '--box:color=0,0,0:fill=1 1008,771,1023,798 --box:color=0,0,0:fill=1 0,1023,1,1023 '

    # Error out if there are any new NaNs generated (there should not be).
    nan_err_cmd = '--fixnan error '

    # Print out how many pixels are greater than the test threshold of 0.002.
    range_cmd = '--rangecheck 0,0,0 .002,.002,.002 '

    # Oiiotool seems to need a -o to avoid issuing a useless warning, so add an output file
    # even though it is not used.  (Note that trying to write to /dev/null here doesn't work.)
    avoid_warning_cmd = '-o ' + os.path.join(tempfile.gettempdir(), 'tmp.exr') + ' '

    oiio_cmd = base_cmd + box_cmd + nan_err_cmd + range_cmd + avoid_warning_cmd

    # Iterate over each pair of test images.
    for f in sorted(os.listdir( ref_path )):
        fname, ext = os.path.splitext( f )
        if ext == '.exr':

            # Build the full path to the files.
            ref = os.path.join( ref_path, f )
            act = os.path.join( act_path, f )

            # Build the command.
            cmd = oiio_cmd % (ref, act)

            print('');  print( cmd )

            # Process the image.
            os.system( cmd )


if __name__=='__main__':
    import sys
    if len( sys.argv ) is not 3:
        raise ValueError( "USAGE: python  compare_clf_test_frames.py  <REF_IMAGE_DIR>  <ACTUAL_IMAGE_DIR>" )
    process_frames( sys.argv[1], sys.argv[2] )
