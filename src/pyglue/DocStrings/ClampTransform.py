class ClampTransform:
    """
    Constrain a value to lie between min and max.  If min is greater than max
    no clamp is performed.  The inverse transform of clamp is a noop.
    """

    def __init__(self, min=[0.0, 0.0, 0.0, 0.0], max=[1.0, 1.0, 1.0, 1.0]):
        pass
    
    def getMin(self):
        pass
        
    def getMax(self):
        pass
        
    def setMin(self, min):
        """
        setMin(min)
        
        Sets the minimum clamp in :py:class:`PyOpenColorIO.ClampTransform`.
        
        :param min: list of four floats
        :type min: object
        """
        pass
        
    def setMax(self, max):
        """
        setMax(max)
        
        Sets the maximum clamp in :py:class:`PyOpenColorIO.ClampTransform`.
        
        :param max: list of four floats
        :type max: object
        """
        pass
