#-------------------------------------------------------------------------------
# Post processing (colour management) related Mari scripts
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
        self._input_colour_space  = ocio.colour_space_default
        self._output_colour_space = ocio.colour_space_default
        self._filter             = mari.gl_render.createQuickApplyGLSL('Colour Correction', '', '', 'ColourManager.png')
        self._filter_cache_id    = None
        self._texture_cache_id   = None
        self._sampler_name       = None

        self._filter.setMetadata('ConfigPath', self._config_file_list)
        self._filter.setMetadataDisplayName('ConfigPath', 'Configuration File')
        self._filter.setMetadataDefault('ConfigPath', ocio.CONFIG_FILE_LIST_RESET)
        self._filter.setMetadataFlags('ConfigPath', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

        colour_spaces = [colour_space.getName() for colour_space in self._config.getColourSpaces()]

        colour_space_reset = ocio.COLOUR_SPACE_RESET
        if colour_spaces.count(colour_space_reset) == 0:
            colour_space_reset = colour_spaces[0]

        self._filter.setMetadata('InputColourSpace', self._input_colour_space)
        self._filter.setMetadataDisplayName('InputColourSpace', 'Input Colour Space')
        self._filter.setMetadataDefault('InputColourSpace', colour_space_reset)
        self._filter.setMetadataItemList('InputColourSpace', colour_spaces)
        self._filter.setMetadataFlags('InputColourSpace', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

        self._filter.setMetadata('OutputColourSpace', self._output_colour_space)
        self._filter.setMetadataDisplayName('OutputColourSpace', 'Output Colour Space')
        self._filter.setMetadataDefault('OutputColourSpace', colour_space_reset)
        self._filter.setMetadataItemList('OutputColourSpace', colour_spaces)
        self._filter.setMetadataFlags('OutputColourSpace', self._filter.METADATA_VISIBLE | self._filter.METADATA_EDITABLE)

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

            colour_spaces = [colour_space.getName() for colour_space in self._config.getColourSpaces()]

            colour_space_reset = ocio.COLOUR_SPACE_RESET
            if colour_spaces.count(colour_space_reset) == 0:
                colour_space_reset = colour_spaces[0]

            if colour_spaces.count(self._input_colour_space) == 0:
                self._input_colour_space = colour_space_reset

            if colour_spaces.count(self._output_colour_space) == 0:
                self._output_colour_space = colour_space_reset

            self._filter.setMetadataItemList('InputColourSpace',  colour_spaces)
            self._filter.setMetadataItemList('OutputColourSpace', colour_spaces)
            self._filter.setMetadataDefault('InputColourSpace',  colour_space_reset)
            self._filter.setMetadataDefault('OutputColourSpace', colour_space_reset)
            self._filter.setMetadata('InputColourSpace',  self._input_colour_space )
            self._filter.setMetadata('OutputColourSpace', self._output_colour_space)

        elif name == 'InputColourSpace':
            if value == self._input_colour_space:
                return
            self._input_colour_space = value

        elif name == 'OutputColourSpace':
            if value == self._output_colour_space:
                return
            self._output_colour_space = value

        else:
            return

        self._rebuildFilter()

    #-----------------------------------------------------------------------------------------

    def _rebuildFilter(self):
        input_colour_space = self._config.getColourSpace(self._input_colour_space)
        if input_colour_space is not None:
            output_colour_space = self._config.getColourSpace(self._output_colour_space)
            if output_colour_space is not None:
                processor = self._config.getProcessor(input_colour_space, output_colour_space)

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
