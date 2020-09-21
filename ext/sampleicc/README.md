ICC monitor profile support in OpenColorIO
==========================================

This is a stripped version of the SampleICC library built on top of the version 1.2.6.
The code could be downloaded from https://sourceforge.net/p/sampleicc/code/HEAD/tree/tags/1.2.6/


Although ociobakelut app uses the lcms library, we did not want to make the core OCIO library depend
on lcms. The use of SampleICC provided the basic ability to parse profile tags via a simple
header-only approach.


icProfileHeader.h
-----------------

The file is a copy & paste from SampleICC 1.2.6.

The only change is:
* Minor compilation fix on a problematic comment.
* Add of the 'dscm' Custom Apple Tag. 


iccProfileReader.h
------------------

The file was built using pieces of code from the library with adjustments to fit OpenColorIO needs.
So, only a few tags are currently supported.
