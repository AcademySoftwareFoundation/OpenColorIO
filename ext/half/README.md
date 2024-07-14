Header-only Half support
========================

This is a stripped version of Imath library's half implementation built on top of the version 3.1.10.
The original code could be found at https://github.com/AcademySoftwareFoundation/Imath/tree/v3.1.10

This header-only version will be used instead of the full Imath library as external dependency when
the OCIO_USE_HALF_LOOKUP_TABLE CMake option is NOT set. 

There are only few modifications on the original code:
- turned off the half<->float conversion LUT by forcing the IMATH_HALF_NO_LOOKUP_TABLE definition 
- commented out std::ostream& >> and << operators
- commented out printBits() functions.

Please note that except for the LUT and test routines, the half support in OCIO is utilizing the 
hardware conversion if the library is built with F16C suport turned on (default) and running on a CPU
that has the F16C instruction set. Therefore the performance impact of not-using the half lookup table
is very limited on typical cases.