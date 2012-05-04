#-------------------------------------------------------------------------------
# Post processing (color management) related Mari scripts
# coding: utf-8
# Copyright (c) 2011 The Foundry Visionmongers Ltd.  All Rights Reserved.
#-------------------------------------------------------------------------------

import mari, time, PythonQt, os, math
QtGui  = PythonQt.QtGui
QtCore = PythonQt.QtCore
ocio   = mari.utils.ocio

##############################################################################################

GAIN_GROUP_MAX_WIDTH = 312
FSTOP_MAX_WIDTH      = 50
EXPOSURE_MAX_WIDTH   = 102
GAIN_MAX_WIDTH       = 80
GAMMA_MAX_WIDTH      = 200
TOOLBAR_SPACING      = 3

toolbar              = None

class OcioToolBar():

    #-----------------------------------------------------------------------------------------

    def __init__(self):
        # Default all members...
        self._config_file_list         = mari.FileList(ocio.config_file_list_default)
        self._config                   = ocio.config_default
        self._lut_file_list            = mari.FileList(ocio.lut_file_list_default)
        self._lut_extrapolate          = ocio.lut_extrapolate_default
        self._color_space              = ocio.color_space_default
        self._display                  = ocio.display_default
        self._view                     = ocio.view_default
        self._swizzle                  = ocio.swizzle_default
        self._gain                     = ocio.gain_default
        self._gamma                    = ocio.gamma_default

        self._lut_filter               = None
        self._lut_filter_cache_id      = None
        self._lut_texture_cache_id     = None
        self._lut_sampler_name         = None

        self._display_filter           = None
        self._display_filter_cache_id  = None
        self._display_texture_cache_id = None
        self._display_sampler_name     = None

        self._lut_extrapolate_widget   = None
        self._color_space_widget       = None
        self._display_widget           = None
        self._view_widget              = None
        self._swizzle_widget           = None
        self._fstop_widget             = None
        self._fstop_decrement_widget   = None
        self._fstop_increment_widget   = None
        self._gain_widget              = None
        self._exposure_widget          = None
        self._gain_reset_widget        = None
        self._gamma_widget             = None
        self._gamma_reset_widget       = None

        self._buildWidgets()
        self._toggle_color_management_action.setEnabled(False)
        self._enableWidgets(False)

        # Enable/disable color management.
        mari.gl_render.setPostProcessingEnabled(self.isColorManagementEnabled())

        # *** IMPORTANT *** The post filter collection used to be called 'OpenColorIO' but was renamed to hide the fact
        # we use OpenColorIO from our users. So as a temporary workaround we need to check for the old filter collection
        # on startup and remove it if found.
        delete_filter_collection = mari.gl_render.findPostFilterCollection('OpenColorIO')
        if delete_filter_collection is not None:
            mari.gl_render.deletePostFilterCollection(delete_filter_collection)

        # Create the OCIO post filter collection if not present.
        self._filter_collection = mari.gl_render.findPostFilterCollection('Color Space')
        if self._filter_collection is None:
            self._filter_collection = mari.gl_render.createPostFilterCollection('Color Space')
        else:
            self._filter_collection.clear()
        self._filter_collection.setReadOnly(True)

        self._lut_filter = self._filter_collection.createGLSL('LUT Transform')
        if not self._lut_file_list.isEmpty() and not self._rebuildLUTFilter(self._lut_file_list.at(0)):
            self._lut_file_list.clear()

        self._display_filter = self._filter_collection.createGLSL('Display Transform')
        self._rebuildDisplayFilter()

        self._buildMetadata()

        # Set the color management filter stack as the current.
        mari.gl_render.setPostFilterCollection(self._filter_collection)

        # Attach ourselves to the applications toolbar created signal so we can rebuild the toolbar when it's been
        # destoyed.
        mari.utils.connect(mari.app.toolBarsCreated, self._toolBarsCreated)

        # Attach ourselves to the appropriate GL signals so we can enable and disable widgets.
        mari.utils.connect(mari.gl_render.postProcessingEnabled,          self._postProcessingEnabled)
        mari.utils.connect(mari.gl_render.setCurrentPostFilterCollection, self._setCurrentPostFilterCollection)

        # Attach ourselves to the appropriate project signals so we can load and save settings.
        mari.utils.connect(mari.projects.openedProject,      self._openedProject)
        mari.utils.connect(mari.projects.aboutToSaveProject, self._aboutToSaveProject)
        mari.utils.connect(mari.projects.projectClosed,      self._closedProject)

        # Update the UI to match the current project, if we have one.
        current_project = mari.projects.current()
        if current_project is not None:
            self._openedProject(current_project)

    #-----------------------------------------------------------------------------------------

    def isColorManagementEnabled(self):
        return self._toggle_color_management_action.isChecked()

    #-----------------------------------------------------------------------------------------

    def setLUTPath(self, value, update_metadata = True, force_shader_build = False):
        if (self._lut_file_list.isEmpty() and value != '') or \
           (not self._lut_file_list.isEmpty() and value == '') or \
           (not self._lut_file_list.isEmpty() and value != self._lut_file_list.at(0)) \
           :
            if self._rebuildLUTFilter(value, force_shader_build):
                self._lut_file_list.clear()
                if value != '':
                    self._lut_file_list.append(value)
                    self._lut_file_list.setPickedFile(value)

                    self._clear_lut_action.setEnabled(True)
                    self._lut_extrapolate_widget.setEnabled(True)
                    self._lut_filter.setEnabled(True)
                else:
                    self._clear_lut_action.setEnabled(False)
                    self._lut_extrapolate_widget.setEnabled(False)
                    self._lut_filter.setEnabled(False)

                if update_metadata:
                    mari.utils.disconnect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)
                    self._lut_filter.setMetadata('File', self._lut_file_list)
                    mari.utils.connect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)

            else:
                # If this was a request via the metadata system we will need to put the value back to what it was
                # before.
                if not update_metadata:
                    mari.utils.disconnect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)
                    self._lut_filter.setMetadata('File', self._lut_file_list)
                    mari.utils.connect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)
                
                return False

        return True

    #-----------------------------------------------------------------------------------------

    def resetLUT(self):
        if ocio.lut_file_list_default.isEmpty() or not self.setLUTPath(ocio.lut_file_list_default.at(0)):
            self.setLUTPath('')

    #-----------------------------------------------------------------------------------------

    def selectLUT(self):
        lut_path = mari.utils.misc.getOpenFileName(None,
                                                   'Select LUT File',
                                                   '' if self._lut_file_list.isEmpty() else self._lut_file_list.at(0),
                                                   ocio.lutFileFilter(),
                                                   None,
                                                   0)
        if os.path.isfile(lut_path):
            self.setLUTPath(lut_path)

    #-----------------------------------------------------------------------------------------

    def setExtrapolateEnabled(self, value, update_widget = True, update_metadata = True):
        if value != self._lut_extrapolate:
            self._lut_extrapolate = value

            if update_widget:
                block = self._lut_extrapolate_widget.blockSignals(True)
                self._lut_extrapolate_widget.setChecked(self._lut_extrapolate)
                self._lut_extrapolate_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)
                self._lut_filter.setMetadata('Extrapolate', self._lut_extrapolate)
                mari.utils.connect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)

            if not self._rebuildLUTFilter(lut_path = '' if self._lut_file_list.isEmpty() else self._lut_file_list.at(0),
                                          force_shader_build = True):
                self.resetLUT()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed extrapolate to \'%s\'' % self._lut_extrapolate)

    #-----------------------------------------------------------------------------------------

    def setConfigPath(self, value, update_metadata = True):
        if self._config_file_list.isEmpty() or value != self._config_file_list.at(0):
            config = ocio.loadConfig(value, True)
            if config is not None:
                self._config_file_list.clear()
                self._config_file_list.append(value)
                self._config_file_list.setPickedFile(value)

                self._config = config

                self._updateDisplayWidgets()
                self._updateDisplayMetadata()

                self._rebuildDisplayFilter()

                ocio.printMessage(ocio.MessageType.DEBUG, 'Changed config to \'%s\'' % self._config_file_list.at(0))

            else:
                # If this was a request via the metadata system we will need to put the value back to what it was
                # before.
                if not update_metadata:
                    mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                    self._display_filter.setMetadata('ConfigPath', self._config_file_list)
                    mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                
                return False

        return True

    #-----------------------------------------------------------------------------------------

    def selectConfig(self):
        config_path = mari.utils.misc.getOpenFileName(None,
                                                      'Select Configuration File',
                                                      '' if self._config_file_list.isEmpty() else self._config_file_list.at(0),
                                                      ocio.configFileFilter(),
                                                      None,
                                                      0)
        if os.path.isfile(config_path):
            self.setConfigPath(config_path)

    #-----------------------------------------------------------------------------------------

    def setColorSpace(self, value, update_widget = True, update_metadata = True):
        if value != self._color_space:
            self._color_space = value

            if update_widget:
                block = self._color_space_widget.blockSignals(True)
                index = self._color_space_widget.findText(self._color_space)
                self._color_space_widget.setCurrentIndex(index)
                self._color_space_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('InputColorSpace', self._color_space)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed input color space to \'%s\'' % self._color_space)

    #-----------------------------------------------------------------------------------------

    def setDisplay(self, value, update_widget = True, update_metadata = True):
        if value != self._display:
            self._display = value

            if update_widget:
                block = self._display_widget.blockSignals(True)
                index = self._display_widget.findText(self._display)
                self._display_widget.setCurrentIndex(index)
                self._display_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('Display', self._display)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self.setView(self._config.getDefaultView(self._display), update_widget, update_metadata)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed display to \'%s\'' % self._display)

    #-----------------------------------------------------------------------------------------

    def setView(self, value, update_widget = True, update_metadata = True):
        if value != self._view:
            self._view = value

            if update_widget:
                block = self._view_widget.blockSignals(True)
                index = self._view_widget.findText(self._view)
                self._view_widget.setCurrentIndex(index)
                self._view_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('View', self._view)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed view to \'%s\'' % self._view)

    #-----------------------------------------------------------------------------------------

    def setSwizzle(self, value, update_widget = True, update_metadata = True):
        if value != self._swizzle:
            self._swizzle = value

            if update_widget:
                block = self._swizzle_widget.blockSignals(True)
                index = self._swizzle_widget.findText(self._swizzle)
                self._swizzle_widget.setCurrentIndex(index)
                self._swizzle_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('Swizzle', self._swizzle)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed swizzle to \'%s\'' % self._swizzle)

    #-----------------------------------------------------------------------------------------

    def setGain(self, value, update_widget = True, update_metadata = True):
        if value != self._gain:
            self._gain = value

            if update_widget:
                self._updateGainWidgets()

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('Gain', self._gain)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed gain to \'%s\'' % self._gain)

    #-----------------------------------------------------------------------------------------

    def setGamma(self, value, update_widget = True, update_metadata = True):
        if value != self._gamma:
            self._gamma = value

            if update_widget:
                block = self._gamma_widget.blockSignals(True)
                self._gamma_widget.setValue(self._gamma)
                self._gamma_widget.blockSignals(block)

            if update_metadata:
                mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
                self._display_filter.setMetadata('Gamma', self._gamma)
                mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

            self._rebuildDisplayFilter()

            ocio.printMessage(ocio.MessageType.DEBUG, 'Changed gamma to \'%s\'' % self._gamma)

    #-----------------------------------------------------------------------------------------

    def updateLUTSize(self):
        ocio.printMessage(ocio.MessageType.DEBUG, 'Updating LUT size...')

        # Rebuild the LUT filter.
        if self._lut_sampler_name is not None:
            self._lut_filter.deleteTexture(self._lut_sampler_name)
            self._lut_sampler_name = None

        self._lut_filter_cache_id  = None
        self._lut_texture_cache_id = None

        if not self._rebuildLUTFilter(lut_path = '' if self._lut_file_list.isEmpty() else self._lut_file_list.at(0),
                                      force_shader_build = True):
            self.resetLUT()

        # Rebuild the display filter.
        if self._display_sampler_name is not None:
            self._display_filter.deleteTexture(self._display_sampler_name)
            self._display_sampler_name = None

        self._display_filter_cache_id  = None
        self._display_texture_cache_id = None

        self._rebuildDisplayFilter()

    #-----------------------------------------------------------------------------------------

    def updateFStopCenter(self):
        ocio.printMessage(ocio.MessageType.DEBUG, 'Updating f-stop center...')

        fstop = ocio.convertGainToFStop(self._gain)
        self._updateFStopWidgetText(fstop)

    #-----------------------------------------------------------------------------------------
    # Widgets:
    #-----------------------------------------------------------------------------------------

    def _buildWidgets(self):
        action_list = list()

        self._toggle_color_management_action = self._addAction(
            '/Mari/OpenColorIO/&Toggle Color Management',
            'mari.system._ocio_toolbar.toolbar._toggleColorManagement()',
            'ColorManager.png',
            'Toggle on/off color management',
            'Toggle color management')
        self._toggle_color_management_action.setCheckable(True)
        self._toggle_color_management_action.setChecked(ocio.enabled_default)
        action_list.append('/Mari/OpenColorIO/&Toggle Color Management')

        self._select_config_action = self._addAction(
            '/Mari/OpenColorIO/&Select Config',
            'mari.system._ocio_toolbar.toolbar.selectConfig()',
            'LoadColorConfig.png',
            'Select color space configuration file',
            'Select config')
        action_list.append('/Mari/OpenColorIO/&Select Config')

        self._select_lut_action = self._addAction(
            '/Mari/OpenColorIO/&Select LUT',
            'mari.system._ocio_toolbar.toolbar.selectLUT()',
            'LoadLookupTable.png',
            'Select LUT file',
            'Select LUT')
        action_list.append('/Mari/OpenColorIO/&Select LUT')

        self._clear_lut_action = self._addAction(
            '/Mari/OpenColorIO/&Clear LUT',
            'mari.system._ocio_toolbar.toolbar._clearLUT()',
            'ClearLookupTable.png',
            'Clear current LUT',
            'Clear LUT')
        action_list.append('/Mari/OpenColorIO/&Clear LUT')

        mari.app.deleteToolBar('Color Space')
        self._toolbar = mari.app.createToolBar('Color Space', True)
        self._toolbar.addActionList(action_list, False)
        self._toolbar.setLockedSlot(True)
        self._toolbar.setSpacing(TOOLBAR_SPACING)

        self._toolbar.insertSeparator('/Mari/OpenColorIO/&Select LUT')

        # Extrapolate:
        self._toolbar.addWidget(QtGui.QLabel('Extrapolate'))
        self._lut_extrapolate_widget = QtGui.QCheckBox()
        self._lut_extrapolate_widget.setToolTip('Extrapolate if outside LUT range');
        self._lut_extrapolate_widget.setChecked(self._lut_extrapolate)
        self._lut_extrapolate_widget.connect(
            QtCore.SIGNAL('toggled(bool)'),
            lambda value: self.setExtrapolateEnabled(value = value, update_widget = False, update_metadata = True))
        self._toolbar.addWidget(self._lut_extrapolate_widget)

        self._toolbar.addSeparator()

        color_spaces = [color_space.getName() for color_space in self._config.getColorSpaces()]

        # Color-Space:
        self._color_space_widget = self._addComboBox(
            'Input Color Space',
            color_spaces,
            self._color_space,
            ocio.color_space_default,
            lambda value: self.setColorSpace(value = value, update_widget = False, update_metadata = True))
        self._color_space = self._color_space_widget.currentText

        # Display:
        self._display_widget = self._addComboBox(
            'Display Device',
            self._config.getDisplays(),
            self._display,
            ocio.display_default,
            lambda value: self.setDisplay(value = value, update_widget = False, update_metadata = True))
        self._display = self._display_widget.currentText

        # View:
        self._view_widget = self._addComboBox(
            'View Transform',
            self._config.getViews(self._display),
            self._view,
            ocio.view_default,
            lambda value: self.setView(value = value, update_widget = False, update_metadata = True))
        self._view = self._view_widget.currentText

        # Swizzle:
        self._swizzle_widget = self._addComboBox(
            'Component',
            ocio.SWIZZLE_TYPES,
            self._swizzle,
            ocio.swizzle_default,
            lambda value: self.setSwizzle(value = value, update_widget = False, update_metadata = True))
        self._swizzle = self._swizzle_widget.currentText

        # Gain Group:
        group_widget, layout = self._addWidgetGroup()
        group_widget.setMaximumWidth(GAIN_GROUP_MAX_WIDTH)

        layout.addWidget(QtGui.QLabel('Gain'))

        # F-Stop:
        subgroup_widget = QtGui.QWidget()
        layout.addWidget(subgroup_widget)

        sublayout = QtGui.QHBoxLayout()
        sublayout.setSpacing(0)
        sublayout.setMargin(0)
        subgroup_widget.setLayout(sublayout)

        exposure     = ocio.convertGainToExposure(self._gain)
        fstop        = ocio.convertExposureToFStop(exposure)
        scale        = (exposure - ocio.EXPOSURE_MIN) / ocio.EXPOSURE_DELTA
        widget_max   = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.FSTOP_STEP_SIZE))
        widget_value = scale * widget_max
        self._fstop_widget = mari.LineEdit()
        self._fstop_widget.setRange(widget_max)
        self._fstop_widget.setMaximumWidth(FSTOP_MAX_WIDTH)
        self._fstop_widget.setReadOnly(True)
        self._updateFStopWidgetText(fstop)
        self._fstop_widget.setValue(widget_value)
        mari.utils.connect(self._fstop_widget.movedMouse, self._fstopMovedMouse)
        self._fstop_widget.addToLayout(sublayout)

        self._fstop_decrement_widget = self._addSmallButtom(
            sublayout,
            '-',
            'Decrease gain 1/2 stop',
            lambda: self.setGain(ocio.convertExposureToGain(ocio.convertGainToExposure(self._gain) - 0.5)))
        self._fstop_increment_widget = self._addSmallButtom(
            sublayout,
            '+',
            'Increase gain 1/2 stop',
            lambda: self.setGain(ocio.convertExposureToGain(ocio.convertGainToExposure(self._gain) + 0.5)))

        ocio.registerLUTSizeChanged(self.updateLUTSize)
        ocio.registerFStopCenterChanged(self.updateFStopCenter)

        # Gain:
        subgroup_widget = QtGui.QWidget()
        layout.addWidget(subgroup_widget)

        sublayout = QtGui.QHBoxLayout()
        sublayout.setSpacing(3)
        sublayout.setMargin(0)
        subgroup_widget.setLayout(sublayout)

        widget_max   = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.EXPOSURE_STEP_SIZE))
        widget_value = scale * widget_max
        self._gain_widget = mari.LineEdit()
        self._gain_widget.setRange(widget_max)
        self._gain_widget.addFloatValidator(ocio.GAIN_MIN, ocio.GAIN_MAX, ocio.GAIN_PRECISION)
        self._gain_widget.setMaximumWidth(GAIN_MAX_WIDTH)
        self._updateGainWidgetText()
        self._gain_widget.setValue(widget_value)
        mari.utils.connect(
            self._gain_widget.lostFocus,
            lambda: self.setGain(max(min(float(self._gain_widget.text()), ocio.GAIN_MAX), ocio.GAIN_MIN)))
        mari.utils.connect(self._gain_widget.movedMouse, self._gainMovedMouse)
        self._gain_widget.addToLayout(sublayout)

        # Exposure:
        self._exposure_widget = QtGui.QSlider()
        self._exposure_widget.orientation = 1
        self._exposure_widget.setMaximum(widget_max)
        self._exposure_widget.setValue(widget_value)
        self._exposure_widget.setMinimumWidth(EXPOSURE_MAX_WIDTH)
        self._exposure_widget.setMaximumWidth(EXPOSURE_MAX_WIDTH)
        mari.utils.connect(self._exposure_widget.valueChanged, self._exposureChanged)
        sublayout.addWidget(self._exposure_widget)

        self._gain_reset_widget = self._addSmallButtom(
            layout,
            'R',
            'Reset gain to default',
            lambda: self.setGain(value = ocio.GAIN_RESET, update_widget = True, update_metadata = True))

        # Gamma:
        group_widget, layout = self._addWidgetGroup()
        group_widget.setMaximumWidth(GAMMA_MAX_WIDTH)

        layout.addWidget(QtGui.QLabel('Gamma'))

        self._gamma_widget = mari.FloatSlider()
        self._gamma_widget.setRange(ocio.GAMMA_MIN, ocio.GAMMA_MAX)
        self._gamma_widget.setStepSize(ocio.GAMMA_STEP_SIZE)
        self._gamma_widget.setPrecision(ocio.GAMMA_PRECISION)
        self._gamma_widget.setValue(self._gamma)
        mari.utils.connect(
            self._gamma_widget.valueChanged,
            lambda value: self.setGamma(value = value, update_widget = False, update_metadata = True))
        self._gamma_widget.addToLayout(layout)

        self._gamma_reset_widget = self._addSmallButtom(
            layout,
            'R',
            'Reset gamma to default',
            lambda: self.setGamma(value = ocio.GAMMA_RESET, update_widget = True, update_metadata = True))

    #-----------------------------------------------------------------------------------------

    def _updateDisplayWidgets(self):
        color_spaces = [color_space.getName() for color_space in self._config.getColorSpaces()]

        self._updateComboBox(self._color_space_widget, color_spaces, self._color_space, ocio.color_space_default)
        self._color_space = self._color_space_widget.currentText

        self._updateComboBox(self._display_widget, self._config.getDisplays(), self._display, ocio.display_default)
        self._display = self._display_widget.currentText

        self._updateComboBox(self._view_widget, self._config.getViews(self._display), self._view, ocio.view_default)
        self._view = self._view_widget.currentText

        self._updateComboBox(self._swizzle_widget, ocio.SWIZZLE_TYPES, self._swizzle, ocio.swizzle_default)
        self._swizzle = self._swizzle_widget.currentText

        self._updateGainWidgets()

        self._gamma_widget.setValue(self._gamma)

    #-----------------------------------------------------------------------------------------

    def _enableWidgets(self, enable):
        self._select_config_action.setEnabled(enable)
        self._select_lut_action.setEnabled(enable)
        lut_enable = enable and not self._lut_file_list.isEmpty()
        self._clear_lut_action.setEnabled(lut_enable)
        self._lut_extrapolate_widget.setEnabled(lut_enable)

        self._color_space_widget.setEnabled(enable)
        self._display_widget.setEnabled(enable)
        self._view_widget.setEnabled(enable)
        self._swizzle_widget.setEnabled(enable)

        self._fstop_widget.setEnabled(enable)
        self._fstop_decrement_widget.setEnabled(enable)
        self._fstop_increment_widget.setEnabled(enable)
        self._gain_widget.setEnabled(enable)
        self._exposure_widget.setEnabled(enable)
        self._gain_reset_widget.setEnabled(enable)

        self._gamma_widget.setEnabled(enable)
        self._gamma_reset_widget.setEnabled(enable)

    #-----------------------------------------------------------------------------------------

    def _addAction(self, identifier, command, icon_filename, tip, whats_this):
        action = mari.actions.find(identifier)
        if action is None:
            action = mari.actions.create(identifier, command)

            icon_path = mari.resources.path(mari.resources.ICONS) + '/' + icon_filename
            action.setIconPath(icon_path)

            action.setStatusTip(tip)
            action.setToolTip(tip)
            action.setWhatsThis(whats_this)

        return action

    #-----------------------------------------------------------------------------------------

    def _addWidgetGroup(self):
        group_widget = QtGui.QWidget()
        self._toolbar.addWidget(group_widget)

        layout = QtGui.QHBoxLayout()
        layout.setSpacing(1)
        layout.setMargin(1)
        group_widget.setLayout(layout)

        return (group_widget, layout)

    #-----------------------------------------------------------------------------------------

    def _addComboBox(self, label, items, value, default, value_changed, *args):
        group_widget, layout = self._addWidgetGroup()

        layout.addWidget(QtGui.QLabel(label))

        widget = QtGui.QComboBox()
        self._updateComboBox(widget, items, value, default)
        widget.connect(QtCore.SIGNAL('currentIndexChanged(const QString &)'), value_changed)
        layout.addWidget(widget)

        return widget

    #-----------------------------------------------------------------------------------------

    def _updateComboBox(self, widget, items, value, default):
        block = widget.blockSignals(True)

        widget.clear()
        for item in items:
            widget.addItem(item)

        if items.count(value) != 0:
            widget.setCurrentIndex(items.index(value))
        elif items.count(default) != 0:
            widget.setCurrentIndex(items.index(default))

        widget.blockSignals(block)

    #-----------------------------------------------------------------------------------------

    def _addSmallButtom(self, layout, label, tool_tip, value_changed, *args):
        widget = QtGui.QPushButton(label);
        widget.setToolTip(tool_tip);
        widget.setFixedHeight(16);
        widget.setFixedWidth(16);
        widget.connect(QtCore.SIGNAL('released()'), value_changed)
        layout.addWidget(widget);

        return widget

    #-----------------------------------------------------------------------------------------

    def _convertFStopWidgetValueToGain(self, value):
        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.FSTOP_STEP_SIZE))
        scale      = float(value) / float(widget_max)
        exposure   = ocio.EXPOSURE_MIN + scale * ocio.EXPOSURE_DELTA
        return ocio.convertExposureToGain(exposure)

    #-----------------------------------------------------------------------------------------

    def _convertExposureWidgetValueToGain(self, value):
        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.EXPOSURE_STEP_SIZE))
        scale      = float(value) / float(widget_max)
        exposure   = ocio.EXPOSURE_MIN + scale * ocio.EXPOSURE_DELTA
        return ocio.convertExposureToGain(exposure)

    #-----------------------------------------------------------------------------------------

    def _updateFStopWidgetText(self, fstop):
        block = self._fstop_widget.blockSignals(True)
        if fstop < 10.0:
            # Floor the value to one decimal place and only display the decimal point if necessary
            text = '%f' % fstop
            index = text.index('.')
            if text[index + 1] == '0':
                text = text[:index]
            else:
                text = text[:index + 2]
            self._fstop_widget.setText('f/%s' % text)
        else:
            self._fstop_widget.setText('f/%d' % int(fstop))
        self._fstop_widget.blockSignals(block)

    #-----------------------------------------------------------------------------------------

    def _updateGainWidgetText(self):
        block = self._gain_widget.blockSignals(True)
        self._gain_widget.setText(('%.' + ('%d' % ocio.GAIN_PRECISION) + 'f') % self._gain)
        self._gain_widget.home(False)
        self._gain_widget.blockSignals(block)

    #-----------------------------------------------------------------------------------------

    def _updateGainWidgets(self):
        exposure = ocio.convertGainToExposure(self._gain)
        fstop    = ocio.convertExposureToFStop(exposure)
        self._updateFStopWidgetText(fstop)

        scale        = (exposure - ocio.EXPOSURE_MIN) / ocio.EXPOSURE_DELTA
        widget_max   = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.FSTOP_STEP_SIZE))
        widget_value = int(round(scale * float(widget_max)))
        block = self._fstop_widget.blockSignals(True)
        self._fstop_widget.setValue(widget_value)
        self._fstop_widget.blockSignals(block)

        self._updateGainWidgetText()

        widget_max   = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.EXPOSURE_STEP_SIZE))
        widget_value = int(round(scale * float(widget_max)))
        block = self._gain_widget.blockSignals(True)
        self._gain_widget.setValue(widget_value)
        self._gain_widget.blockSignals(block)
        block = self._exposure_widget.blockSignals(True)
        self._exposure_widget.setValue(widget_value)
        self._exposure_widget.blockSignals(block)

    #-----------------------------------------------------------------------------------------

    def _toggleColorManagement(self):
        enabled = self.isColorManagementEnabled()
        mari.gl_render.setPostProcessingEnabled(enabled)
        self._enableWidgets(enabled)
        ocio.printMessage(ocio.MessageType.DEBUG, 'Toggled color management to \'%s\'' % ('on' if enabled else 'off'))

    #-----------------------------------------------------------------------------------------

    def _clearLUT(self):
        self.setLUTPath('')
        ocio.printMessage(ocio.MessageType.DEBUG, 'Cleared lut')

    #-----------------------------------------------------------------------------------------

    def _fstopMovedMouse(self, value):
        self.setGain(self._convertFStopWidgetValueToGain(float(value)), False)

        exposure = ocio.convertGainToExposure(self._gain)
        fstop    = ocio.convertExposureToFStop(exposure)
        self._updateFStopWidgetText(fstop)

        self._updateGainWidgetText()

        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.EXPOSURE_STEP_SIZE))
        scale      = (exposure - ocio.EXPOSURE_MIN) / ocio.EXPOSURE_DELTA
        value      = int(round(scale * float(widget_max)))
        self._gain_widget.setValue(value)

        value = max(min(value, widget_max), 0)
        self._exposure_widget.setValue(value)

    #-----------------------------------------------------------------------------------------

    def _gainMovedMouse(self, value):
        self.setGain(self._convertExposureWidgetValueToGain(float(value)), False)

        self._updateGainWidgetText()

        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.EXPOSURE_STEP_SIZE))
        value      = max(min(value, widget_max), 0)
        self._exposure_widget.setValue(value)

        exposure = ocio.convertGainToExposure(self._gain)
        fstop    = ocio.convertExposureToFStop(exposure)
        self._updateFStopWidgetText(fstop)

        scale      = (exposure - ocio.EXPOSURE_MIN) / ocio.EXPOSURE_DELTA
        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.FSTOP_STEP_SIZE))
        value      = int(round(scale * float(widget_max)))
        self._fstop_widget.setValue(value)

    #-----------------------------------------------------------------------------------------

    def _exposureChanged(self, value):
        self.setGain(value = self._convertExposureWidgetValueToGain(float(value)),
                     update_widget = False,
                     update_metadata = True)

        self._updateGainWidgetText()

        self._gain_widget.setValue(value)

        exposure = ocio.convertGainToExposure(self._gain)
        fstop    = ocio.convertExposureToFStop(exposure)
        self._updateFStopWidgetText(fstop)

        scale      = (exposure - ocio.EXPOSURE_MIN) / ocio.EXPOSURE_DELTA
        widget_max = int(math.ceil(ocio.EXPOSURE_DELTA / ocio.FSTOP_STEP_SIZE))
        value      = int(round(scale * float(widget_max)))
        self._fstop_widget.setValue(value)

    #-----------------------------------------------------------------------------------------
    # Metadata:
    #-----------------------------------------------------------------------------------------

    def _buildMetadata(self):
        # LUT:
        # ---
        mari.utils.connect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)
        self._updateLUTMetadata()

        flags = self._lut_filter.METADATA_VISIBLE | self._lut_filter.METADATA_EDITABLE
        self._lut_filter.setMetadataFlags('File', flags)

        self._lut_filter.setMetadataFlags('Extrapolate', flags)

        # Display:
        # -------
        mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)
        self._updateDisplayMetadata()

        self._display_filter.setMetadataDisplayName('ConfigPath', 'Configuration File')
        flags = self._display_filter.METADATA_VISIBLE | self._display_filter.METADATA_EDITABLE
        self._display_filter.setMetadataFlags('ConfigPath', flags)

        self._display_filter.setMetadataDisplayName('InputColorSpace', 'Input Color Space')
        self._display_filter.setMetadataFlags('InputColorSpace', flags)

        self._display_filter.setMetadataDisplayName('Display', 'Display Device')
        self._display_filter.setMetadataFlags('Display', flags)

        self._display_filter.setMetadataDisplayName('View', 'View Transform')
        self._display_filter.setMetadataFlags('View', flags)

        self._display_filter.setMetadataDisplayName('Swizzle', 'Component')
        self._display_filter.setMetadataFlags('Swizzle', flags)

        self._display_filter.setMetadataDefault('Gain', ocio.GAIN_RESET)
        self._display_filter.setMetadataRange('Gain', ocio.GAIN_MIN, ocio.GAIN_MAX)
        self._display_filter.setMetadataStep('Gain', ocio.GAIN_STEP_SIZE)
        self._display_filter.setMetadataFlags('Gain', flags)

        self._display_filter.setMetadataDefault('Gamma', ocio.GAMMA_RESET)
        self._display_filter.setMetadataRange('Gamma', ocio.GAMMA_MIN, ocio.GAMMA_MAX)
        self._display_filter.setMetadataStep('Gamma', ocio.GAMMA_STEP_SIZE)
        self._display_filter.setMetadataFlags('Gamma', flags)

    #-----------------------------------------------------------------------------------------

    def _updateLUTMetadata(self):
        mari.utils.disconnect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)

        self._lut_filter.setMetadata('File', self._lut_file_list)

        self._lut_filter.setMetadata('Extrapolate', self._lut_extrapolate)

        mari.utils.connect(self._lut_filter.metadataValueChanged, lutMetadataValueChanged)

    #-----------------------------------------------------------------------------------------

    def _updateDisplayMetadata(self):
        mari.utils.disconnect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)

        self._display_filter.setMetadata('ConfigPath', self._config_file_list)

        color_spaces = [color_space.getName() for color_space in self._config.getColorSpaces()]

        self._display_filter.setMetadata('InputColorSpace', self._color_space)
        self._display_filter.setMetadataItemList('InputColorSpace', color_spaces)

        self._display_filter.setMetadata('Display', self._display)
        self._display_filter.setMetadataItemList('Display', self._config.getDisplays())

        self._display_filter.setMetadata('View', self._view)
        self._display_filter.setMetadataItemList('View', self._config.getViews(self._display))

        self._display_filter.setMetadata('Swizzle', self._swizzle)
        self._display_filter.setMetadataItemList('Swizzle', ocio.SWIZZLE_TYPES)

        self._display_filter.setMetadata('Gain', self._gain)

        self._display_filter.setMetadata('Gamma', self._gain)

        mari.utils.connect(self._display_filter.metadataValueChanged, displayMetadataValueChanged)


    #-----------------------------------------------------------------------------------------
    # External Connections:
    #-----------------------------------------------------------------------------------------

    def _openedProject(self, project):
        ocio.printMessage(ocio.MessageType.DEBUG, 'Loading settings for project \'%s\'' % project.name())

        # Load the settings stored as metadata on the project...

        # General:
        # -------

        self._toggle_color_management_action.setEnabled(True)
        self._toggle_color_management_action.setChecked(project.metadata('ColorEnabled') if project.hasMetadata('ColorEnabled') else ocio.enabled_default)

        # Enable/disable color management (MUST be done after modifications to 'self._toggle_color_management_action'.
        mari.gl_render.setPostProcessingEnabled(self.isColorManagementEnabled())

        filter_collection = None
        if project.hasMetadata('ColorProfile'):
            # *** IMPORTANT *** The post filter collection used to be called 'OpenColorIO' but was renamed to hide the
            # fact we use OpenColorIO from our users. So as a temporary workaround we need to check for the old filter
            # collection correct for it.
            name = project.metadata('ColorProfile')
            if name == 'OpenColorIO':
                name = 'Color Space'
            filter_collection = mari.gl_render.findPostFilterCollection(name)

        # Default the color management filter stack if the working one doesn't exist.
        if filter_collection is None:
            filter_collection = mari.gl_render.findPostFilterCollection(ocio.profile_default)

        mari.gl_render.setPostFilterCollection(filter_collection)

        # LUT:
        # ---

        lut_extrapolate = project.metadata('OcioLutExtrapolate') if project.hasMetadata('OcioLutExtrapolate') else ocio.lut_extrapolate_default
        force_shader_build = lut_extrapolate != self._lut_extrapolate
        self._lut_extrapolate = lut_extrapolate
        self._lut_extrapolate_widget.setChecked(self._lut_extrapolate)

        if project.hasMetadata('OcioLutPath'):
            lut_path = ocio.buildLoadPath(project.metadata('OcioLutPath'))
            if not self.setLUTPath(value = lut_path, update_metadata = True, force_shader_build = force_shader_build):
                self.resetLUT()
        else:
            self.resetLUT()

        # Display:
        # -------

        self._color_space = project.metadata(  'OcioColorSpace') if project.hasMetadata('OcioColorSpace') else ocio.color_space_default
        self._display     = project.metadata(     'OcioDisplay') if project.hasMetadata(   'OcioDisplay') else ocio.display_default
        self._view        = project.metadata(        'OcioView') if project.hasMetadata(      'OcioView') else ocio.view_default
        self._swizzle     = project.metadata(     'OcioSwizzle') if project.hasMetadata(   'OcioSwizzle') else ocio.swizzle_default
        self._gain        = max(min(project.metadata('OcioGain'),
                                    ocio.GAIN_MAX),
                                ocio.GAIN_MIN)                   if project.hasMetadata(      'OcioGain') else ocio.gain_default
        self._gamma       = project.metadata(       'OcioGamma') if project.hasMetadata(     'OcioGamma') else ocio.gamma_default

        # Attempt to load a configuration file...
        self._config_file_list.clear()
        self._config = None

        # 1. Environment variable.
        config_path = os.getenv('OCIO')
        if config_path is not None:
            self.setConfigPath(config_path)

        # 2. Project setting.
        if self._config is None and project.hasMetadata('OcioConfigPath'):
            self.setConfigPath(ocio.buildLoadPath(project.metadata('OcioConfigPath')))

        # 3. Use the default if nothing was found.
        if self._config is None:
            self._config_file_list = mari.FileList(ocio.config_file_list_default)
            self._config           = ocio.config_default

            self._updateDisplayWidgets()
            self._rebuildDisplayFilter()

        self._enableWidgets(filter_collection.name() == 'Color Space' and self._toggle_color_management_action.isChecked())
        self._updateLUTMetadata()
        self._updateDisplayMetadata()

        self._printLog()

    #-----------------------------------------------------------------------------------------

    def _aboutToSaveProject(self, project):
        ocio.printMessage(ocio.MessageType.DEBUG, 'Saving settings for project \'%s\'' % project.name())

        # Store the settings as metadata on the project.
        project.setMetadata(      'ColorEnabled', self.isColorManagementEnabled())
        filter_collection = mari.gl_render.currentPostFilterCollection()
        if filter_collection is not None:
            project.setMetadata(  'ColorProfile', filter_collection.name())
        project.setMetadata('OcioLutExtrapolate', self._lut_extrapolate)
        project.setMetadata(       'OcioLutPath', '' if self._lut_file_list.isEmpty() else ocio.buildSavePath(self._lut_file_list.at(0)))
        if os.getenv('OCIO') is None:
            project.setMetadata('OcioConfigPath', '' if self._config_file_list.isEmpty() else ocio.buildSavePath(self._config_file_list.at(0)))
        project.setMetadata(    'OcioColorSpace', self._color_space)
        project.setMetadata(       'OcioDisplay', self._display)
        project.setMetadata(          'OcioView', self._view)
        project.setMetadata(       'OcioSwizzle', self._swizzle)
        project.setMetadata(          'OcioGain', self._gain)
        project.setMetadata(         'OcioGamma', self._gamma)

    #-----------------------------------------------------------------------------------------

    def _closedProject(self):
        self._toggle_color_management_action.setEnabled(False)
        self._enableWidgets(False)

    #-----------------------------------------------------------------------------------------

    def _toolBarsCreated(self):
        # Things like deleting Mari's configuration file and reseting the layout to the default will destroy the toolbar
        # so we need to detect if this is the case and rebuild it!
        toolbar = mari.app.findToolBar('Color Space')
        if toolbar is None:
            ocio.printMessage(ocio.MessageType.DEBUG, 'Rebuilding missing toolbar...')
            self._buildWidgets()

    #-----------------------------------------------------------------------------------------

    def _postProcessingEnabled(self, enabled):
        self._toggle_color_management_action.setChecked(enabled)

        # Only enable or disable UI if we have a current project.
        current_project = mari.projects.current()
        if current_project is not None:
            self._enableWidgets(enabled)

    #-----------------------------------------------------------------------------------------

    def _setCurrentPostFilterCollection(self):
        # Only enable or disable UI if we have a current project.
        current_project = mari.projects.current()
        if current_project is not None:
            filter_collection = mari.gl_render.currentPostFilterCollection()
            if filter_collection is None or filter_collection.name() != 'Color Space':
                ocio.printMessage(ocio.MessageType.DEBUG, 'Disabling OpenColorIO')
                self._enableWidgets(False)
            else:
                ocio.printMessage(ocio.MessageType.DEBUG, 'Enabling OpenColorIO')
                self._enableWidgets(True)

    #-----------------------------------------------------------------------------------------
    # Filter:
    #-----------------------------------------------------------------------------------------

    def _rebuildLUTFilter(self, lut_path, force_shader_build = False):
        if lut_path == '':
            self._lut_filter.setDefinitionsSnippet('')
            self._lut_filter.setBodySnippet('')

            if self._lut_sampler_name is not None:
                self._lut_filter.deleteTexture(self._lut_sampler_name)
                self._lut_sampler_name = None

            self._lut_filter_cache_id  = None
            self._lut_texture_cache_id = None

        else:
            # There is a chance this is a bad file so we need to guard against it.
            try:
                self._lut_filter_cache_id, self._lut_texture_cache_id, self._lut_sampler_name = ocio.buildLUTFilter(
                    self._config,
                    lut_path,
                    self._lut_filter,
                    self._lut_filter_cache_id,
                    self._lut_texture_cache_id,
                    self._lut_extrapolate,
                    force_shader_build)

            except Exception, e:
                message = 'Failed to load LUT file \'%s\' due to \'%s\'' % (lut_path, e)
                ocio.printMessage(ocio.MessageType.ERROR, '%s' % message)
                if not mari.app.inTerminalMode():
                    mari.utils.misc.message(message, 'Color Space', 1024, 2)

                return False

        ocio.printMessage(ocio.MessageType.DEBUG, 'Changed LUT to \'%s\'' % lut_path)

        return True

    #-----------------------------------------------------------------------------------------

    def _rebuildDisplayFilter(self):
        display_transform = ocio.PyOpenColorIO.DisplayTransform()
        display_transform.setInputColorSpaceName(self._color_space)

        if hasattr(display_transform, 'setDisplay'):
            # OCIO 1.0+
            display_transform.setDisplay(self._display)
            display_transform.setView(self._view)
        else:
            # OCIO 0.8.X
            display_color_space = self._config.getDisplayColorSpaceName(self._display, self._view)
            display_transform.setDisplayColorSpaceName(display_color_space)

        # Add the channel sizzle.
        luma_coefs = self._config.getDefaultLumaCoefs()
        mtx, offset = ocio.PyOpenColorIO.MatrixTransform.View(ocio.SWIZZLE_VALUES[self._swizzle], luma_coefs)

        transform = ocio.PyOpenColorIO.MatrixTransform()
        transform.setValue(mtx, offset)
        display_transform.setChannelView(transform)

        # Add the linear gain.
        transform = ocio.PyOpenColorIO.CDLTransform()
        transform.setSlope((self._gain, self._gain, self._gain))
        display_transform.setLinearCC(transform)

        # Add the post-display CC.
        transform = ocio.PyOpenColorIO.ExponentTransform()
        transform.setValue([1.0 / max(1e-6, v) for v in (self._gamma, self._gamma, self._gamma, self._gamma)])
        display_transform.setDisplayCC(transform)
        
        processor = self._config.getProcessor(display_transform)

        self._display_filter_cache_id, self._display_texture_cache_id, self._display_sampler_name = ocio.buildProcessorFilter(
            processor,
            self._display_filter,
            self._display_filter_cache_id,
            self._display_texture_cache_id)

        current_canvas = mari.canvases.current()
        if current_canvas is not None:
            current_canvas.repaint()

    #-----------------------------------------------------------------------------------------
    # Debugging:
    #-----------------------------------------------------------------------------------------

    def _printLog(self):
        ocio.printMessage(    ocio.MessageType.INFO, '==============================================================')
        ocio.printMessage(    ocio.MessageType.INFO, 'Configuration:')
        ocio.printMessage(    ocio.MessageType.INFO, '==============================================================')
        ocio.printMessage(    ocio.MessageType.INFO, '     Enabled: %s; Default: %s'             % (mari.gl_render.isPostProcessingEnabled(),
                                                                                                    ocio.enabled_default))
        filter_collection = mari.gl_render.currentPostFilterCollection()
        if filter_collection is not None:
            ocio.printMessage(ocio.MessageType.INFO, '     Profile: %s; Default: %s'             % (filter_collection.name(),
                                                                                                    ocio.profile_default))
        else:
            ocio.printMessage(ocio.MessageType.INFO, '     Profile: None; Default: %s'           % (ocio.profile_default))
        ocio.printMessage(    ocio.MessageType.INFO, '    LUT Path: %s; Default: %s'             % ('' if self._lut_file_list.isEmpty() else self._lut_file_list.at(0),
                                                                                                    '' if ocio.lut_file_list_default.isEmpty() else ocio.lut_file_list_default.at(0)))
        ocio.printMessage(    ocio.MessageType.INFO, ' Extrapolate: %s; Default: %s'             % (self._lut_extrapolate,
                                                                                                    ocio.lut_extrapolate_default))
        ocio.printMessage(    ocio.MessageType.INFO, ' Config Path: %s; Default: %s'             % ('' if self._config_file_list.isEmpty() else self._config_file_list.at(0),
                                                                                                    '' if ocio.config_file_list_default.isEmpty() else ocio.config_file_list_default.at(0)))
        ocio.printMessage(    ocio.MessageType.INFO, ' Color Space: %s; Default: %s'             % (self._color_space,
                                                                                                    ocio.color_space_default))
        ocio.printMessage(    ocio.MessageType.INFO, '     Display: %s; Default: %s'             % (self._display,
                                                                                                    ocio.display_default))
        ocio.printMessage(    ocio.MessageType.INFO, '        View: %s; Default: %s'             % (self._view,
                                                                                                    ocio.view_default))
        ocio.printMessage(    ocio.MessageType.INFO, '     Swizzle: %s; Default: %s'             % (self._swizzle,
                                                                                                    ocio.swizzle_default))
        ocio.printMessage(    ocio.MessageType.INFO, '      F-Stop: %f; Default: %f; Center: %f' % (ocio.convertGainToFStop(self._gain),
                                                                                                    ocio.convertGainToFStop(ocio.gain_default),
                                                                                                    ocio.fstop_center))
        ocio.printMessage(    ocio.MessageType.INFO, '        Gain: %f; Default: %f'             % (self._gain,
                                                                                                    ocio.gain_default))
        ocio.printMessage(    ocio.MessageType.INFO, '       Gamma: %f; Default: %f'             % (self._gamma,
                                                                                                    ocio.gamma_default))
        ocio.printMessage(    ocio.MessageType.INFO, '==============================================================')

