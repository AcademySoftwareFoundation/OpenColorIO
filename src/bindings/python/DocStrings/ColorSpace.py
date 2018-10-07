
class ColorSpace:
    """
    A color space is the state of an image in terms of colorimetry and color
    encoding. I.e., it defines how an image's color information needs to be
    interpreted.
    
    Transforming images between different color spaces is the primary
    motivation for the OCIO library.
    
    While a complete discussion of color spaces is beyond the scope of this
    documentation, traditional uses would be to have color spaces describing
    image capture devices, such as cameras and scanners, and internal
    'convenience' spaces, such as scene-linear and logarithmic.
    
    Color spaces are specific to a particular image precision
    (float32, uint8, etc.). The set of color spaces that provide equivalent
    mappings (at different precisions) are referred to as a 'family'.
    
    .. code-block:: python
        
        import PyOpenColorIO as OCIO
        config = OCIO.Config()
    
    """
    def __init__(self):
        pass
        
    def isEditable(self):
        pass
        
    def createEditableCopy(self):
        pass
        
    def getName(self):
        pass
        
    def setName(self, name):
        pass
        
    def getFamily(self):
        pass
        
    def setFamily(self, family):
        pass
        
    def getEqualityGroup(self):
        pass
        
    def setEqualityGroup(self, equalityGroup):
        pass
        
    def getDescription(self):
        pass
        
    def setDescription(self, desc):
        pass
        
    def getBitDepth(self):
        pass
        
    def setBitDepth(self, bitDepth):
        pass
        
    def isData(self):
        """
        ColorSpaces that are data are treated a bit special. Basically, any
        colorspace transforms you try to apply to them are ignored. (Think
        of applying a gamut mapping transform to an ID pass). Also, the
        :py:class:`PyOpenColorIO.DisplayTransform` process obeys special
        'data min' and 'data max' args.
        
        This is traditionally used for pixel data that represents non-color
        pixel data, such as normals, point positions, ID information, etc.
        """
        pass
        
    def setIsData(self, isData):
        pass
        
    def getAllocation(self):
        """
        If this colorspace needs to be transferred to a limited dynamic
        range coding space (such as during display with a GPU path), use this
        allocation to maximize bit efficiency.
        """
        pass
        
    def setAllocation(self, allocation):
        pass
        
    def getAllocationVars(self):
        pass
        
    def setAllocationVars(self, vars):
        pass
        
    def getTransform(self):
        pass
        
    def setTransform(self, transform, direction):
        pass
        
