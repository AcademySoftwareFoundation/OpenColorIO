..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. autoclass:: PyOpenColorIO.GpuShaderDesc
   :members:
   :undoc-members:
   :special-members: __init__
   :exclude-members: TextureType, 
                     TEXTURE_RED_CHANNEL, 
                     TEXTURE_RGB_CHANNEL, 
                     UniformData,
                     Texture, 
                     Texture3D, 
                     UniformIterator,
                     TextureIterator, 
                     Texture3DIterator,
                     DynamicPropertyIterator
   :inherited-members:

.. autoclass:: PyOpenColorIO.GpuShaderDesc.TextureType
   :members:
   :undoc-members:
   :exclude-members: name

   .. py:method:: name() -> str
      :property:

.. autoclass:: PyOpenColorIO.GpuShaderDesc.UniformData
   :members:
   :undoc-members:

.. autoclass:: PyOpenColorIO.GpuShaderDesc.Texture
   :members:
   :undoc-members:

.. autoclass:: PyOpenColorIO.GpuShaderDesc.Texture3D
   :members:
   :undoc-members:

.. autoclass:: PyOpenColorIO.GpuShaderDesc.UniformIterator
   :special-members: __getitem__, __iter__, __len__, __next__

.. autoclass:: PyOpenColorIO.GpuShaderDesc.TextureIterator
   :special-members: __getitem__, __iter__, __len__, __next__

.. autoclass:: PyOpenColorIO.GpuShaderDesc.Texture3DIterator
   :special-members: __getitem__, __iter__, __len__, __next__

.. autoclass:: PyOpenColorIO.GpuShaderDesc.DynamicPropertyIterator
   :special-members: __getitem__, __iter__, __len__, __next__
