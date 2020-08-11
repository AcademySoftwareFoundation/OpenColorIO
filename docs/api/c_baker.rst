
Baker
*****

.. class:: Baker


In certain situations it is necessary to serialize transforms into a variety of application specific LUT formats. Note that not all file formats that may be read also support baking.

**Usage Example:** *Bake a CSP sRGB viewer LUT*

::

   OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
   OCIO::BakerRcPtr baker = OCIO::Baker::Create();
   baker->setConfig(config);
   baker->setFormat("csp");
   baker->setInputSpace("lnf");
   baker->setShaperSpace("log");
   baker->setTargetSpace("sRGB");
   auto & metadata = baker->getFormatMetadata();
   metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A first comment");
   metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A second comment");
   std::ostringstream out;
   baker->bake(out); // fresh bread anyone!
   std::cout << out.str();

Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: BakerRcPtr OpenColorIO::Baker::Create()

         Create a new Baker. 

      .. cpp:function:: BakerRcPtr OpenColorIO::Baker::createEditableCopy() const

         Create a copy of this Baker.

   .. group-tab:: Python

      .. py:method:: Baker.__init__(*args, **kwargs)**

         Overloaded function.

         1. __init__(self: PyOpenColorIO.Baker) -> None

         2. __init__(self: PyOpenColorIO.Baker, config: PyOpenColorIO.Config, format: str, inputSpace: str, targetSpace: str, looks: str = ‘’, cubeSize: int = -1, shaperSpace: str = ‘’, shaperSize: int = -1) -> None


