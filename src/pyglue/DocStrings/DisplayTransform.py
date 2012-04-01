
class DisplayTransform:
    """
    Used to create transforms for displays.
    """
    def __init__(self):
        pass

    def getInputColourSpaceName(self):
        """
        getInputColourSpaceName()
        
        Returns the name of the input ColourSpace of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: name of input ColourSpace
        :rtype: string
        """
        pass
        
    def setInputColourSpaceName(self, name):
        """
        setInputColourSpaceName(name)
        
        Sets the name of the input ColourSpace of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param name: name of input ColourSpace
        :type name: string
        """
        pass
        
    def getLinearCC(self):
        """
        getLinearCC()
        
        Returns the linear CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: linear CC transform
        :rtype: object
        """
        pass
        
    def setLinearCC(self, transform):
        """
        setLinearCC(pyCC)
        
        Sets the linear CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param pyCC: linear CC
        :type pyCC: object
        """
        pass
        
    def getColourTimingCC(self):
        """
        getColourTimingCC()
        
        Returns the colour timing CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: colour timing CC transform
        :rtype: object
        """
        pass
        
    def setColourTimingCC(self, transform):
        """
        setColourTimingCC(pyCC)
        
        Sets the colour timing CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param pyCC: colour timing CC
        :type pyCC: object
        """
        pass
        
    def getChannelView(self):
        """
        getChannelView()
        
        Returns the channel view of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: channel view
        :rtype: object
        """
        pass
        
    def setChannelView(self, transform):
        """
        setChannelView(pyCC)
        
        Sets the channel view transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param pyCC: channel view transform
        :type pyCC: object
        """
        pass
        
    def getDisplay(self):
        """
        getDisplay()
        
        Returns the display of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: display
        :rtype: string
        """
        pass
        
    def setDisplay(self, displayName):
        """
        setDisplay(str)
        
        Sets the display of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param str: display
        :type str: string
        """
        pass
        
    def getView(self):
        """
        getView()
        
        Returns the view of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: view
        :rtype: string
        """
        pass
        
    def setView(self, viewName):
        """
        setView(str)
        
        Sets the view of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param str: view
        :type str: string
        """
        pass
        
    def getDisplayCC(self):
        """
        getDisplayCC()
        
        Returns the display CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :return: display CC
        :rtype: object
        """
        pass
        
    def setDisplayCC(self, transform):
        """
        setDisplayCC(pyCC)
        
        Sets the display CC transform of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param pyCC: display CC
        :type pyCC: object
        """
        pass
        
    def getLooksOverride(self):
        """
        getLooksOverride()
        
        Returns the looks in :py:class:`PyOpenColourIO.DisplayTransform` that's
        overriding :py:class:`PyOpenColourIO.Config`'s.
        
        :return: looks override
        :rtype: string
        """
        pass
        
    def setLooksOverride(self, looksStr):
        """
        setLooksOverride(str)
        
        Sets the looks override of :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param str: looks override
        :type str: string
        """
        pass
        
    def getLooksOverrideEnabled(self):
        """
        getLooksOverrideEnabled()
        
        Returns whether the looks override of
        :py:class:`PyOpenColourIO.DisplayTransform` is enabled.
        
        :return: looks override enabling
        :rtype: bool
        """
        pass
        
    def setLooksOverrideEnabled(self, enabled):
        """
        setLooksOverrideEnabled(enabled)
        
        Sets the looks override enabling of
        :py:class:`PyOpenColourIO.DisplayTransform`.
        
        :param enabled: looks override enabling
        :type enabled: object
        """
        pass
