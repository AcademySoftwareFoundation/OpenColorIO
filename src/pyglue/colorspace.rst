COLOR SPACE

A ColorSpace is the state of an image in terms of colorimetry and color encoding. I.e., it defines how an
image's color information needs to be interpreted.

Transforming images between different color spaces is the primary motivation for the OCIO library.

While a complete discussion of color spaces is beyond the scope of this documentation,
traditional uses would be to have color spaces describing image capture devices,
such as cameras and scanners, and internal 'convenience' spaces, such as scene-linear and logarithmic.

ColorSpaces are specific to a particular image precision (float32, uint8, etc.).
The set of ColorSpaces that provide equivalent mappings (at different precisions)
are referred to as a 'family'.



DATA

Color spaces that are data are treated a bit specially. Any color-space transforms you apply to them are ignored
(think of applying a gamut-mapping transform to an ID pass). Also, the DisplayTransform process obeys 
special 'data min' and 'data max' args.

This is traditionally used for data representing noncolor pixel information, 
such as normals, point positions, and ID information.



ALLOCATION

If this ColorSpace needs to be transferred to a limited-dynamic-range coding space 
(such as during display with a GPU path), use this allocation to maximize bit efficiency.



LOOK
set of transforms. artistic intent rather than changes in coding.  in ACES (look modification transformation)

shot on a certain stock.  in DI amke warmer so warming filterwwith a specific amount.  Useful because 

A look is an 'artistic' image modification, in a specified image state.
The processSpace defines the color space the image is required to be in, for the math to apply correctly.



BAKER
In certain situations it is necessary to serialize transforms into a variety of application-specific 
lut formats. The Baker can be used to create lut formats that OCIO supports for writing.



TRANSFORMS
These are typically only needed when creating or manipulating configurations.



ROLES
Roles are used so that plugins can have abstract ways of asking for common color spaces, in addition
to this API, without referring to them by hardcoded names.
