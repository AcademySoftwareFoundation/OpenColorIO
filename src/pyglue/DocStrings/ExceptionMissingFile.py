
class ExceptionMissingFile:
    """
    An exception class for errors detected at runtime, thrown when OCIO cannot
    find a file that is expected to exist. This is provided as a custom type to
    distinguish cases where one wants to continue looking for missing files,
    but wants to properly fail for other error conditions.
    """
    def __init__(self):
        pass

