# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import platform
import sys

import tempfile
if sys.version_info.major < 3:
    import shutil

import PyOpenColorIO as OCIO
from UnitTestUtils import (SIMPLE_CONFIG, TEST_DATAFILES_DIR)

class ArchiveIsConfigArchivableTest(unittest.TestCase):

    def setUp(self):
        """
        Initialize the CONFIG
        """
        self.CONFIG = OCIO.Config().CreateFromStream(SIMPLE_CONFIG)
        # Since a working directory is needed to archive a config, setting a fake working directory 
        # in order to test the search paths and FileTransform source logic.
        if platform.system() == 'Windows':
            self.CONFIG.setWorkingDir('C:\\fake_working_dir')
        else:
            self.CONFIG.setWorkingDir('/fake_working_dir')
        self.CONFIG.validate()
    
    def tearDown(self):
        self.CONFIG = None

    def test_is_archivable(self):
        """
        Test different scenario for isArchivable
        """
        ###############################
        # Testing search paths
        ###############################

        # Valid search path.
        self.CONFIG.setSearchPath('luts')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts/myluts1')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts\myluts1')
        self.assertEqual(True, self.CONFIG.isArchivable())        

        # Valid Search path starting with './' or '.\'.
        self.CONFIG.setSearchPath('./myLuts')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('.\myLuts')
        self.assertEqual(True, self.CONFIG.isArchivable())

        # Valid search path starting with './' or '.\' and a context variable.
        self.CONFIG.setSearchPath('./$SHOT/myluts')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('.\$SHOT\myluts')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts/$SHOT')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts/$SHOT/luts1')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts\$SHOT:luts\$SHOT\luts2')
        self.assertEqual(True, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts\$SHOT\luts2')
        self.assertEqual(True, self.CONFIG.isArchivable())

        #
        # Illegal scenarios
        #

        # Illegal search path starting with '..'.
        self.CONFIG.setSearchPath('luts:../luts')
        self.assertEqual(False, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts:..\myLuts')
        self.assertEqual(False, self.CONFIG.isArchivable())

        # Illegal search path starting with a context variable.
        self.CONFIG.setSearchPath('luts:$SHOT')
        self.assertEqual(False, self.CONFIG.isArchivable())

        # Illegal search path with absolute path.
        self.CONFIG.setSearchPath('luts:/luts')
        self.assertEqual(False, self.CONFIG.isArchivable())

        self.CONFIG.setSearchPath('luts:/$SHOT')
        self.assertEqual(False, self.CONFIG.isArchivable())
        
        if platform.system() == 'Windows':
            self.CONFIG.clearSearchPaths()
            self.CONFIG.addSearchPath('C:\luts')
            self.assertEqual(False, self.CONFIG.isArchivable())

            self.CONFIG.clearSearchPaths()
            self.CONFIG.addSearchPath('C:\\')
            self.assertEqual(False, self.CONFIG.isArchivable())

            self.CONFIG.clearSearchPaths()
            self.CONFIG.addSearchPath('C:\$SHOT')
            self.assertEqual(False, self.CONFIG.isArchivable())


        ###############################
        # Testing FileTransfrom paths
        ###############################

        # Clear search paths so it doesn't affect the tests below.
        self.CONFIG.clearSearchPaths()

        # Function to facilitate adding a new FileTransform to a config.
        def addFTAndTestIsArchivable(cfg, path, isArchivable):
            fullPath = os.path.join(path, "fake_lut.clf")
            ft = OCIO.FileTransform()
            ft.setSrc(fullPath)

            cs = OCIO.ColorSpace()
            cs.setName("csTest")
            cs.setTransform(ft, OCIO.COLORSPACE_DIR_TO_REFERENCE)
            cfg.addColorSpace(cs)
            self.assertEqual(isArchivable, cfg.isArchivable())

            cfg.removeColorSpace("csTest")

        # Valid FileTransfrom path.
        addFTAndTestIsArchivable(self.CONFIG, "luts", True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts/myluts1', True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts\myluts1', True)

        # Valid Search path starting with './' or '.\'.
        addFTAndTestIsArchivable(self.CONFIG, './myLuts', True)
        addFTAndTestIsArchivable(self.CONFIG, '.\myLuts', True)

        # Valid search path starting with './' or '.\' and a context variable.
        addFTAndTestIsArchivable(self.CONFIG, './$SHOT/myluts', True)
        addFTAndTestIsArchivable(self.CONFIG, '.\$SHOT\myluts', True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts/$SHOT', True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts/$SHOT/luts1', True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts\$SHOT:luts\$SHOT\luts2', True)
        addFTAndTestIsArchivable(self.CONFIG, 'luts\$SHOT\luts2', True)

        #
        # Illegal scenarios
        #

        # Illegal search path starting with '..'.
        addFTAndTestIsArchivable(self.CONFIG, '../luts', False)
        addFTAndTestIsArchivable(self.CONFIG, '..\myLuts', False)

        # Illegal search path starting with a context variable.
        addFTAndTestIsArchivable(self.CONFIG, '$SHOT', False)

        # Illegal search path with absolute path.
        addFTAndTestIsArchivable(self.CONFIG, '/luts', False)
        addFTAndTestIsArchivable(self.CONFIG, '/$SHOT', False)

        if platform.system() == 'Windows':
            addFTAndTestIsArchivable(self.CONFIG, 'C:\luts', False)
            addFTAndTestIsArchivable(self.CONFIG, 'C:\\', False)
            addFTAndTestIsArchivable(self.CONFIG, 'C:\$SHOT', False)

class ArchiveContextTest(unittest.TestCase):

    def setUp(self):
        """
        Initialize the CONFIG file and CONTEXT for the tests.
        """
        ocioz_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'context_test1', 'context_test1_windows.ocioz'
            )
        )
        self.CONFIG = OCIO.Config().CreateFromFile(ocioz_file)

        # OCIO will pick up context vars from the environment that runs the test,
        # so set these explicitly, even though the config has default values.
        ctx = self.CONFIG.getCurrentContext()
        ctx['SHOT'] = 'none'
        ctx['LUT_PATH'] = 'none'
        ctx['CAMERA'] = 'none'
        ctx['CCCID'] = 'none'
        self.CONTEXT = ctx

    def tearDown(self):
        self.CONFIG = None
        self.CONTEXT = None

    def test_search_paths(self):
        """
        Test search path processing involving context variables.
        """
        # This is independent of the context.
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'shot1_lut1_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 10.)

        # This is independent of the context.
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'shot2_lut1_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 20.)

        # This is independent of the context.
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'shot2_lut2_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 2.)

        # This is independent of the context but the file is in a second level sub-directory.
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'lut3_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 3.)

        # This uses a context for the FileTransform src but is independent of the search_path.
        self.CONTEXT['LUT_PATH'] = 'shot3/lut1.clf'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'lut_path_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 30.)

        # The FileTransform src is ambiguous and the context configures the search_path.
        self.CONTEXT['SHOT'] = '.'      # (use the working directory)
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'plain_lut1_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 5.)

        # The FileTransform src is ambiguous and the context configures the search_path.
        self.CONTEXT['SHOT'] = 'shot2'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'plain_lut1_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 20.)

        # The FileTransform src is ambiguous and the context configures the search_path.
        self.CONTEXT['SHOT'] = 'no_shot'  # (path doesn't exist)
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'plain_lut1_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 10.)

        # This should be in the archive but is not on the search path at all without the context.
        self.CONTEXT['SHOT'] = 'no_shot'  # (path doesn't exist)
        with self.assertRaises(OCIO.ExceptionMissingFile):
            processor = self.CONFIG.getProcessor(self.CONTEXT, 'lut4_cs', 'reference')

        # This should be in the archive but is not on the search path at all without the context.
        self.CONTEXT['SHOT'] = 'shot4'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'lut4_cs', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 4.)

    def test_colorspace(self):
        """
        Test a color space where a ColorSpaceTransform uses a context variable as the src.
        """
        self.CONTEXT['CAMERA'] = 'sony'
        self.CONTEXT['SHOT'] = 'shot4'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'context_camera', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 4.)

        self.CONTEXT['CAMERA'] = 'arri'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 'context_camera', 'reference')
        mtx = processor.createGroupTransform()[0]
        self.assertEqual(mtx.getMatrix()[0], 3.)

    def test_cccid(self):
        """
        Test a Look that uses a context variable for the FileTransform cccid.
        """
        look_transform = OCIO.LookTransform('reference', 'reference', 'shot_look')

        self.CONTEXT['CCCID'] = 'look-02'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 
            look_transform, 
            OCIO.TRANSFORM_DIR_FORWARD)
        cdl = processor.createGroupTransform()[0]
        self.assertEqual(cdl.getSlope()[0], 0.9)

        self.CONTEXT['CCCID'] = 'look-03'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 
            look_transform, 
            OCIO.TRANSFORM_DIR_FORWARD)
        cdl = processor.createGroupTransform()[0]
        self.assertEqual(cdl.getSlope()[0], 1.2)

        self.CONTEXT['CCCID'] = 'look-01'
        processor = self.CONFIG.getProcessor(self.CONTEXT, 
            look_transform, 
            OCIO.TRANSFORM_DIR_FORWARD)
        cdl = processor.createGroupTransform()[0]
        self.assertEqual(cdl.getSlope()[0], 1.1)