##############################################################################################
# The following functions CAN'T be part of the toolbar class as a potential bug in PythonQt
# causes the disconnect function to fail

def lutMetadataValueChanged(name, value):
    global toolbar

    ocio.printMessage(ocio.MessageType.DEBUG, 'LUT metadata \'%s\' changed to \'%s\'' % (name, value))

    if name == 'File':
        toolbar.setLUTPath(value = '' if value.isEmpty() else value.at(0),
                           update_metadata = False,
                           force_shader_build = False)

    elif name == 'Extrapolate':
        toolbar.setExtrapolateEnabled(value = value, update_widget = True, update_metadata = False)

#-----------------------------------------------------------------------------------------

def displayMetadataValueChanged(name, value):
    global toolbar

    ocio.printMessage(ocio.MessageType.DEBUG, 'Display metadata \'%s\' changed to \'%s\'' % (name, value))

    if name == 'ConfigPath':
        toolbar.setConfigPath(value = '' if value.isEmpty() else value.at(0), update_metadata = False)

    elif name == 'InputColorSpace':
        toolbar.setColorSpace(value = value, update_widget = True, update_metadata = False)

    elif name == 'Display':
        toolbar.setDisplay(value = value, update_widget = True, update_metadata = False)

    elif name == 'View':
        toolbar.setView(value = value, update_widget = True, update_metadata = False)

    elif name == 'Swizzle':
        toolbar.setSwizzle(value = value, update_widget = True, update_metadata = False)

    elif name == 'Gain':
        toolbar.setGain(value = value, update_widget = True, update_metadata = False)

    elif name == 'Gamma':
        toolbar.setGamma(value = value, update_widget = True, update_metadata = False)

##############################################################################################

if mari.app.isRunning():
    if not hasattr(mari.gl_render, 'createPostFilterCollection'):
        ocio.printMessage(ocio.MessageType.ERROR, 'This version of Mari does not support the mari.gl_render.createPostFilterCollection API')

    else:
        if ocio.config_default is not None:
            toolbar = OcioToolBar()

        else:
            # Destroy the OCIO post filter collection if present to prevent the user trying to use it.
            filter_collection = mari.gl_render.findPostFilterCollection('Color Space')
            if filter_collection is not None:
                mari.gl_render.deletePostFilterCollection(filter_collection)

            # Destroy the toolbar to prevent the user trying to use it.
            mari.app.deleteToolBar('Color Space')
