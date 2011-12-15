#-------------------------------------------------------------------------------
# OpenColorIO (color management) related Mari scripts
# coding: utf-8
# Copyright (c) 2011 The Foundry Visionmongers Ltd.  All Rights Reserved.
#-------------------------------------------------------------------------------

import mari, time, PythonQt, os, math

##############################################################################################

# Enable to output extended debugging messages.
VERBOSE_ENABLED = False

# Message type identifiers.
class MessageType:
    DEBUG   = 1
    INFO    = 2
    WARNING = 3
    ERROR   = 4

def printMessage(type, message):
    if type == MessageType.DEBUG:
        if VERBOSE_ENABLED:
            mari.app.log('[ OpenColorIO ] %s' % message)
    elif type == MessageType.INFO:
        mari.app.log('[ OpenColorIO ] %s' % message)
    elif type == MessageType.WARNING:
        mari.app.log('[ OpenColorIO ] [ WARNING ] %s' % message)
    elif type == MessageType.ERROR:
        mari.app.log('[ OpenColorIO ] [ ERROR ] %s' % message)

##############################################################################################

def configFileFilter():
    return 'OpenColorIO Configuration (*.ocio)'

#---------------------------------------------------------------------------------------------

def lutFileFilter():
    result  = 'All LUT Files (*.3dl *.ccc *.cc *.csp *.cub *.cube *.hdl *.m3d *.mga *.spi1d *.spi3d *.spimtx *.vf);;'
    result += 'Autodesk LUT (*.3dl);;'
    result += 'ASC CDL Color Correction Collection LUT (*.ccc);;'
    result += 'ASC CDL Color Correction LUT (*.cc);;'
    result += 'Cinespace LUT (*.csp);;'
    result += 'Truelight LUT (*.cub);;'
    result += 'Iridas LUT (*.cube);;'
    result += 'Houdini LUT (*.hdl);;'
    result += 'Pandora LUT (*.m3d *.mga);;'
    result += 'Imageworks LUT (*.spi1d *.spi3d *.spimtx);;'
    result += 'Inventor LUT (*.vf)'
    return result

##############################################################################################

# Make sure the OpenColorIO python bindings are okay.
try:
    import PyOpenColorIO
    printMessage(MessageType.INFO, 'Loaded Python bindings \'%s\' successfully' % PyOpenColorIO.__file__)
except ImportError, e:
    PyOpenColorIO = None
    printMessage(MessageType.ERROR, 'Failed to load Python bindings \'%s\'' % e)

LUT_FILE_LIST_RESET = mari.FileList(mari.FileList.TYPE_SINGLE_FILE)
LUT_FILE_LIST_RESET.setAcceptNonExisting(True)
LUT_FILE_LIST_RESET.setFilter(lutFileFilter())

CONFIG_FILE_LIST_RESET = mari.FileList(mari.FileList.TYPE_SINGLE_FILE)
if mari.app.isRunning():
    CONFIG_FILE_LIST_RESET.append(mari.resources.path(mari.resources.COLOR) + '/OpenColorIO/nuke-default/config.ocio')
else:
    CONFIG_FILE_LIST_RESET.append('/OpenColorIO/nuke-default/config.ocio')
CONFIG_FILE_LIST_RESET.setPickedFile(CONFIG_FILE_LIST_RESET.at(0))
CONFIG_FILE_LIST_RESET.setAcceptNonExisting(True)
CONFIG_FILE_LIST_RESET.setFilter(configFileFilter())

SQRT_TWO                 = math.sqrt(2.0)
LUT_SIZE_TYPES           = ['Small', 'Medium', 'Large']
LUT_SIZE_VALUES          = {'Small': 16,
                           'Medium': 32,
                            'Large': 64}
LUT_SIZE_RESET           = LUT_SIZE_TYPES[1]
ENABLED_RESET            = True
PROFILE_RESET            = 'Color Space'
LUT_EXTRAPOLATE_RESET    = True
COLOR_SPACE_RESET        = 'sRGB'
DISPLAY_RESET            = 'default'
VIEW_RESET               = 'sRGB'
SWIZZLE_TYPES            = ['Luminance', 'RGB', 'R', 'G', 'B', 'A']
SWIZZLE_VALUES           = {'Luminance': ( True,  True,  True, False),
                                  'RGB': ( True,  True,  True,  True),
                                    'R': ( True, False, False, False),
                                    'G': (False,  True, False, False),
                                    'B': (False, False,  True, False),
                                    'A': (False, False, False,  True)}
