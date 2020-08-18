..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Baker
=====

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class Baker**

In certain situations it is necessary to serialize transforms into
a variety of application specific LUT formats. Note that not all
file formats that may be read also support baking.

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

.. tabs::

    .. group-tab:: C++

        .. cpp:function:: BakerRcPtr createEditableCopy() const

            Create a copy of this Baker.

        .. cpp:function:: ConstConfigRcPtr getConfig() const

        .. cpp:function:: void setConfig(const ConstConfigRcPtr &config)

            Set the config to use.

        .. cpp:function:: const char *getFormat() const

        .. cpp:function:: void setFormat(const char *formatName)

            Set the LUT output format.

        .. cpp:function:: const FormatMetadata &getFormatMetadata() const

        .. cpp:function:: FormatMetadata &getFormatMetadata()

            Get editable *optional* format metadata. The metadata that will
            be used varies based on the capability of the given file format.
            Formats such as CSP, IridasCube, and ResolveCube will create
            comments in the file header using the value of any first-level
            children elements of the formatMetadata. The CLF/CTF formats
            will make use of the top-level “id” and “name” attributes and
            children elements “Description”, “InputDescriptor”,
            “OutputDescriptor”, and “Info”.

        .. cpp:function:: const char *getInputSpace() const

        .. cpp:function:: void setInputSpace(const char *inputSpace)

            Set the input ColorSpace that the LUT will be applied to.

        .. cpp:function:: const char *getShaperSpace() const

        .. cpp:function:: void setShaperSpace(const char *shaperSpace)

            Set an *optional* ColorSpace to be used to shape / transfer the
            input colorspace. This is mostly used to allocate an HDR
            luminance range into an LDR one. If a shaper space is not
            explicitly specified, and the file format supports one, the
            ColorSpace Allocation will be used (not implemented for all
            formats).

        .. cpp:function:: const char *getLooks() const

        .. cpp:function:: void setLooks(const char *looks)

            Set the looks to be applied during baking. Looks is a
            potentially comma (or colon) delimited list of lookNames, where
            +/- prefixes are optionally allowed to denote forward/inverse
            look specification. (And forward is assumed in the absence of
            either).

        .. cpp:function:: const char *getTargetSpace() const

        .. cpp:function:: void setTargetSpace(const char *targetSpace)

            Set the target device colorspace for the LUT.

        .. cpp:function:: int getShaperSize() const

        .. cpp:function:: void setShaperSize(int shapersize)

            Override the default shaper LUT size. Default value is -1, which
            allows each format to use its own most appropriate size. For the
            CLF format, the default uses a half-domain LUT1D (which is ideal
            for scene-linear inputs).

        .. cpp:function:: int getCubeSize() const

        .. cpp:function:: void setCubeSize(int cubesize)

            Override the default cube sample size. default: <format specific>

        .. cpp:function:: void bake(std::ostream &os) const

            Bake the LUT into the output stream.

        .. cpp:function:: ~Baker()

        -[ Public Static Functions ]-

        .. cpp:function:: BakerRcPtr Create()

            Create a new Baker.

        .. cpp:function:: int getNumFormats()

            Get the number of LUT bakers.

        .. cpp:function:: const char *getFormatNameByIndex(int index)

            Get the LUT baker format name at index, return empty string if
            an invalid index is specified.

        .. cpp:function:: const char *getFormatExtensionByIndex(int index)

            Get the LUT baker format extension at index, return empty string
            if an invalid index is specified.


    .. group-tab:: Python

        .. py:class:: PyOpenColorIO.Baker

        .. py:class:: FormatIterator

        .. py:method:: bake(*args,**kwargs)

        Overloaded function.

        1. .. py:method:: bake(self: PyOpenColorIO.Baker, fileName: str) -> None

        2. .. py:method:: bake(self: PyOpenColorIO.Baker) -> str

        .. py:method:: getConfig(self: PyOpenColorIO.Baker) -> PyOpenColorIO.Config

        .. py:method:: getCubeSize(self: PyOpenColorIO.Baker) -> int

        .. py:method:: getFormat(self: PyOpenColorIO.Baker) -> str

        .. py:method:: getFormatMetadata(*args, **kwargs)**

        Overloaded function.

        1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Baker) -> PyOpenColorIO.FormatMetadata

        2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Baker) -> PyOpenColorIO.FormatMetadata

        .. py:method:: getFormats() -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Baker>, 0>

        .. py:method:: getInputSpace(self: PyOpenColorIO.Baker) -> str

        .. py:method:: getLooks(self: PyOpenColorIO.Baker) -> str

        .. py:method:: getShaperSize(self: PyOpenColorIO.Baker) -> int

        .. py:method:: getShaperSpace(self: PyOpenColorIO.Baker) -> str

        .. py:method:: getTargetSpace(self: PyOpenColorIO.Baker) -> str

        .. py:method:: setConfig(self: PyOpenColorIO.Baker, config: PyOpenColorIO.Config) -> None

        .. py:method:: setCubeSize(self: PyOpenColorIO.Baker, cubeSize: int) -> None

        .. py:method:: setFormat(self: PyOpenColorIO.Baker, formatName: str) -> None

        .. py:method:: setInputSpace(self: PyOpenColorIO.Baker, inputSpace: str) -> None

        .. py:method:: setLooks(self: PyOpenColorIO.Baker, looks: str) -> None

        .. py:method:: setShaperSize(self: PyOpenColorIO.Baker, shaperSize: int) -> None

        .. py:method:: setShaperSpace(self: PyOpenColorIO.Baker, shaperSpace: str) -> None

        .. py:method:: setTargetSpace(self: PyOpenColorIO.Baker, targetSpace: str) -> None
