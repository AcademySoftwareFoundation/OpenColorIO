# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Use OpenColorIO to apply the CLF files from the CLF test kit to the CLF target image
# and produce a directory of processed OpenEXR images at the specified location.
# Run the script with "-h" for usage information.

# This script is python 2.7 and python 3 compatible.

import os
import argparse

def process_frames( options ):

    dst_path = options.dst_dir
    use_gpu = options.gpu
    opt_level = options.opt

    # Check the arguments are as expected.
    if not os.path.exists( dst_path ):
        os.mkdir( dst_path )
    if not os.path.isdir( dst_path ):
        raise ValueError( "Destination path must be a directory: " + dst_path )

    # Get the path to the CLF target image, relative to the path of this script.
    script_path = os.path.abspath( __file__ )
    parts = script_path.split( os.sep )
    ocio_base_path = os.path.join( os.sep, *parts[0:-3] )
    src_image = os.path.join( ocio_base_path, 'share', 'clf', 'CLF_testimage.exr' )

    # Get the path to the CLF files, relative to the path of this script.
    clf_path = os.path.join( ocio_base_path, 'tests', 'data', 'files', 'clf' )

    # Set the optimization level. None or lossless avoids the fast SSE log/exponent.
    # (Note that the decimal value is available by simply printing the enum in Python.)
    if (opt_level == 'none') or (opt_level is None):
        # For default for this script, use no optimization rather than OCIO's default optimization
        # in order to apply the operators exactly as they appear in the CLF file with no attempt
        # to speed up the processing.
        print( 'Optimization level: None' )
        os.environ["OCIO_OPTIMIZATION_FLAGS"] = "0"
    elif opt_level == 'lossless':
        print( 'Optimization level: Lossless' )
        os.environ["OCIO_OPTIMIZATION_FLAGS"] = "144457667"
    elif opt_level == 'default':
        print( 'Optimization level: Default' )
    else:
        raise ValueError( 'Unexpected --opt argument.' )

    # TODO:  Add an option to turn on only SSE without removing any ops.

    if use_gpu:
        print( 'Processing on the GPU\n' )
        cmd_base = 'ocioconvert --gpu --lut %s %s %s'
    else:
        print( 'Processing on the CPU\n' )
        cmd_base = 'ocioconvert --lut %s %s %s'

    # Iterate over each legal CLF file in the suite.
    for f in os.listdir( clf_path ):
        fname, ext = os.path.splitext( f )
        if ext == '.clf':

            # Build the full path to the file.
            p = os.path.join( clf_path, f )

            # Build the name of the destination image.
            dst_image = os.path.join( dst_path, fname + '.exr' )

            # Build the command.
            cmd = cmd_base % (p, src_image, dst_image)
            print('=================');  print( cmd )

            # Process the image.
            os.system( cmd )


if __name__=='__main__':
    import sys

    import argparse
    parser = argparse.ArgumentParser(description='Process CLF test images using OCIO.')
    parser.add_argument('dst_dir',
                        help='Path to a directory where the result images will be stored.')
    parser.add_argument('--gpu', action='store_true',
                        help='Process using the GPU rather than the CPU.')
    parser.add_argument('--opt', choices=['none','lossless','default'],
                        help='Specify the OCIO optimization level. If not specified, "none" will be used.')
    options = parser.parse_args(sys.argv[1:])

    process_frames(options)