SWIZZLE_RESET            = SWIZZLE_TYPES[1]
FSTOP_STEP_SIZE          = 0.5
FSTOP_CENTER_MIN         = 1.0
FSTOP_CENTER_MAX         = 64.0
FSTOP_CENTER_STEP_SIZE   = 0.001
FSTOP_CENTER_RESET       = 8.0
EXPOSURE_MIN             = -6.0
EXPOSURE_MAX             = +6.0
EXPOSURE_DELTA           = EXPOSURE_MAX - EXPOSURE_MIN
EXPOSURE_STEP_SIZE       = 0.1
GAIN_RESET               = 1.0
GAIN_MIN                 = 2.0 ** EXPOSURE_MIN
GAIN_MAX                 = 2.0 ** EXPOSURE_MAX
GAIN_STEP_SIZE           = 0.000001
GAIN_PRECISION           = 6
GAMMA_RESET              = 1.0
GAMMA_MIN                = 0.0
GAMMA_MAX                = 4.0
GAMMA_STEP_SIZE          = 0.01
GAMMA_PRECISION          = 2

enabled_default          = ENABLED_RESET
profile_default          = PROFILE_RESET
lut_file_list_default    = mari.FileList(LUT_FILE_LIST_RESET)
lut_extrapolate_default  = LUT_EXTRAPOLATE_RESET
config_file_list_default = mari.FileList(CONFIG_FILE_LIST_RESET)
color_space_default      = COLOR_SPACE_RESET
display_default          = DISPLAY_RESET
view_default             = VIEW_RESET
swizzle_default          = SWIZZLE_RESET
gain_default             = GAIN_RESET
gamma_default            = GAMMA_RESET
lut_size                 = LUT_SIZE_RESET
fstop_center             = FSTOP_CENTER_RESET

config_default           = None

lut_size_functions       = []
fstop_center_functions   = []

##############################################################################################

def registerLUTSizeChanged(function):
    global lut_size_functions
    lut_size_functions.append(function)

#---------------------------------------------------------------------------------------------

def registerFStopCenterChanged(function):
    global fstop_center_functions
    fstop_center_functions.append(function)

#---------------------------------------------------------------------------------------------