class ArchiveAndExtractComparison(unittest.TestCase):
    def setUp(self):
        """
        Initialize the CONFIGs
        """
        original_config = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'context_test1', 'config.ocio'
            )
        )
        self.ORIGINAL_CONFIG = OCIO.Config().CreateFromFile(original_config)
        self.ORIGINAL_CONFIG.validate()

        self.ORIGNAL_ARCHIVED_CONFIG_PATH = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'context_test1', 'context_test1_windows.ocioz'
            )
        )
        self.ORIGNAL_ARCHIVED_CONFIG = OCIO.Config().CreateFromFile(
            self.ORIGNAL_ARCHIVED_CONFIG_PATH
        )
        self.ORIGNAL_ARCHIVED_CONFIG.validate()
    
    def tearDown(self):
        self.CONFIG = None

    def test_archive_config_and_compare_to_original(self):
        """
        Archive config X and create a new config object Y with it, and then compare X and Y.
        """
        # Testing CreateFromFile and archive method on a successful path.

        # 1 - Archive the original config
        temp_ocioz_file = tempfile.NamedTemporaryFile(delete = False)
        # Close temporary file to make sure that archive can use it.
        temp_ocioz_file.close()
        try:
            self.ORIGINAL_CONFIG.archive(str(temp_ocioz_file.name))

            # 2 - Create a config from the archived config in step 1.
            new_ocioz_config = OCIO.Config().CreateFromFile(str(temp_ocioz_file.name))
            new_ocioz_config.validate()

            # 3 - Compare config CacheIDs
            ctx = self.ORIGINAL_CONFIG.getCurrentContext()
            self.assertEqual(self.ORIGINAL_CONFIG.getCacheID(ctx), new_ocioz_config.getCacheID(ctx))

            # 4 - Compare a processor cacheID
            original_proc = self.ORIGINAL_CONFIG.getProcessor("plain_lut1_cs", "shot1_lut1_cs")
            new_proc = new_ocioz_config.getProcessor("plain_lut1_cs", "shot1_lut1_cs")
            self.assertEqual(original_proc.getCacheID(), new_proc.getCacheID())

            # 5 - Compare serialization
            self.assertEqual(self.ORIGINAL_CONFIG.serialize(), new_ocioz_config.serialize())
        finally:
            # Delete the temporary file.
            os.unlink(temp_ocioz_file.name)

    def test_extract_config_and_compare_to_original(self):
        """
        Extract an OCIOZ archive that came from config X and create a new Config object Y with it, 
        and then compare X and Y.
        """
        # Testing CreateFromFile and ExtractOCIOZArchive on a successful path.

        temp_dir_name = ""
        try:
            if sys.version_info.major >= 3:
                # Python 3.
                self.temp_ocioz_directory = tempfile.TemporaryDirectory()
                temp_dir_name = self.temp_ocioz_directory.name
            else:
                # Python 2.
                temp_dir_name = tempfile.mkdtemp() 

            # 1 - Extract the OCIOZ archive to temporary directory.
            OCIO.ExtractOCIOZArchive(self.ORIGNAL_ARCHIVED_CONFIG_PATH, temp_dir_name)

            # 2 - Create config from extracted OCIOZ archive.
            new_config = OCIO.Config().CreateFromFile(
                os.path.join(temp_dir_name, "config.ocio")
            )
            new_config.validate()

            # 3 - Compare config cacheID.
            ctx = self.ORIGNAL_ARCHIVED_CONFIG.getCurrentContext()
            self.assertEqual(
                self.ORIGNAL_ARCHIVED_CONFIG.getCacheID(ctx), 
                new_config.getCacheID(ctx)
            )

            # 4 - Compare a processor cacheID
            original_proc = self.ORIGNAL_ARCHIVED_CONFIG.getProcessor(
                "plain_lut1_cs", 
                "shot1_lut1_cs"
            )
            new_proc = new_config.getProcessor("plain_lut1_cs", "shot1_lut1_cs")
            self.assertEqual(original_proc.getCacheID(), new_proc.getCacheID())

            # 5 - Compare serialization
            self.assertEqual(self.ORIGNAL_ARCHIVED_CONFIG.serialize(), new_config.serialize())
        finally:
            if sys.version_info.major >= 3:
                # Python 3.
                self.temp_ocioz_directory.cleanup()
            else:
                # Python 2.
                if not temp_dir_name:
                    shutil.rmtree(temp_dir_name)