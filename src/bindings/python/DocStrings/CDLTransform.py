
class CDLTransform:
    """
    CDLTransform
    """
    def __init__(self):
        pass
    
    def CreateFromFile(self, src, cccid):
        pass
    
    def equals(self, cdl):
        pass
        
    def getXML(self):
        pass
        
    def setXML(self, xmltext):
        pass
        
    def getSlope(self):
        pass
        
    def getOffset(self):
        pass
        
    def getPower(self):
        pass
        
    def getSOP(self):
        pass
        
    def getSat(self):
        pass
        
    def setSlope(self, slope):
        """
        setSlope(pyData)
        
        Sets the slope ('S' part of SOP) in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param pyData: 
        :type pyData: object
        """
        pass
        
    def setOffset(self, offset):
        """
        setOffset(pyData)
        
        Sets the offset ('O' part of SOP) in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param pyData: list of three floats
        :type pyData: object
        """
        pass
        
    def setPower(self, power):
        """
        setPower(pyData)
        
        Sets the power ('P' part of SOP) in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param pyData: list of three floats
        :type pyData: object
        """
        pass

    def setSOP(self, sop):
        """
        setSOP(pyData)
        
        Sets SOP in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param pyData: list of nine floats
        :type pyData: object
        """
        pass
        
    def setSat(self, sat):
        """
        setSAT(pyData)
        
        Sets SAT (saturation) in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param pyData: saturation
        :type pyData: float
        """
        pass
        
    def getSatLumaCoefs(self):
        """
        getSatLumaCoefs(pyData)
        
        Returns the SAT (saturation) and luma coefficients in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :return: saturation and luma coefficients
        :rtype: list of floats
        """
        pass
        
    def getID(self):
        """
        getID()
        
        Returns the ID from :py:class:`PyOpenColorIO.CDLTransform`.
        
        :return: ID
        :rtype: string
        """
        pass
        
    def setID(self, id):
        """
        setID(str)
        
        Sets the ID in :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param str: ID
        :type str: string
        """
        pass
        
    def getDescription(self):
        """
        getDescription()
        
        Returns the description of :py:class:`PyOpenColorIO.CDLTransform`.
        
        :return: description
        :rtype: string
        """
        pass
        
    def setDescription(self, desc):
        """
        setDescription(str)
        
        Sets the description of :py:class:`PyOpenColorIO.CDLTransform`.
        
        :param str: description
        :type str: string
        """
        pass
