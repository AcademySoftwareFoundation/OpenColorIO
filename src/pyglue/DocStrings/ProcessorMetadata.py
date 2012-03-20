
class ProcessorMetadata:
    """
    ProcessorMetadata
    
    This contains meta information about the process that generated
    this processor.  The results of these functions do not
    impact the pixel processing.
    
    """
    def __init__(self):
        pass
    
    def getFiles(self):
        """
        getFiles()
        
        Returns a list of file references used internally by this processor
        
        :return: list of filenames
        :rtype: list
        """
        pass
    
    def getLooks(self):
        """
        getLooks()
        
        Returns a list of looks used internally by this processor
        
        :return: list of look names
        :rtype: list
        """
        pass