def _enabledDefaultChanged():
    global enabled_default
    enabled_default = mari.prefs.get('/Color/Color Management Defaults/colorEnabledDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _profileDefaultChanged():
    global profile_default
    profile_default = mari.prefs.get('/Color/Color Management Defaults/colorProfileDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _postFilterCollectionAdded(filter_collection):
    mari.prefs.setItemList('/Color/Color Management Defaults/colorProfileDefault', mari.gl_render.postFilterCollectionNames())

#---------------------------------------------------------------------------------------------

def _postFilterCollectionRemoved(name):
    collection_names = mari.gl_render.postFilterCollectionNames()
    mari.prefs.setItemList('/Color/Color Management Defaults/colorProfileDefault', collection_names)

    global PROFILE_RESET
    if name == PROFILE_RESET:
        PROFILE_RESET = collection_names[0]
        mari.prefs.setDefault('/Color/Color Management Defaults/colorProfileDefault', PROFILE_RESET)

    global profile_default
    if name == profile_default:
        profile_default = PROFILE_RESET
        mari.prefs.set('/Color/Color Management Defaults/colorProfileDefault', profile_default)
        _savePreferences()

#---------------------------------------------------------------------------------------------

def _lutPathDefaultChanged():
    global lut_file_list_default
    lut_file_list_default = mari.FileList(mari.prefs.get('/Color/LUT Defaults/lutPathDefault'))
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _lutExtrapolateDefaultChanged():
    global lut_extrapolate_default
    lut_extrapolate_default = mari.prefs.get('/Color/LUT Defaults/lutExtrapolateDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _configPathDefaultChanged():
    global config_file_list_default

    # Only replace the existing configuration file if the new one is valid!
    config_file_list = mari.FileList(mari.prefs.get('/Color/Display Defaults/displayConfigPathDefault'))

    if not config_file_list.isEmpty():
        config = loadConfig(config_file_list.at(0), True)
        if config is not None:
            config_file_list_default = config_file_list

            global config_default
            config_default = config

            _updateColorSpaceDefault()
            _updateDisplayDefault()
            _updateViewDefault()

            _savePreferences()

        else:
            # Put back the existing configuration file that works...
            mari.prefs.set('/Color/Display Defaults/displayConfigPathDefault', config_file_list_default)
    else:
        # Put back the existing configuration file that works...
        mari.prefs.set('/Color/Display Defaults/displayConfigPathDefault', config_file_list_default)

#---------------------------------------------------------------------------------------------

def _colorSpaceDefaultChanged():
    global color_space_default
    color_space_default = mari.prefs.get('/Color/Display Defaults/displayColorSpaceDefault')
    _updateDisplayDefault()
    _updateViewDefault()
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _displayDefaultChanged():
    global display_default
    display_default = mari.prefs.get('/Color/Display Defaults/displayDisplayDefault')
    _updateViewDefault()
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _viewDefaultChanged():
    global view_default
    view_default = mari.prefs.get('/Color/Display Defaults/displayViewDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _swizzleDefaultChanged():
    global swizzle_default
    swizzle_default = mari.prefs.get('/Color/Display Defaults/displaySwizzleDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _gainDefaultChanged():
    global gain_default
    gain_default = mari.prefs.get('/Color/Display Defaults/displayGainDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _gammaDefaultChanged():
    global gamma_default
    gamma_default = mari.prefs.get('/Color/Display Defaults/displayGammaDefault')
    _savePreferences()

#---------------------------------------------------------------------------------------------

def _lutSizeChanged():
    global lut_size
    global lut_size_functions
    lut_size = mari.prefs.get('/Color/Display General/displayLutSize')
    _savePreferences()
    for function in lut_size_functions:
        function()

#---------------------------------------------------------------------------------------------

def _fstopCenterChanged():
    global fstop_center
    global fstop_center_functions
    fstop_center = mari.prefs.get('/Color/Display General/displayFStopCenter')
    _savePreferences()
    for function in fstop_center_functions:
        function()

#---------------------------------------------------------------------------------------------

def _registerPreferences():
    global enabled_default
    mari.prefs.set('/Color/Color Management Defaults/colorEnabledDefault', enabled_default)
    mari.prefs.setChangedScript('/Color/Color Management Defaults/colorEnabledDefault', 'mari.utils.ocio._enabledDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Color Management Defaults/colorEnabledDefault', 'Enabled')
    mari.prefs.setDefault('/Color/Color Management Defaults/colorEnabledDefault', ENABLED_RESET)

    global profile_default
    mari.prefs.set('/Color/Color Management Defaults/colorProfileDefault', profile_default)
    mari.prefs.setChangedScript('/Color/Color Management Defaults/colorProfileDefault', 'mari.utils.ocio._profileDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Color Management Defaults/colorProfileDefault', 'Color Profile')
    mari.prefs.setDefault('/Color/Color Management Defaults/colorProfileDefault', PROFILE_RESET)
    mari.prefs.setItemList('/Color/Color Management Defaults/colorProfileDefault', mari.gl_render.postFilterCollectionNames())

    global lut_file_list_default
    if not lut_file_list_default.isEmpty() and not os.path.isfile(lut_file_list_default.at(0)):
        message = 'LUT file \'%s\' does not exist' % lut_file_list_default.at(0)
        printMessage(MessageType.ERROR, '%s' % message)
        lut_file_list_default = mari.FileList(LUT_FILE_LIST_RESET)

    mari.prefs.set('/Color/LUT Defaults/lutPathDefault', lut_file_list_default)
    mari.prefs.setChangedScript('/Color/LUT Defaults/lutPathDefault', 'mari.utils.ocio._lutPathDefaultChanged()')
    mari.prefs.setDisplayName('/Color/LUT Defaults/lutPathDefault', 'File')
    mari.prefs.setDefault('/Color/LUT Defaults/lutPathDefault', LUT_FILE_LIST_RESET)

    global lut_extrapolate_default
    mari.prefs.set('/Color/LUT Defaults/lutExtrapolateDefault', lut_extrapolate_default)
    mari.prefs.setChangedScript('/Color/LUT Defaults/lutExtrapolateDefault', 'mari.utils.ocio._lutExtrapolateDefaultChanged()')
    mari.prefs.setDisplayName('/Color/LUT Defaults/lutExtrapolateDefault', 'Extrapolate')
    mari.prefs.setDefault('/Color/LUT Defaults/lutExtrapolateDefault', LUT_EXTRAPOLATE_RESET)

    global config_file_list_default
    global config_default
    if not config_file_list_default.isEmpty():
        config = loadConfig(config_file_list_default.at(0), False)
        if config is not None:
            config_default = config
        else:
            config_file_list_default = mari.FileList(CONFIG_FILE_LIST_RESET)
    else:
        config_file_list_default = mari.FileList(CONFIG_FILE_LIST_RESET)

    mari.prefs.set('/Color/Display Defaults/displayConfigPathDefault', config_file_list_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayConfigPathDefault', 'mari.utils.ocio._configPathDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayConfigPathDefault', 'Configuration File')
    mari.prefs.setDefault('/Color/Display Defaults/displayConfigPathDefault', CONFIG_FILE_LIST_RESET)

    color_spaces = [color_space.getName() for color_space in config_default.getColorSpaces()]

    color_space_reset = COLOR_SPACE_RESET
    if color_spaces.count(color_space_reset) == 0:
        color_space_reset = color_spaces[0]

    global color_space_default
    if color_spaces.count(color_space_default) == 0:
        color_space_default = color_space_reset

    mari.prefs.set('/Color/Display Defaults/displayColorSpaceDefault', color_space_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayColorSpaceDefault', 'mari.utils.ocio._colorSpaceDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayColorSpaceDefault', 'Input Color Space')
    mari.prefs.setDefault('/Color/Display Defaults/displayColorSpaceDefault', color_space_reset)
    mari.prefs.setItemList('/Color/Display Defaults/displayColorSpaceDefault', color_spaces)

    displays = config_default.getDisplays()

    display_reset = DISPLAY_RESET
    if displays.count(display_reset) == 0:
        display_reset = config_default.getDefaultDisplay()

    global display_default
    if displays.count(display_default) == 0:
        display_default = display_reset

    mari.prefs.set('/Color/Display Defaults/displayDisplayDefault', display_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayDisplayDefault', 'mari.utils.ocio._displayDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayDisplayDefault', 'Display')
    mari.prefs.setDefault('/Color/Display Defaults/displayDisplayDefault', display_reset)
    mari.prefs.setItemList('/Color/Display Defaults/displayDisplayDefault', displays)

    views = config_default.getViews(display_default)

    view_reset = VIEW_RESET
    if views.count(view_reset) == 0:
        view_reset = config_default.getDefaultView(display_default)

    global view_default
    if views.count(view_default) == 0:
        view_default = view_reset

    mari.prefs.set('/Color/Display Defaults/displayViewDefault', view_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayViewDefault', 'mari.utils.ocio._viewDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayViewDefault', 'View')
    mari.prefs.setDefault('/Color/Display Defaults/displayViewDefault', view_reset)
    mari.prefs.setItemList('/Color/Display Defaults/displayViewDefault', views)

    global swizzle_default
    if SWIZZLE_TYPES.count(swizzle_default) == 0:
        swizzle_default = SWIZZLE_RESET

    mari.prefs.set('/Color/Display Defaults/displaySwizzleDefault', swizzle_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displaySwizzleDefault', 'mari.utils.ocio._swizzleDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displaySwizzleDefault', 'Component')
    mari.prefs.setDefault('/Color/Display Defaults/displaySwizzleDefault', SWIZZLE_RESET)
    mari.prefs.setItemList('/Color/Display Defaults/displaySwizzleDefault', SWIZZLE_TYPES)

    global gain_default
    mari.prefs.set('/Color/Display Defaults/displayGainDefault', gain_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayGainDefault', 'mari.utils.ocio._gainDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayGainDefault', 'Gain')
    mari.prefs.setDefault('/Color/Display Defaults/displayGainDefault', GAIN_RESET)
    mari.prefs.setRange('/Color/Display Defaults/displayGainDefault', GAIN_MIN, GAIN_MAX)
    mari.prefs.setStep('/Color/Display Defaults/displayGainDefault', GAIN_STEP_SIZE)

    global gamma_default
    mari.prefs.set('/Color/Display Defaults/displayGammaDefault', gamma_default)
    mari.prefs.setChangedScript('/Color/Display Defaults/displayGammaDefault', 'mari.utils.ocio._gammaDefaultChanged()')
    mari.prefs.setDisplayName('/Color/Display Defaults/displayGammaDefault', 'Gamma')
    mari.prefs.setDefault('/Color/Display Defaults/displayGammaDefault', GAMMA_RESET)
    mari.prefs.setRange('/Color/Display Defaults/displayGammaDefault', GAMMA_MIN, GAMMA_MAX)
    mari.prefs.setStep('/Color/Display Defaults/displayGammaDefault', GAMMA_STEP_SIZE)

    global lut_size
    mari.prefs.set('/Color/Display General/displayLutSize', lut_size)
    mari.prefs.setChangedScript('/Color/Display General/displayLutSize', 'mari.utils.ocio._lutSizeChanged()')
    mari.prefs.setDisplayName('/Color/Display General/displayLutSize', 'LUT Size')
    mari.prefs.setDefault('/Color/Display General/displayLutSize', LUT_SIZE_RESET)
    mari.prefs.setItemList('/Color/Display General/displayLutSize', LUT_SIZE_TYPES)

    global fstop_center
    mari.prefs.set('/Color/Display General/displayFStopCenter', fstop_center)
    mari.prefs.setChangedScript('/Color/Display General/displayFStopCenter', 'mari.utils.ocio._fstopCenterChanged()')
    mari.prefs.setDisplayName('/Color/Display General/displayFStopCenter', 'Center F-Stop')
    mari.prefs.setDefault('/Color/Display General/displayFStopCenter', FSTOP_CENTER_RESET)
    mari.prefs.setRange('/Color/Display General/displayFStopCenter', FSTOP_CENTER_MIN, FSTOP_CENTER_MAX)
    mari.prefs.setStep('/Color/Display General/displayFStopCenter', FSTOP_CENTER_STEP_SIZE)

    # Attach ourselves to the appropriate project signals so we can update widgets.
    PythonQt.QtCore.QObject.connect(mari.gl_render.postFilterCollectionAdded.__self__,
                                    mari.gl_render.postFilterCollectionAdded.__name__,
                                    _postFilterCollectionAdded)
    PythonQt.QtCore.QObject.connect(mari.gl_render.postFilterCollectionRemoved.__self__,
                                    mari.gl_render.postFilterCollectionRemoved.__name__,
                                    _postFilterCollectionRemoved)

#---------------------------------------------------------------------------------------------

def _updateColorSpaceDefault():
    global config_default
    global color_space_default

    color_spaces = [color_space.getName() for color_space in config_default.getColorSpaces()]

    color_space_reset = COLOR_SPACE_RESET
    if color_spaces.count(color_space_reset) == 0:
        color_space_reset = color_spaces[0]

    if color_spaces.count(color_space_default) == 0:
        color_space_default = color_space_reset

    mari.prefs.setItemList('/Color/Display Defaults/displayColorSpaceDefault', color_spaces)
    mari.prefs.set('/Color/Display Defaults/displayColorSpaceDefault', color_space_default)
    mari.prefs.setDefault('/Color/Display Defaults/displayColorSpaceDefault', color_space_reset)


#---------------------------------------------------------------------------------------------

def _updateDisplayDefault():
    global config_default
    global display_default

    displays = config_default.getDisplays()

    display_reset = DISPLAY_RESET
    if displays.count(display_reset) == 0:
        display_reset = config_default.getDefaultDisplay()

    if displays.count(display_default) == 0:
        display_default = display_reset

    mari.prefs.setItemList('/Color/Display Defaults/displayDisplayDefault', displays)
    mari.prefs.set('/Color/Display Defaults/displayDisplayDefault', display_default)
    mari.prefs.setDefault('/Color/Display Defaults/displayDisplayDefault', display_reset)

#---------------------------------------------------------------------------------------------

def _updateViewDefault():
    global config_default
    global display_default
    global view_default

    views = config_default.getViews(display_default)

    view_reset = VIEW_RESET
    if views.count(view_reset) == 0:
        view_reset = config_default.getDefaultView(display_default)

    if views.count(view_default) == 0:
        view_default = view_reset

    mari.prefs.setItemList('/Color/Display Defaults/displayViewDefault', views)
    mari.prefs.set('/Color/Display Defaults/displayViewDefault', view_default)
    mari.prefs.setDefault('/Color/Display Defaults/displayViewDefault', view_reset)

#---------------------------------------------------------------------------------------------

def _loadPreferences():
    settings = mari.Settings()
    settings.beginGroup('OpenColorIO')

    try:
        global enabled_default
        global profile_default
        global lut_file_list_default
        global lut_extrapolate_default
        global config_file_list_default
        global color_space_default
        global display_default
        global view_default
        global swizzle_default
        global gain_default
        global gamma_default
        global lut_size
        global fstop_center

        enabled_default         =         False if int(settings.value('enabledDefault', ENABLED_RESET)) == 0 else True
        profile_default         =                  str(settings.value('profileDefault', PROFILE_RESET))
        lut_path                =    buildLoadPath(str(settings.value('lutPathDefault', '' if LUT_FILE_LIST_RESET.isEmpty() else LUT_FILE_LIST_RESET.at(0))))
        lut_extrapolate_default =  False if int(settings.value('lutExtrapolateDefault', LUT_EXTRAPOLATE_RESET)) == 0 else True
        config_path             = buildLoadPath(str(settings.value('configPathDefault', '' if CONFIG_FILE_LIST_RESET.isEmpty() else CONFIG_FILE_LIST_RESET.at(0))))
        color_space_default     =               str(settings.value('colorSpaceDefault', COLOR_SPACE_RESET))
        display_default         =                  str(settings.value('displayDefault', DISPLAY_RESET))
        view_default            =                     str(settings.value('viewDefault', VIEW_RESET))
        swizzle_default         =                  str(settings.value('swizzleDefault', SWIZZLE_RESET))
        gain_default            =           max(min(float(settings.value('gainDefault', GAIN_RESET)), GAIN_MAX), GAIN_MIN)
        gamma_default           =          max(min(float(settings.value('gammaDefault', GAMMA_RESET)), GAMMA_MAX), GAMMA_MIN)
        lut_size                =                         str(settings.value('lutSize', LUT_SIZE_RESET))
        fstop_center            =           max(min(float(settings.value('fstopCenter', FSTOP_CENTER_RESET)), FSTOP_CENTER_MAX), FSTOP_CENTER_MIN)

        if os.path.isfile(lut_path):
            lut_file_list_default.clear()
            lut_file_list_default.append(lut_path)
            lut_file_list_default.setPickedFile(lut_path)

        if os.path.isfile(config_path):
            config_file_list_default.clear()
            config_file_list_default.append(config_path)
            config_file_list_default.setPickedFile(config_path)

    except ValueError, e:
        printMessage(MessageType.ERROR, 'Failed to load preferences \'%s\'' % e)

    settings.endGroup()

    _printPreferences(MessageType.DEBUG, 'Loaded Preferences:')

#---------------------------------------------------------------------------------------------

def _savePreferences():
    settings = mari.Settings()
    settings.beginGroup('OpenColorIO')

    global enabled_default
    global profile_default
    global lut_file_list_default
    global lut_extrapolate_default
    global config_file_list_default
    global color_space_default
    global display_default
    global view_default
    global swizzle_default
    global gain_default
    global gamma_default
    global lut_size
    global fstop_center

    settings.setValue(       'enabledDefault', 1 if enabled_default else 0)
    settings.setValue(       'profileDefault', profile_default)
    settings.setValue(       'lutPathDefault', '' if lut_file_list_default.isEmpty() else buildSavePath(lut_file_list_default.at(0)))
    settings.setValue('lutExtrapolateDefault', 1 if lut_extrapolate_default else 0)
    settings.setValue(    'configPathDefault', '' if config_file_list_default.isEmpty() else buildSavePath(config_file_list_default.at(0)))
    settings.setValue(    'colorSpaceDefault', color_space_default)
    settings.setValue(       'displayDefault', display_default)
    settings.setValue(          'viewDefault', view_default)
    settings.setValue(       'swizzleDefault', swizzle_default)
    settings.setValue(          'gainDefault', gain_default)
    settings.setValue(         'gammaDefault', gamma_default)
    settings.setValue(              'lutSize', lut_size)
    settings.setValue(          'fstopCenter', fstop_center)

    settings.endGroup()

    _printPreferences(MessageType.DEBUG, 'Saved Preferences:')

#---------------------------------------------------------------------------------------------

def _printPreferences(type, title):
    global enabled_default
    global profile_default
    global lut_file_list_default
    global lut_extrapolate_default
    global config_file_list_default
    global color_space_default
    global display_default
    global view_default
    global swizzle_default
    global gain_default
    global fstop_center
    global lut_size
    global gamma_default

    printMessage(type, '==============================================================')
    printMessage(type, title)
    printMessage(type, '==============================================================')
    printMessage(type, '     Enabled: %s'             % enabled_default)
    printMessage(type, '     Profile: %s'             % profile_default)
    printMessage(type, '    LUT Path: %s'             % '' if lut_file_list_default.isEmpty() else lut_file_list_default.at(0))
    printMessage(type, ' Extrapolate: %s'             % lut_extrapolate_default)
    printMessage(type, ' Config Path: %s'             % '' if config_file_list_default.isEmpty() else config_file_list_default.at(0))
    printMessage(type, ' Color Space: %s'             % color_space_default)
    printMessage(type, '     Display: %s'             % display_default)
    printMessage(type, '        View: %s'             % view_default)
    printMessage(type, '     Swizzle: %s'             % swizzle_default)
    printMessage(type, '      F-Stop: %f; Center: %f' % (convertGainToFStop(gain_default), fstop_center))
    printMessage(type, '        Gain: %f'             % gain_default)
    printMessage(type, '       Gamma: %f'             % gamma_default)
    printMessage(type, '    LUT Size: %s'             % lut_size)
    printMessage(type, '==============================================================')

##############################################################################################

def convertExposureToGain(exposure):
    return 2.0 ** exposure

#---------------------------------------------------------------------------------------------

def convertGainToExposure(gain):
    return math.log(gain, 2.0)

#---------------------------------------------------------------------------------------------

def convertExposureToFStop(exposure):
    global fstop_center
    exposure_center = math.log(fstop_center, SQRT_TWO)
    return math.pow(SQRT_TWO, exposure_center - exposure)

#---------------------------------------------------------------------------------------------

def convertGainToFStop(gain):
    exposure = convertGainToExposure(gain)
    return convertExposureToFStop(exposure)

#---------------------------------------------------------------------------------------------

def buildProcessorFilter(processor, filter, filter_cache_id, texture_cache_id, extrapolate = False, force_shader_build = False):
    # Create a name, using the filter's name, that can be used in uniquely naming parameters and functions.
    name = filter.name();
    name = name.lower()
    name = name.replace(' ', '_')

    sampler_name  = 'ocio_' + name + '_lut_$ID_'
    function_name = 'ocio_' + name + '_$ID_'

    global lut_size
    shader_desc = {    'language': PyOpenColorIO.Constants.GPU_LANGUAGE_GLSL_1_3,
                   'functionName': function_name,
                   'lut3DEdgeLen': LUT_SIZE_VALUES[lut_size]}

    cache_id = processor.getGpuShaderTextCacheID(shader_desc)
    if cache_id != filter_cache_id or force_shader_build:
        filter_cache_id = cache_id
        printMessage(MessageType.DEBUG, 'Creating new GLSL filter...')

        desc  = 'uniform sampler3D ' + sampler_name + ';\n'
        desc += processor.getGpuShaderText(shader_desc)
        body  = ''
        if extrapolate:
            # The following code was taken from Nuke's 'LUT3D::Extrapolate' functionality. It attempts to estimate what
            # the corresponding color value would be when the incoming color value is outside the normal range of [0-1],
            # such as the case with HDR images.
            rcp_lut_edge_length = 1.0 / float(LUT_SIZE_VALUES[lut_size])

            body += '{\n'
            body += '    if( 1.0 < Out.r || 1.0 < Out.g || 1.0 < Out.b )\n'
            body += '    {\n'
            body += '        vec4 closest;\n'
            body += '        closest.rgb = clamp(Out.rgb, vec3(0.0), vec3(1.0));\n'
            body += '        closest.a = Out.a;\n'
            body += '\n'
            body += '        vec3 offset = Out.rgb - closest.rgb;\n'
            body += '        float offset_distance = length(offset);\n'
            body += '        offset = normalize(offset);\n'
            body += '\n'
            body += '        vec4 nbr_position;\n'
            body += '        nbr_position.rgb = closest.rgb - %f * offset;\n' % rcp_lut_edge_length
            body += '        nbr_position.a = Out.a;\n'
            body += '\n'
            body += '        Out = ' + function_name + '(closest, ' + sampler_name + ');\n'
            body += '        Out.rgb += (Out.rgb - ' + function_name + '(nbr_position, ' + sampler_name + ').rgb) / %f * offset_distance;\n' % rcp_lut_edge_length
            body += '    }\n'
            body += '    else\n'
            body += '    {\n'
            body += '        Out = ' + function_name + '(Out, ' + sampler_name + ');\n'
            body += '    }\n'
            body += '}\n'
        else:
            body += '{ Out = ' + function_name + '(Out, ' + sampler_name + '); }\n'

        filter.setDefinitionsSnippet(desc)
        filter.setBodySnippet(body)
    else:
        printMessage(MessageType.DEBUG, 'No GLSL filter update required')

    cache_id = processor.getGpuLut3DCacheID(shader_desc)
    if cache_id != texture_cache_id:
        lut = processor.getGpuLut3D(shader_desc)

        printMessage(MessageType.DEBUG, 'Updating LUT...')

        if texture_cache_id is None:
            filter.setTexture3D(sampler_name,
                                LUT_SIZE_VALUES[lut_size],
                                LUT_SIZE_VALUES[lut_size],
                                LUT_SIZE_VALUES[lut_size],
                                filter.FORMAT_RGB,
                                lut)
        else:
            filter.updateTexture3D(sampler_name, lut)

        texture_cache_id = cache_id
    else:
        printMessage(MessageType.DEBUG, 'No LUT update required')

    return (filter_cache_id, texture_cache_id, sampler_name)

#---------------------------------------------------------------------------------------------

def buildLUTFilter(config, path, filter, filter_cache_id, texture_cache_id, extrapolate, force_shader_build = False):
    file_transform = PyOpenColorIO.FileTransform()
    file_transform.setSrc(path)
    file_transform.setInterpolation('linear')

    processor = config.getProcessor(file_transform)

    return buildProcessorFilter(processor, filter, filter_cache_id, texture_cache_id, extrapolate, force_shader_build)

#---------------------------------------------------------------------------------------------

def loadConfig(path, display_message_box = True):
    try:
        config = PyOpenColorIO.Config.CreateFromFile(path)
        return config

    except Exception, e:
        message = 'Failed to load configuration file \'%s\' due to \'%s\'' % (path, e)
        printMessage(MessageType.ERROR, '%s' % message)
        if display_message_box and not mari.app.inTerminalMode():
            mari.utils.misc.message(message, 'Color Space', 1024, 2)

        return None

#---------------------------------------------------------------------------------------------

# This converts a path into a form which can be shared among different platforms and installations.
def buildSavePath(path):
    result = path.replace(mari.resources.path(mari.resources.COLOR), '$MARI_COLOR_PATH', 1)
    return result

#---------------------------------------------------------------------------------------------

# This converts a path saved out to disk back into a form which can used by the application.
def buildLoadPath(path):
    result = path.replace('$MARI_COLOR_PATH', mari.resources.path(mari.resources.COLOR), 1)
    return result

##############################################################################################

if mari.app.isRunning():
    if PyOpenColorIO is not None:
        # Attempt to load the default configuration file... without it we can't do nothing!
        config_file_lists = [config_file_list_default, CONFIG_FILE_LIST_RESET]
        for config_file_list in config_file_lists:
            if not config_file_list.isEmpty():
                config_default = loadConfig(config_file_list.at(0), False)
                if config_default is not None:
                    config_file_list_default = mari.FileList(config_file_list)
                    break

        if config_default is not None:
            _loadPreferences()
            _registerPreferences()
            _savePreferences()

        else:
            message = 'Failed to find a working configuration file. OpenColorIO will be disabled!'
            printMessage(MessageType.ERROR, message)
            if not mari.app.inTerminalMode():
                mari.utils.misc.message(message, 'OpenColorIO', 1024, 3)
