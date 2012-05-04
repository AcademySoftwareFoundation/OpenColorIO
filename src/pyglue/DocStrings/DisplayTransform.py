
class DisplayTransform:
    """
    Used to create transforms for displays.
    """
    def __init__(self):
        pass

    def getInputColorSpaceName(self):
        """
        getInputColorSpaceName()
        
        Returns the name of the input ColorSpace of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: name of input ColorSpace
        :rtype: string
        """
        pass
        
    def setInputColorSpaceName(self, name):
        """
        setInputColorSpaceName(name)
        
        Sets the name of the input ColorSpace of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param name: name of input ColorSpace
        :type name: string
        """
        pass
        
    def getLinearCC(self):
        """
        getLinearCC()
        
        Returns the linear CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: linear CC transform
        :rtype: object
        """
        pass
        
    def setLinearCC(self, transform):
        """
        setLinearCC(pyCC)
        
        Sets the linear CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param pyCC: linear CC
        :type pyCC: object
        """
        pass
        
    def getColorTimingCC(self):
        """
        getColorTimingCC()
        
        Returns the color timing CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: color timing CC transform
        :rtype: object
        """
        pass
        
    def setColorTimingCC(self, transform):
        """
        setColorTimingCC(pyCC)
        
        Sets the color timing CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param pyCC: color timing CC
        :type pyCC: object
        """
        pass
        
    def getChannelView(self):
        """
        getChannelView()
        
        Returns the channel view of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: channel view
        :rtype: object
        """
        pass
        
    def setChannelView(self, transform):
        """
        setChannelView(pyCC)
        
        Sets the channel view transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param pyCC: channel view transform
        :type pyCC: object
        """
        pass
        
    def getDisplay(self):
        """
        getDisplay()
        
        Returns the display of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: display
        :rtype: string
        """
        pass
        
    def setDisplay(self, displayName):
        """
        setDisplay(str)
        
        Sets the display of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param str: display
        :type str: string
        """
        pass
        
    def getView(self):
        """
        getView()
        
        Returns the view of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: view
        :rtype: string
        """
        pass
        
    def setView(self, viewName):
        """
        setView(str)
        
        Sets the view of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param str: view
        :type str: string
        """
        pass
        
    def getDisplayCC(self):
        """
        getDisplayCC()
        
        Returns the display CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :return: display CC
        :rtype: object
        """
        pass
        
    def setDisplayCC(self, transform):
        """
        setDisplayCC(pyCC)
        
        Sets the display CC transform of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param pyCC: display CC
        :type pyCC: object
        """
        pass
        
    def getLooksOverride(self):
        """
        getLooksOverride()
        
        Returns the looks in :py:class:`PyOpenColorIO.DisplayTransform` that's
        overriding :py:class:`PyOpenColorIO.Config`'s.
        
        :return: looks override
        :rtype: string
        """
        pass
        
    def setLooksOverride(self, looksStr):
        """
        setLooksOverride(str)
        
        Sets the looks override of :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param str: looks override
        :type str: string
        """
        pass
        
    def getLooksOverrideEnabled(self):
        """
        getLooksOverrideEnabled()
        
        Returns whether the looks override of
        :py:class:`PyOpenColorIO.DisplayTransform` is enabled.
        
        :return: looks override enabling
        :rtype: bool
        """
        pass
        
    def setLooksOverrideEnabled(self, enabled):
        """
        setLooksOverrideEnabled(enabled)
        
        Sets the looks override enabling of
        :py:class:`PyOpenColorIO.DisplayTransform`.
        
        :param enabled: looks override enabling
        :type enabled: object
        """
        pass
