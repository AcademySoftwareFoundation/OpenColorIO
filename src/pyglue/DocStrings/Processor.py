
class Processor:
    """
    Processor
    """
    def __init__(self):
        pass
    
    def isNoOp(self):
        """
        isNoOp()
        
        Returns whether the actual transformation represented by
        :py:class:`PyOpenColorIO.Processor` is a no-op.
        
        :return: whether transform is a no-op
        :rtype: bool
        """
        pass
        
    def hasChannelCrosstalk(self):
        """
        hasChannelCrosstalk()
        
        Returns whether the transformation of
        :py:class:`PyOpenColorIO.Processor` introduces crosstalk between the
        image channels.
        
        :return: whether there's crosstalk between channels
        :rtype: bool
        """
        pass
        
    def applyRGB(self):
        """
        applyRGB(pyData)
        
        Apply the RGB part of the transform represented by
        :py:class:`PyOpenColorIO.Processor` to an image.
        
        :param pyData: 
        :type pyData: object
        :return: 
        :rtype: list
        """
        pass
        
    def applyRGBA(self):
        """
        applyRGBA(pyData)
        
        Apply the RGB and alpha part of the transform represented by
        :py:class:`PyOpenColorIO.Processor` to an image.
        
        :param pyData:
        :type pyData: object
        :return:
        :rtype: list
        """
        pass
        
    def getCpuCacheID(self):
        """
        getCpuCacheID()
        
        Returns the cache ID of the CPU that :py:class:`PyOpenColorIO.Processor`
        will run on.
        
        :return: CPU cache ID
        :rtype: string
        """
        pass
        
    def getGpuShaderText(self):
        """
        getGpuShaderText(pyData)
        
        Returns the GPU shader text.
        
        :param pyData: two params
        :type pyData: object
        :return: GPU shader text
        :rtype: string
        """
        pass
        
    def getGpuShaderTextCacheID(self):
        """
        getGpuShaderTextCacheID(pyData)
        
        Returns the GPU shader text cache ID.
        
        :param pyData: two params
        :type pyData: object
        :return: GPU shader text cache ID
        :rtype: string
        """
        pass
        
    def getGpuLut3D(self):
        """
        getGpuLut3D(pyData)
        
        Returns the GPU LUT 3D.
        
        :param pyData: two params?
        :type pyData: object
        :return: GPU LUT 3D
        :rtype: list
        """
        pass
        
    def getGpuLut3DCacheID(self):
        """
        getGpuLut3DCacheID(pyData)
        
        Returns the GPU 3D LUT cache ID.
        
        :param pyData: two params
        :type pyData: object
        :return: GPU 3D LUT cache ID
        :rtype: string
        """
        pass
