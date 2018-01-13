.. _allocationvars:

How to Configure ColorSpace Allocation
======================================

The allocation / allocation vars are utilized using during GPU 3dlut / shader
text generation. (Processor::getGpuShaderText, Processor::getGpuLut3D).

If, in the course of GPU processing, a 3D lut is required, the "allocation /
allocation vars" direct how OCIO should sample the colorspace, with the intent
being to maintain maximum fidelity and minimize clamping.

Currently support allocations / variables:

ALLOCATION_UNIFORM::
    2 vars: [min, max]

ALLOCATION_LG2::
    2 vars: [lg2min, lg2max]
    3 vars: [lg2min, lg2max, linear_offset]

So say you have an srgb image (such as an 8-bit tif), where you know the data
ranges between 0.0 - 1.0 (after converting to float).  If you wanted to apply
a 3d lut to this data, there is no danger in samplingthat space uniformly and
clamping data outside (0,1).   So for this colorspace we would tag it:

.. code-block:: yaml

    allocation: uniform
    allocationvars: [0.0, 1.0]

These are the defaults, so the tagging could also be skipped.

But what if you were actually first processing the data, where occasionally
small undershoot and overshoot values were encountered? If you wanted OCIO to
preserve this overshoot / undershoot pixel information, you would do so by
modifying the allocation vars.

.. code-block:: yaml

    allocation: uniform
    allocationvars: [-0.125, 1.125]

This would mean that any image data originally within [-0.125, 1.125] will be
preserved during GPU processing.  (Protip: Data outside this range *may*
actually be preserved in some circumstances - such as if a 3d lut is not needed
- but it's not required to be preserved).

So why not leave this at huge values (such as [-1000.0, 1000.0]) all the time?
Well, there's a cost to supporting this larger dynamic range, and that cost is
reduced precision within the 3D luts sample space.   So in general you're best
served by using sensible allocations (the smallest you can get away with, but
no smaller).

Now in the case of high-dynamic range color spaces (such as float linear), a
uniform sampling is not sufficient because the max value we need to preserve is
so high.

Say you were using a 32x32x32 3d lookup table (a common size).  Middle gray is
at 0.18, and specular values are very much above that.  Say the max value we
wanted to preserve in our coding space is 256.0, each 3d lut lattice coordinates
would represent 8.0 units of linear light! That means the vast majority of the
perceptually significant portions of the space wouldnt be sampled at all!

unform allocation from 0-256\:
0
8.0
16.0
...
240.0
256.0

So another allocation is defined, lg2

.. code-block:: yaml

      - !<ColorSpace>
        name: linear
        description: |
            Scene-linear, high dynamic range. Used for rendering and compositing.
        allocation: lg2
        allocationvars: [-8, 8]

In this case, we're saying that the appropriate ways to sample the 3d lut are
logarithmically, from 2^-8 stops to 2^8 stops.

Sample locations:
2^-8\: 0.0039
2^-7\: 0.0078
2^-6\: 0.0156
...
2^0\:  1.0
...
2^6\:  64.0
2^7\:  128.0
2^8\:  256.0

Which gives us a much better perceptual sampling of the space.

The one downside of this approach is that it can't represent 0.0,
which is why we optionally allow a 3d allocation var, a black point
offset.  If you need to preserve 0.0 values, and you have a high
dynamic range space, you can specify a small offset.

Example:

.. code-block:: yaml

    allocation: lg2
    allocationvars: [-8, 8, 0.00390625]

The [-15.0, 6.0] values in spi-vfx come from the fact that all of the
linearizations provided in that profile span the region from 2^-15
stops, to 2^6 stops.   One could probably change that black point to a
higher number (such as -8), but if you raised it too much you would
start seeing black values be clipped.   Conversely, on the high end
one could raise it a bit but if you raised it too far the precision
would suffer around gray, and if you lowered it further you'd start to
see highlight clipping.
