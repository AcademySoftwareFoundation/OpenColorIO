
class AllocationTransform:
    """
    Respans the 'expanded' range into the specified (often compressed) range.
    
    Performs both squeeze (offset) and log transforms.
    """
    def __init__(self):
        pass
    
    def getAllocation(self):
        """
        getAllocation()
        
        Returns the allocation specified in the transform. Allocation is an
        enum, defined in Constants.
        
        :return: Allocation
        :rtype: string
        """
        pass
        
    def setAllocation(self, hwalloc):
        """
        setAllocation(hwalloc)
        
        Sets the allocation of the transform.
        
        :param hwalloc: Allocation
        :type hwalloc: object
        """
        pass
    
    def getNumVars(self):
        pass
    
    def getVars(self):
        """
        getVars()
        
        Returns the allocation values specified in the transform.
        
        :return: allocation values
        :rtype: list of floats
        """
        pass
        
    def setVars(self, vars):
        """
        setVars(pyvars)
        
        Sets the allocation in the transform.
        
        :param pyvars: list of floats
        :type pyvars: object
        """
        pass
