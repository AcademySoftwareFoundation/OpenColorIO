
class Baker:
    """
    In certain situations it is necessary to serialize transforms into
    a variety of application specific lut formats. The Baker can be
    used to create lut formats that ocio supports for writing.

    **Usage Example:** *Bake a houdini sRGB viewer lut*

    .. code-block:: python

        import PyOpenColorIO as OCIO
        config = OCIO.Config.CreateFromEnv()
        baker = OCIO.Baker()
        baker.setConfig(config)
        baker.setFormat('houdini')  # set the houdini type
        baker.setType('3D')  # we want a 3D lut
        baker.setInputSpace('lnf')
        baker.setShaperSpace('log')
        baker.setTargetSpace('sRGB')
        out = baker.bake()  # fresh bread anyone!
        with open('linear_to_sRGB.lut', 'w') as lutFile:
            lutFile.write(out)

    .. note::
        Except for icc profiles, this interface can be used in place of
        *ociobakelut* for lut file creation.

    """
    def __init__(self):
        pass

    def isEditable(self):
        """
        isEditable()

        Returns whether this Baker's :py:class:`PyOpenColorIO.Config`
        is editable.

        :return: state of :py:class:`PyOpenColorIO.Config`'s
            editability
        :rtype: bool
        """
        pass

    def createEditableCopy(self):
        """
        createEditableCopy()

        Create a copy of this :py:class:`PyOpenColorIO.Baker`.

        :return: Baker object
        :rtype: :py:class:`PyOpenColorIO.Baker`
        """
        pass

    def setConfig(self, config):
        """
        setConfig(config)

        Set the :py:class:`PyOpenColorIO.Config` to use.

        :param config: Config object
        :type config: :py:class:`PyOpenColorIO.Config`
        """
        pass

    def getConfig(self):
        """
        getConfig()

        Get the :py:class:`PyOpenColorIO.Config` to use.

        :return: Config object
        :rtype: :py:class:`PyOpenColorIO.Config`
        """
        pass

    def setFormat(self, formatName):
        """
        setFormat(formatName)

        Set the lut output format.

        Any registered file format *with write capability* is supported
        by Baker. Available formats can be queried programmatically via
        Python:

        .. code-block:: python

            import PyOpenColorIO as OCIO
            baker = OCIO.Baker()
            for i in range(baker.getNumFormats()):
                print baker.getFormatNameByIndex(i)

        :param formatName: format name (e.g. 'houdini')
        :type formatName: string
        """
        pass

    def getFormat(self):
        """
        getFormat()

        Get the lut output format.

        :return: format name
        :rtype: string
        """
        pass

    def setType(self, type):
        """
        setType(type)

        Set the lut output type (1D or 3D).

        :param type: type name (e.g. '3D')
        :type type: string
        """
        pass

    def getType(self):
        """
        getType()

        Get the lut output type.

        :return: type name
        :rtype: string
        """
        pass

    def setMetadata(self, metadata):
        """
        setMetadata(metadata)

        Set *optional* meta data for luts that support it.

        :param metadata: meta data text
        :type metadata: string
        """
        pass

    def getMetadata(self):
        """
        getMetadata()

        Get the meta data that has been set.

        :return: meta data text
        :rtype: string
        """
        pass

    def setInputSpace(self, inputSpace):
        """
        setInputSpace(inputSpace)

        Set the input :py:class:`PyOpenColorIO.ColorSpace` that the lut
        will be applied to.

        :param inputSpace: input ColorSpace name
        :type inputSpace: string
        """
        pass

    def getInputSpace(self):
        """
        getInputSpace()

        Get the input :py:class:`PyOpenColorIO.ColorSpace` that has
        been set.

        :return: input ColorSpace name
        :rtype: string
        """
        pass

    def setShaperSpace(self, shaperSpace):
        """
        setShaperSpace(shaperSpace)

        Set an *optional* :py:class:`PyOpenColorIO.ColorSpace` to be
        used to shape/transfer the input colorspace. This is mostly
        used to allocate an HDR luminance range into an LDR one. If a
        shaper space is not explicitly specified, and the file format
        supports one, the ColorSpace allocation will be used

        :param shaperSpace: shaper ColorSpace name
        :type shaperSpace: string
        """
        pass

    def getShaperSpace(self):
        """
        getShaperSpace()

        Get the shaper :py:class:`PyOpenColorIO.ColorSpace` that has
        been set.

        :return: shaper ColorSpace name
        :rtype: string
        """
        pass

    def setLooks(self, looks):
        """
        setLooks(looks)

        Set the looks to be applied during baking. Looks is a
        potentially comma (or colon) delimited list of look names,
        where +/- prefixes are optionally allowed to denote forward/
        inverse look specification (and forward is assumed in the
        absence of either).

        .. code-block:: python

            baker.setLooks('+cc,-di')

        A logical OR ("|") delimited looks token list specifies
        multiple looks options, evaluated left-to-right. This provides
        a fallback if one or more looks aren't available in the current
        context.

        .. code-block:: python

            # Evaluates to TWO options: (+cc,-onset), (+cc)
            baker.setLooks('+cc,-di|+cc')

        A trailing OR provides a fallback to NO applied looks.

        .. code-block:: python

            # Evaluates to THREE options: (+cc,-onset), (+cc), ()
            baker.setLooks('+cc,-di|+cc|')

        :param looks: looks token list
        :type looks: string
        """
        pass

    def getLooks(self):
        """
        getLooks()

        Get the looks to be applied during baking.

        :return: looks definition
        :rtype: string
        """
        pass

    def setTargetSpace(self, targetSpace):
        """
        setTargetSpace(targetSpace)

        Set the target :py:class:`PyOpenColorIO.ColorSpace` for the
        lut.

        :param targetSpace: target ColorSpace name
        :type targetSpace: string
        """
        pass

    def getTargetSpace(self):
        """
        getTargetSpace()

        Get the target :py:class:`PyOpenColorIO.ColorSpace` that has
        been set.

        :return: target ColorSpace name
        :rtype: string
        """
        pass

    def setShaperSize(self, shapersize):
        """
        setShaperSize(shapersize)

        Override the default shaper sample size.

        :param shapersize: sample size
        :type shapersize: int
        """
        pass

    def getShaperSize(self):
        """
        getShaperSize()

        Get the shaper sample size.

        :return: sample size
        :rtype: int
        """
        pass

    def setCubeSize(self, cubesize):
        """
        setCubeSize(cubesize)

        Override the default cube sample size.

        :param cubesize: sample size
        :type cubesize: int
        """
        pass

    def getCubeSize(self):
        """
        getCubeSize()

        Get the cube sample size.

        :return: sample size
        :rtype: int
        """
        pass

    def bake(self):
        """
        bake()

        Bake the lut into a string and return it.

        :return: formatted lut data
        :rtype: string
        """
        pass

    def getNumFormats(self):
        """
        getNumFormats()

        Get the number of lut writers.

        :return: format count
        :rtype: int
        """
        pass

    def getFormatNameByIndex(self, index):
        """
        getFormatNameByIndex(index)

        Get the lut writer name at index. Return an empty string if an
        invalid index is specified.

        :param index: format index
        :type index: int
        :return: format name
        :rtype: string
        """
        pass

    def getFormatExtensionByIndex(self, index):
        """
        getFormatExtensionByIndex(index)

        Get the lut writer file extension at index. Return an empty
        string if an invalid index is specified.

        :param index: format index
        :type index: int
        :return: format file extension
        :rtype: string
        """
        pass
