..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Glossary
========

- Transform - a function that alters RGB(A) data (e.g transform an image 
  from scene linear to sRGB)
- Reference space - a space that connects colorspaces
- Colorspace - a meaningful space that can be transferred to and from the 
  reference space
- Display - a virtual or physical display device (e.g an sRGB display device)
- View - a meaningful view of the reference space on a Display (e.g a film 
  emulation view on an sRGB display device)
- Role - abstract colorspace naming (e.g specify the "lnh" colorspace as the 
  scene_linear role, or the color-picker UI uses color_picking role)
- Look - a color transform which applies a creative look (for example a 
  per-shot neutral grade to remove color-casts from a sequence of film scans, 
  or a DI look)
