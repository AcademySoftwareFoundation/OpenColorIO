#-------------------------------------------------------------------------------
# Post processing (color management) related Mari scripts
# coding: utf-8
# Copyright (c) 2011 The Foundry Visionmongers Ltd.  All Rights Reserved.
#-------------------------------------------------------------------------------

import mari, time, PythonQt, os, math
ocio = mari.utils.ocio

##############################################################################################

filter = None

class OcioFilter():

    #-----------------------------------------------------------------------------------------

    def __init__(self):
        # Default all members...
        self._config_file_list   = mari.FileList(ocio.config_file_list_default)
        self._config             = ocio.config_default
        self._input_color_space  = ocio.color_space_default
        self._output_color_space = ocio.color_space_default
        self._filter             = mari.gl_render.createQuickApplyGLSL('Color Correction', '', '', 'ColorManager.png')
        self._filter_cache_id    = None
        self._texture_cache_id   = None
        self._sampler_name       = None

        self._filter.setMetadata('ConfigPath', self._config_file_list)
        self._filter.setMetadataDisplayName('ConfigPath', 'Configuration File')
        self._filter.setMetadataDefault('ConfigPath', ocio.CONFIG_FILE_LIST_RESET)
        self._filter.setMetadataFlags('ConfigPath', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

        color_spaces = [color_space.getName() for color_space in self._config.getColorSpaces()]

        color_space_reset = ocio.COLOR_SPACE_RESET
        if color_spaces.count(color_space_reset) == 0:
            color_space_reset = color_spaces[0]

        self._filter.setMetadata('InputColorSpace', self._input_color_space)
        self._filter.setMetadataDisplayName('InputColorSpace', 'Input Color Space')
        self._filter.setMetadataDefault('InputColorSpace', color_space_reset)
        self._filter.setMetadataItemList('InputColorSpace', color_spaces)
        self._filter.setMetadataFlags('InputColorSpace', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

        self._filter.setMetadata('OutputColorSpace', self._output_color_space)
        self._filter.setMetadataDisplayName('OutputColorSpace', 'Output Color Space')
        self._filter.setMetadataDefault('OutputColorSpace', color_space_reset)
        self._filter.setMetadataItemList('OutputColorSpace', color_spaces)
        self._filter.setMetadataFlags('OutputColorSpace', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

        mari.utils.connect(self._filter.metadataValueChanged, self._metadataValueChanged)

        self._rebuildFilter()

    #-----------------------------------------------------------------------------------------

    def _metadataValueChanged(self, name, value):
        ocio.printMessage(ocio.MessageType.DEBUG, 'Metadata \'%s\' changed to \'%s\'' % (name, value))

        if name == 'ConfigPath':
            if value == self._config_file_list:
                return
            self._config_file_list = mari.FileList(value)
            self._config           = ocio.loadConfig(self._config_file_list.at(0), False)

            color_spaces = [color_space.getName() for color_space in self._config.getColorSpaces()]

            color_space_reset = ocio.COLOR_SPACE_RESET
            if color_spaces.count(color_space_reset) == 0:
                color_space_reset = color_spaces[0]

            if color_spaces.count(self._input_color_space) == 0:
                self._input_color_space = color_space_reset

            if color_spaces.count(self._output_color_space) == 0:
                self._output_color_space = color_space_reset

            self._filter.setMetadataItemList('InputColorSpace',  color_spaces)
            self._filter.setMetadataItemList('OutputColorSpace', color_spaces)
            self._filter.setMetadataDefault('InputColorSpace',  color_space_reset)
            self._filter.setMetadataDefault('OutputColorSpace', color_space_reset)
            self._filter.setMetadata('InputColorSpace',  self._input_color_space )
            self._filter.setMetadata('OutputColorSpace', self._output_color_space)

        elif name == 'InputColorSpace':
            if value == self._input_color_space:
                return
            self._input_color_space = value

        elif name == 'OutputColorSpace':
            if value == self._output_color_space:
                return
            self._output_color_space = value

        else:
            return

        self._rebuildFilter()

    #-----------------------------------------------------------------------------------------

    def _rebuildFilter(self):
        input_color_space = self._config.getColorSpace(self._input_color_space)
        if input_color_space is not None:
            output_color_space = self._config.getColorSpace(self._output_color_space)
            if output_color_space is not None:
                processor = self._config.getProcessor(input_color_space, output_color_space)

                self._filter_cache_id, self._texture_cache_id, self._sampler_name = ocio.buildProcessorFilter(
                    processor,
                    self._filter,
                    self._filter_cache_id,
                    self._texture_cache_id)

                current_canvas = mari.canvases.current()
                if current_canvas is not None:
                    current_canvas.repaint()

##############################################################################################

if mari.app.isRunning():
    if not hasattr(mari.gl_render, 'createQuickApplyGLSL'):
        printMessage(MessageType.ERROR, 'This version of Mari does not support the mari.gl_render.createQuickApplyGLSL API')

    elif ocio is not None:
        filter = OcioFilter()
