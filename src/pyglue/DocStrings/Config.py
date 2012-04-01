
class Config:
    """
    Config
    """
    
    def __init__(self):
        pass

    def CreateFromEnv(self):
        """
        CreateFromEnv()
        
        Create a :py:class:`PyOpenColourIO.Config` object using the environment variable.
                     
        :returns: Config object
        """
        pass

    def CreateFromFile(self):
        """
        CreateFromFile(filename)
        
        Create a :py:class:`PyOpenColourIO.Config` object using the information in a file.
        
        :param filename: name of file
        :type filename: string
        :return: Config object
        """
        pass

    def isEditable(self):
        """
        isEditable()
        
        Returns whether Config is editable.
        
        The configurations returned from
        :py:function:`PyOpenColourIO.GetCurrentConfig` are not editable, and if
        you want to edit them you can use
        :py:method:`PyOpenColourIO.Config.createEditableCopy`.
           
        If you attempt to call any of the set functions on a noneditable
        Config, an exception will be thrown.
           
        :return: state of :py:class:`PyOpenColourIO.Config`'s editability
        :rtype: bool
        """
        pass

    def createEditableCopy(self):
        """
        createEditableCopy()
        
        Returns an editable copy of :py:class:`PyOpenColourIO.Config`.
        
        :return: editable copy of :py:class:`PyOpenColourIO.Config`
        :rtype: Config object
        """
        pass

    def sanityCheck(self):
        """
        sanityCheck()
        
        This will throw an exception if :py:class:`PyOpenColourIO.Config` is
        malformed. The most common error occurs when references are made to
        colourspaces that do not exist.
        """
        pass

    def getDescription(self):
        """
        getDescription()
        
        Returns the stored description of :py:class:`PyOpenColourIO.Config`.
           
        :return: stored description of :py:class:`PyOpenColourIO.Config`
        :rtype: string
        """
        pass

    def setDescription(self, desc):
        """
        setDescription(desc)
        
        Sets the description of :py:class:`PyOpenColourIO.Config`.
        
        :param desc: description of :py:class:`PyOpenColourIO.Config`
        :type desc: string
        """
        pass

    def serialize(self):
        """
        serialize()
        
        Returns the string representation of :py:class:`PyOpenColourIO.Config`
        in YAML text form. This is typically stored on disk in a file with the
        .ocio extension.
        
        :return: :py:class:`PyOpenColourIO.Config` in YAML text form
        :rtype: string
        """
        pass

    def getCacheID(self, pycontext=None):
        """
        getCacheID([, pycontext])
        
        This will produce a hash of the all colourspace definitions, etc.
        
        All external references, such as files used in FileTransforms, etc.,
        will be incorporated into the cacheID. While the contents of the files
        are not read, the file system is queried for relavent information
        (mtime, inode) so that the :py:class:`PyOpenColourIO.Config`'s cacheID
        will change when the underlying luts are updated.
        
        If a context is not provided, the current Context will be used. If a
        null context is provided, file references will not be taken into
        account (this is essentially a hash of :py:method:`PyOpenColourIO.Config.serialize`).
           
        :param pycontext: optional
        :type pycontext: object
        :return: hash of :py:class:`PyOpenColourIO.Config`
        :rtype: string
        """
        pass

    def getSearchPath(self):
        """
        getSearchPath()
        
        Returns the search path.
        
        :return: search path
        :rtype: string
        """
        pass

    def setSearchPath(self, searchpath):
        """
        setSearchPath(path)
        
        Sets the search path.
        
        :param path: the search path
        :type path: string
        """
        pass

    def getWorkingDir(self):
        """
        getWorkingDir()
        
        Returns the working directory.
        
        :return: the working directory
        :rtype path: string
        """
        pass

    def setWorkingDir(self, dirname):
        """
        setWorkingDir(path)
        
        Sets the working directory.
        
        :param path: the working directory
        :type path: string
        """
        pass

    def getColourSpaces(self):
        """
        getColourSpaces()
        
        Returns all the ColourSpaces defined in :py:class:`Config`.
           
        :return: ColourSpaces in :py:class:`PyOpenColourIO.Config`
        :rtype: tuple
        """
        pass

    def getColourSpace(self, name):
        """
        getColourSpace(name)
        
        Returns the data for the specified colour space in :py:class:`Config`.
        
        This will return null if the specified name is not found.
        
        :param name: name of colour space
        :type name: string
        :return: data for specified colour space
        :rtype: pyColourSpace object
        """
        pass

    def addColourSpace(self, colourSpace):
        """
        addColourSpace(pyColourSpace)
        
        Add a specified colour space to :py:class:`PyOpenColourIO.Config`.
        
        :param pyColourSpace: colour space
        :type pyColourSpace: object
        
        .. note::
           If another colour space is already registered with the same name,
           this will overwrite it.
        """
        pass

    def clearColourSpaces(self):
        """
        clearColourSpaces()
        
        Clear the colour spaces in :py:class:`PyOpenColourIO.Config`.
        """
        pass

    def parseColourSpaceFromString(self, str):
        """
        parseColourSpaceFromString(str)
        
        Parses out the colour space from a string.
        
        Given the specified string, gets the longest, right-most colour space substring.
        * If strict parsing is enabled, and no colour space is found, return an empty string.
        * If strict parsing is disabled, return the default role, if defined.
        * If the default role is not defined, return an empty string.
        
        :param str: ColourSpace data
        :type str: string
        :return: parsed data
        :rtype: string
        """
        pass

    def setRole(self, role, csname):
        """
        setRole(role, csname)
        
        Set a role's ColourSpace.
        
        Setting the colourSpaceName name to a null string unsets it.
        
        :param role: role whose ColourSpace will be set
        :type role: string
        :param csname: name of ColourSpace
        :type csname: string
        """
        pass
        
    def getDefaultDisplay(self):
        """
        getDefaultDisplay()
        
        Returns the default display set in :py:class:`PyOpenColourIO.Config`.
        
        :return: default display
        :rtype: string 
        """
        pass

    def getDisplays(self):
        """
        getDisplays()
        
        Returns all the displays defined in :py:class:`PyOpenColourIO.Config`.
        
        :return: displays in :py:class:`Config`
        :rtype: list of strings
        """
        pass

    def getDefaultView(self, display):
        """
        getDefaultView(display)
        
        Returns the default view of :py:class:`PyOpenColourIO.Config`.
        
        :param display: default view
        :type display: string
        :return: view
        :rtype: string
        """
        pass

    def getViews(self, display):
        """
        getViews(display)
        
        Returns all the views defined in :py:class:`PyOpenColourIO.Config`.
        
        :param display: views in :py:class:`Config`
        :type display: string
        :return: views in :py:class:`Config`.
        :rtype: list of strings
        """
        pass

    def getDisplayColourSpaceName(self, display, view):
        """
        getDisplayColourSpaceName(display, view)
        
        Returns the ColourSpace name corresponding to the display and view
        combination in :py:class:`PyOpenColourIO.Config`.
        
        :param display: display
        :type display: string
        :param view: view
        :type view: string
        :return: display colour space name
        :rtype: string
        """
        pass
    
    def getDisplayLooks(self, display, view):
        """
        getDisplayLooks(display, view)
        
        Returns the looks corresponding to the display and view combination in
        :py:class:`PyOpenColourIO.Config`.
        
        :param display: display
        :type display: string
        :param view: view
        :type view: string
        :return: looks
        :rtype: string
        """
        pass
    
    def addDisplay(self, display, view, csname, looks=None):
        """
        addDisplay(display, view, colourSpaceName[, looks])
        
        NEEDS WORK
        
        :param display:
        :type display: string
        :param view: 
        :type view: string
        :param colourSpaceName: 
        :type colourSpaceName: string
        :param looks: optional
        :type looks: string
        """
        pass

    def clearDisplays(self):
        """
        clearDisplays()
        """
        pass

    def setActiveDisplays(self, dislpays):
        """
        setActiveDisplays(displays)
        
        Sets the active displays in :py:class:`PyOpenColourIO.Config`.
        
        :param displays: active displays
        :type displays: string
        """
        pass

    def getActiveDisplays(self):
        """
        getActiveDisplays()
        
        Returns the active displays in :py:class:`PyOpenColourIO.Config`.
        
        :return: active displays
        :rtype: string
        """
        pass

    def setActiveViews(self, views):
        """
        setActiveViews(views)
        
        Sets the active views in :py:class:`PyOpenColourIO.Config`.
        
        :param views: active views
        :type views: string
        """
        pass

    def getActiveViews(self):
        """
        getActiveViews()
        
        Returns the active views in :py:class:`PyOpenColourIO.Config`.
        
        :return: active views
        :rtype: string
        """
        pass

    def getDefaultLumaCoefs(self):
        """
        getDefaultLumaCoefs()
        
        Returns the default luma coefficients in :py:class:`PyOpenColourIO.Config`.
           
        :return: luma coefficients
        :rtype: list of floats
        """
        pass

    def setDefaultLumaCoefs(self, coefficients):
        """
        setDefaultLumaCoefs(pyCoef)
        
        Sets the default luma coefficients in :py:class:`PyOpenColourIO.Config`.
        
        :param pyCoef: luma coefficients
        :type pyCoef: object
        """
        pass

    def getLook(self, lookname):
        """
        getLook(str)
        
        Returns the information of a specified look in
        :py:class:`PyOpenColourIO.Config`.
        
        :param str: look
        :type str: string
        :return: specified look
        :rtype: look object
        """
        pass

    def getLooks(self):
        """
        getLooks()
        
        Returns a list of all the looks defined in
        :py:class:`PyOpenColourIO.Config`.
        
        :return: looks
        :rtype: tuple of look objects
        """
        pass

    def addLook(self, look):
        """
        addLook(pylook)
        
        Adds a look to :py:class:`PyOpenColourIO.Config`.
        
        :param pylook: look
        :type pylook: look object
        """
        pass

    def clearLooks(self):
        """
        clearLooks()
        
        Clear looks in :py:class:`PyOpenColourIO.Config`.
        """
        pass

    def getProcessor(self, arg1, arg2=None, direction=None, context=None):
        """
        getProcessor(arg1[, arg2[, direction[, context]])
        
        Returns a processor for a specified transform.
        
        Although this is not often needed, it allows for the reuse of atomic
        OCIO functionality, such as applying an individual LUT file.
        
        There are two canonical ways of creating a
        :py:class:`PyOpenColourIO.Processor`:
        
        #. Pass a transform into arg1, in which case arg2 will be ignored. 
        #. Set arg1 as the source and arg2 as the destination. These can be
           ColourSpace names, objects, or roles.
        
        Both arguments, ``direction`` (of transform) and ``context``, are
        optional and respected for both methods of
        :py:class:`PyOpenColourIO.Processor` creation.
        
        This will fail if either the source or destination colour space is null.
        
        See Python: Processor for more details.
        
        .. note::
            This may provide higher fidelity than anticipated due to internal
            optimizations. For example, if inputColourSpace and outputColourSpace
            are members of the same family, no conversion will be applied, even
            though, strictly speaking, quantization should be added.
            
            If you wish to test these calls for quantization characteristics,
            apply in two steps; the image must contain RGB triples (though
            arbitrary numbers of additional channels can be optionally
            supported using the pixelStrideBytes arg). ???
        
        :param arg1: 
        :type arg1: object
        :param arg2: ignored if arg1 is a transform
        :type arg2: object
        :param direction: optional
        :type direction: string
        :param context: optional
        :type context: object
        """
        pass