Methods
=======
.. tabs::

   .. group-tab:: C++

      **Config**

      .. cpp:function:: ConstConfigRcPtr OpenColorIO::Baker::getConfig() const


      .. cpp:function:: void OpenColorIO::Baker::setConfig(const ConstConfigRcPtr &config)


         Set the config to use. 

      **Format**

      .. cpp:function:: const char *OpenColorIO::Baker::getFormat() const


      .. cpp:function:: void OpenColorIO::Baker::setFormat(const char *formatName)

         Set the LUT output format. 

      .. cpp:function:: FormatMetadata &OpenColorIO::Baker::getFormatMetadata()

         Get editable *optional* format metadata. The metadata that will be used varies based on the capability of the given file format. Formats such as CSP, IridasCube, and ResolveCube will create comments in the file header using the value of any first-level children elements of the formatMetadata. The CLF/CTF formats will make use of the top-level “id” and “name” attributes and children elements “Description”, “InputDescriptor”, “OutputDescriptor”, and “Info”. 

      .. cpp:function:: const char *OpenColorIO::Baker::getFormatNameByIndex(int index)

         Get the LUT baker format name at index, return empty string if an invalid index is specified. 

      .. cpp:function:: const char *OpenColorIO::Baker::getFormatExtensionByIndex(int index)

         Get the LUT baker format extension at index, return empty string if an invalid index is specified. 

      .. cpp:function:: int OpenColorIO::Baker::getNumFormats()

         Get the number of LUT bakers. 

      **Input Space**

      .. cpp:function:: const char *OpenColorIO::Baker::getInputSpace() const


      .. cpp:function:: void OpenColorIO::Baker::setInputSpace(const char *inputSpace)

         Set the input `ColorSpace` that the LUT will be applied to. 

      **Shaper Space**

      .. cpp:function:: const char *OpenColorIO::Baker::getShaperSpace() const


      .. cpp:function:: void OpenColorIO::Baker::setShaperSpace(const char *shaperSpace)

         Set an *optional* `ColorSpace` to be used to shape / transfer the input colorspace. This is mostly used to allocate an HDR luminance range into an LDR one. If a shaper space is not explicitly specified, and the file format supports one, the `ColorSpace <c_colorspace.rst#classOpenColorIO_1_1ColorSpace>`_ Allocation will be used (not implemented for all formats). 

      **Looks**

      .. cpp:function:: const char *OpenColorIO::Baker::getLooks() const


      .. cpp:function:: void OpenColorIO::Baker::setLooks(const char *looks)

         Set the looks to be applied during baking. Looks is a potentially comma (or colon) delimited list of lookNames, where +/- prefixes are optionally allowed to denote forward/inverse look specification. (And forward is assumed in the absence of either). 

      **Target Space**

      .. cpp:function:: const char *OpenColorIO::Baker::getTargetSpace() const


      .. cpp:function:: void OpenColorIO::Baker::setTargetSpace(const char *targetSpace)

         Set the target device colorspace for the LUT. 

      **Shaper Space**

      .. cpp:function:: int OpenColorIO::Baker::getShaperSize() const


      .. cpp:function:: void OpenColorIO::Baker::setShaperSize(int shapersize)

         Override the default shaper LUT size. Default value is -1, which allows each format to use its own most appropriate size. For the CLF format, the default uses a half-domain LUT1D (which is ideal for scene-linear inputs). 

      **Cube Size**

      .. cpp:function:: int OpenColorIO::Baker::getCubeSize() const


      .. cpp:function:: void OpenColorIO::Baker::setCubeSize(int cubesize)

         Override the default cube sample size. default: <format specific> 

      **Execute**

      .. cpp:function:: void OpenColorIO::Baker::bake(std::ostream &os) const

         Bake the LUT into the output stream. 

   .. group-tab:: Python

      **Config**

      .. py:method:: Baker.getConfig(self: PyOpenColorIO.Baker) -> PyOpenColorIO.Config

      .. py:method:: Baker.setConfig(self: PyOpenColorIO.Baker, config: PyOpenColorIO.Config) -> None

      **Format**

      .. py:method:: Baker.getFormats() -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Baker>, 0>

      .. py:method:: Baker.getFormat(self: PyOpenColorIO.Baker) -> str

      .. py:method:: Baker.setFormat(self: PyOpenColorIO.Baker, formatName: str) -> None

      .. py:method:: Baker.getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. getFormatMetadata(self: PyOpenColorIO.Baker) -> PyOpenColorIO.FormatMetadata

         2. getFormatMetadata(self: PyOpenColorIO.Baker) -> PyOpenColorIO.FormatMetadata

      **Input Space**

      .. py:method:: Baker.getInputSpace(self: PyOpenColorIO.Baker) -> str

      .. py:method:: Baker.setInputSpace(self: PyOpenColorIO.Baker, inputSpace: str) -> None

      **Shaper Space**

      .. py:method:: Baker.getShaperSpace(self: PyOpenColorIO.Baker) -> str

      .. py:method:: Baker.setShaperSpace(self: PyOpenColorIO.Baker, shaperSpace: str) -> None

      **Looks**

      .. py:method:: Baker.getLooks(self: PyOpenColorIO.Baker) -> str

      .. py:method:: Baker.setLooks(self: PyOpenColorIO.Baker, looks: str) -> None

      **Target Space**

      .. py:method:: Baker.getTargetSpace(self: PyOpenColorIO.Baker) -> str

      .. py:method:: Baker.setTargetSpace(self: PyOpenColorIO.Baker, targetSpace: str) -> None

      **Shaper Space**

      .. py:method:: Baker.getShaperSize(self: PyOpenColorIO.Baker) -> int

      .. py:method:: Baker.setShaperSize(self: PyOpenColorIO.Baker, shaperSize: int) -> None

      **Cube Size**

      .. py:method:: Baker.getCubeSize(self: PyOpenColorIO.Baker) -> int

      .. py:method:: Baker.setCubeSize(self: PyOpenColorIO.Baker, cubeSize: int) -> None

      **Execute**

      .. py:method:: Baker.bake(*args, **kwargs)

         Overloaded function.

         1. bake(self: PyOpenColorIO.Baker, fileName: str) -> None

         2. bake(self: PyOpenColorIO.Baker) -> str
