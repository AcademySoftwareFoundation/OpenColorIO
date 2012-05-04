
class Processor:
    """
    Processor is the baked representation of a particular color transform.
    Once you have a process for a particular transform created, you can hang
    onto it to efficiently transform pixels.
    
    Processors can only be created from the `PyOpenColorIO.Config`
    getProcessor(...) call.
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
    
    def getMetadata(self):
        """
        getMetadata()
        
        Returns information about the process that generated this processor. 
        
        :return: processor metadata
        :rtype: `PyOpenColorIO.ProcessorMetadata`
        """
        pass
    
    def applyRGB(self, pixeldata):
        """
        applyRGB(pixeldata)
        
        Apply the RGB part of the transform represented by
        :py:class:`PyOpenColorIO.Processor` to an image.
        
        :param pixeldata: rgbrgb... array (length % 3 == 0)
        :type pixeldata: object
        :return: color converted pixeldata
        :rtype: list
        """
        pass
        
    def applyRGBA(self, pixeldata):
        """
        applyRGBA(pixeldata)
        
        Apply the RGB and alpha part of the transform represented by
        :py:class:`PyOpenColorIO.Processor` to an image.
        
        :param pixeldata: rgbargba... array (length % 4 == 0)
        :type pixeldata: object
        :return: color converted pixeldata
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
        
    def getGpuShaderText(self, shaderDesc):
        """
        getGpuShaderText(shaderDesc)
        
        Returns the GPU shader text.
        
        :param shaderDesc: define 'language','functionName','lut3DEdgeLen'
        :type shaderDesc: dict
        :return: GPU shader text
        :rtype: string
        """
        pass
        
    def getGpuShaderTextCacheID(self, shaderDesc):
        """
        getGpuShaderTextCacheID(shaderDesc)
        
        Returns the GPU shader text cache ID.
        
        :param shaderDesc: define 'language','functionName','lut3DEdgeLen'
        :type shaderDesc: dict
        :return: GPU shader text cache ID
        :rtype: string
        """
        pass
        
    def getGpuLut3D(self, shaderDesc):
        """
        getGpuLut3D(shaderDesc)
        
        Returns the GPU LUT 3D.
        
        :param shaderDesc: define 'language','functionName','lut3DEdgeLen'
        :type shaderDesc: dict
        :return: GPU LUT 3D
        :rtype: list
        """
        pass
        
    def getGpuLut3DCacheID(self, shaderDesc):
        """
        getGpuLut3DCacheID(shaderDesc)
        
        Returns the GPU 3D LUT cache ID.
        
        :param shaderDesc: two params
        :type shaderDesc: dict
        :return: GPU 3D LUT cache ID
        :rtype: string
        """
        pass
