..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _quick_start_artists:

Quick Start for Artists
=======================

Most users will likely want to use the OpenColorIO that comes precompiled with
their applications.  See the :ref:`compatiblesoftware` for further details on
each application.

Note that OCIO configurations are required to do any 'real' work, and are
available separately on the :ref:`downloads` section of this site. Example
images are also available. For assistance customizing .ocio configurations,
contact `ocio-user <https://lists.aswf.io/g/ocio-user>`__\.

- Step 1:  set the ``$OCIO`` environment-variable to ``/path/to/your/config.ocio``
- Step 2:  Launch supported application.

If you are on a platform that is not envvar friendly, most applications also
provide a menu option to select a different OCIO configuration after launch.

Please be sure to select a configuration that matches your color workflow (VFX 
work typically requires a different profile than animated features). If you need
assistance picking a profile, email 
`ocio-user <https://lists.aswf.io/g/ocio-user>`__\.
