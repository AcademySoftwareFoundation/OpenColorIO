.. _python-transforms:

Python Transforms
=================

Transform
*********

.. autoclass:: PyOpenColorIO.Transform
    :members:
    :undoc-members:

AllocationTransform
*******************

.. code-block:: python
    
    import PyOpenColorIO as OCIO
    transform = OCIO.AllocationTransform()
    transform.setAllocation(OCIO.Constants.ALLOCATION)

.. autoclass:: PyOpenColorIO.AllocationTransform
    :members:
    :undoc-members:
    :inherited-members:

CDLTransform
************

.. code-block:: python
    
    import PyOpenColorIO as OCIO
    
    cdl = OCIO.CDLTransform()
    # Set the slope, offset, power, and saturation for each channel.
    cdl.setSOP([, , , , , , , , ])
    cdl.setSat([, , ])
    cdl.getSatLumaCoefs()

.. autoclass:: PyOpenColorIO.CDLTransform
    :members:
    :undoc-members:
    :inherited-members:

ColorSpaceTransform
*******************

This class is meant so that ColorSpace conversions can be reused, referencing ColorSpaces that already exist.

.. note::
    Careless use of this may create infinite loops, so avoid referencing the colorspace you're in. 

.. code-block:: python
    
    import PyOpenColorIO as OCIO
    transform = OCIO.ColorSpaceTransform()

.. autoclass:: PyOpenColorIO.ColorSpaceTransform
    :members:
    :undoc-members:
    :inherited-members:

DisplayTransform
****************

.. code-block:: python
    
    import PyOpenColorIO as OCIO
    transform = OCIO.DisplayTransform()

.. autoclass:: PyOpenColorIO.DisplayTransform
    :members:
    :undoc-members:
    :inherited-members:

ExponentTransform
*****************

.. code-block:: python
    
    import PyOpenColorIO as OCIO
    transform = OCIO.ExponentTransform()

.. autoclass:: PyOpenColorIO.ExponentTransform
    :members:
    :undoc-members:
    :inherited-members:

FileTransform
*************

.. autoclass:: PyOpenColorIO.FileTransform
    :members:
    :undoc-members:
    :inherited-members:

GroupTransform
**************

.. autoclass:: PyOpenColorIO.GroupTransform
    :members:
    :undoc-members:
    :inherited-members:

LogTransform
************

.. code-block:: python
    
    import PyOpenColorIO as OCIO

:py:class:`PyOpenColorIO.LogTransform` is used to define a log transform. The direction of
the transform and its numerical base can be specified.

.. autoclass:: PyOpenColorIO.LogTransform
    :members:
    :undoc-members:
    :inherited-members:

LookTransform
*************

.. autoclass:: PyOpenColorIO.LookTransform
    :members:
    :undoc-members:
    :inherited-members:

MatrixTransform
***************

.. autoclass:: PyOpenColorIO.MatrixTransform
    :members:
    :undoc-members:
    :inherited-members:
