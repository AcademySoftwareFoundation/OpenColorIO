# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO

from UnitTestUtils import (TEST_DATAFILES_DIR, MuteLogging)

class ConfigMergingHelpersTest(unittest.TestCase):

    def test_ConfigMergingParameters_defaults(self):

        params = OCIO.ConfigMergingParameters.Create()
        self.assertIsInstance(params, OCIO.ConfigMergingParameters)
        self.assertEqual(params.getBaseConfigName(), "")
        self.assertEqual(params.getInputConfigName(), "")
        self.assertEqual(params.getOutputName(), "merged")
        self.assertEqual(params.getDefaultStrategy(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getInputFamilyPrefix(), "")
        self.assertEqual(params.getBaseFamilyPrefix(), "")
        self.assertTrue(params.isInputFirst())
        self.assertFalse(params.isErrorOnConflict())
        self.assertTrue(params.isAvoidDuplicates())
        self.assertTrue(params.isAdjustInputReferenceSpace())
        self.assertEqual(params.getName(), "")
        self.assertEqual(params.getDescription(), "")
        self.assertEqual(params.getNumEnvironmentVars(), 0)
        self.assertEqual(params.getSearchPath(), "")
        self.assertEqual(params.getActiveDisplays(), "")
        self.assertEqual(params.getActiveViews(), "")
        self.assertEqual(params.getInactiveColorSpaces(), "")
        self.assertEqual(params.getRoles(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getFileRules(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getDisplayViews(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getViewTransforms(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getLooks(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getColorspaces(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)
        self.assertEqual(params.getNamedTransforms(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_INPUT)

    def test_ConfigMergingParameters_setters(self):

        params = OCIO.ConfigMergingParameters.Create()
        params.setBaseConfigName("base")
        self.assertEqual(params.getBaseConfigName(), "base")
        params.setInputConfigName("input")
        self.assertEqual(params.getInputConfigName(), "input")
        params.setOutputName("merged")
        self.assertEqual(params.getOutputName(), "merged")
        params.setDefaultStrategy(OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        self.assertEqual(params.getDefaultStrategy(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        params.setInputFamilyPrefix("iprefix")
        self.assertEqual(params.getInputFamilyPrefix(), "iprefix")
        params.setBaseFamilyPrefix("bprefix")
        self.assertEqual(params.getBaseFamilyPrefix(), "bprefix")
        params.setInputFirst(False)
        self.assertFalse(params.isInputFirst())
        params.setErrorOnConflict(True)
        self.assertTrue(params.isErrorOnConflict())
        params.setAvoidDuplicates(False)
        self.assertFalse(params.isAvoidDuplicates())
        params.setAdjustInputReferenceSpace(False)
        self.assertFalse(params.isAdjustInputReferenceSpace())
        params.setName("name")
        self.assertEqual(params.getName(), "name")
        params.setDescription("desc")
        self.assertEqual(params.getDescription(), "desc")
        params.addEnvironmentVar("var1", "value 1")
        self.assertEqual(params.getNumEnvironmentVars(), 1)
        self.assertEqual(params.getEnvironmentVar(0), "var1")
        self.assertEqual(params.getEnvironmentVarValue(0), "value 1")
        params.addEnvironmentVar("var2", "value 2")
        self.assertEqual(params.getNumEnvironmentVars(), 2)
        self.assertEqual(params.getEnvironmentVar(1), "var2")
        self.assertEqual(params.getEnvironmentVarValue(1), "value 2")
        params.setSearchPath("path1:path2")
        self.assertEqual(params.getSearchPath(), "path1:path2")
        params.addSearchPath("path3:path4")
        self.assertEqual(params.getSearchPath(), "path1:path2:path3:path4")
        params.setActiveDisplays("disp1, disp2")
        self.assertEqual(params.getActiveDisplays(), "disp1, disp2")
        params.setActiveViews("view1, view2")
        self.assertEqual(params.getActiveViews(), "view1, view2")
        params.setInactiveColorSpaces("cs1, cs2, cs3")
        self.assertEqual(params.getInactiveColorSpaces(), "cs1, cs2, cs3")
        params.setRoles(OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        self.assertEqual(params.getRoles(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        params.setFileRules(OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        self.assertEqual(params.getFileRules(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        params.setDisplayViews(OCIO.ConfigMergingParameters.STRATEGY_INPUT_ONLY)
        self.assertEqual(params.getDisplayViews(), OCIO.ConfigMergingParameters.STRATEGY_INPUT_ONLY)
        params.setViewTransforms(OCIO.ConfigMergingParameters.STRATEGY_BASE_ONLY)
        self.assertEqual(params.getViewTransforms(), OCIO.ConfigMergingParameters.STRATEGY_BASE_ONLY)
        params.setLooks(OCIO.ConfigMergingParameters.STRATEGY_BASE_ONLY)
        self.assertEqual(params.getLooks(), OCIO.ConfigMergingParameters.STRATEGY_BASE_ONLY)
        params.setColorspaces(OCIO.ConfigMergingParameters.STRATEGY_REMOVE)
        self.assertEqual(params.getColorspaces(), OCIO.ConfigMergingParameters.STRATEGY_REMOVE)
        params.setNamedTransforms(OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        self.assertEqual(params.getNamedTransforms(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)

    def test_ConfigMerger_defaults(self):

        merger = OCIO.ConfigMerger.Create()
        self.assertEqual(merger.getNumSearchPaths(), 0)
        self.assertEqual(merger.getWorkingDir(), "")
        self.assertEqual(merger.getNumConfigMergingParameters(), 0)
        self.assertEqual(merger.getMajorVersion(), 1)
        self.assertEqual(merger.getMinorVersion(), 0)

    def test_ConfigMerger_setters(self):

        merger = OCIO.ConfigMerger.Create()
        merger.setSearchPath("/var/tmp")
        self.assertEqual(merger.getNumSearchPaths(), 1)
        self.assertEqual(merger.getSearchPath(0), "/var/tmp")
        merger.addSearchPath("/usr/local")
        self.assertEqual(merger.getNumSearchPaths(), 2)
        self.assertEqual(merger.getSearchPath(1), "/usr/local")
        merger.setSearchPath("/var/tmp")
        self.assertEqual(merger.getNumSearchPaths(), 1)
        self.assertEqual(merger.getSearchPath(0), "/var/tmp")

        merger.setWorkingDir("/var/tmp")
        self.assertEqual(merger.getWorkingDir(), "/var/tmp")
        self.assertEqual(merger.getNumConfigMergingParameters(), 0)
        self.assertEqual(merger.getMajorVersion(), 1)
        self.assertEqual(merger.getMinorVersion(), 0)

        params = OCIO.ConfigMergingParameters.Create()
        params.setBaseConfigName("base_name")
        merger.addParams(params)
        self.assertEqual(merger.getNumConfigMergingParameters(), 1)
        params2 = merger.getParams(0)
        self.assertEqual(params2.getBaseConfigName(), "base_name")
        merger.addParams(params)
        self.assertEqual(merger.getNumConfigMergingParameters(), 2)

        txt = merger.serialize()
        self.assertEqual(txt[0:18], "ociom_version: 1.0")

        merger.setVersion(2, 3)
        self.assertEqual(merger.getMajorVersion(), 2)
        self.assertEqual(merger.getMinorVersion(), 3)

    def test_ConfigMerger_parse(self):
        import os

        ociom_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'mergeconfigs', 'merged2', 'merged.ociom'
            )
        )
        merger = OCIO.ConfigMerger.CreateFromFile(ociom_file)
        self.assertEqual(merger.getNumConfigMergingParameters(), 2)

        params1 = merger.getParams(0)
        self.assertEqual(params1.getOutputName(), "Merge1")
        self.assertEqual(params1.getDefaultStrategy(), OCIO.ConfigMergingParameters.STRATEGY_PREFER_BASE)
        params2 = merger.getParams(1)
        self.assertEqual(params2.getOutputName(), "Merge2")
        self.assertEqual(params2.getDefaultStrategy(), OCIO.ConfigMergingParameters.STRATEGY_BASE_ONLY)
        self.assertEqual(params2.getDescription(), "Description override")

    def test_ExecuteMerger(self):
        import os

        ociom_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'mergeconfigs', 'merged2', 'merged.ociom'
            )
        )
        merger = OCIO.ConfigMerger.CreateFromFile(ociom_file)

        # Mute expected warnings from this merge in the log such as:
        # [OpenColorIO Warning]: The Input config contains a value that would override the Base config: file_rules: Default
        log = MuteLogging()
        with log:
            merger2 = merger.mergeConfigs()

        # The full merger consists of two merges (two sets of params).
        self.assertEqual(merger2.getNumMergedConfigs(), 2)

        # Test result of the first merge.
        cfg = merger2.getMergedConfig(0)
        cs = cfg.getColorSpace("ACES2065-1")
        self.assertEqual(cs.getFamily(), "ACES~Linear")
        self.assertEqual(cs.getDescription(), "from input")

        self.assertEqual(len(cfg.getRoles()), 2)
        self.assertEqual(cfg.getRoleColorSpace("cie_xyz_d65_interchange"), "ap0")

        # Test result of the second merge.
        cfg = merger2.getMergedConfig(1)
        self.assertEqual(len(cfg.getRoles()), 1)
        self.assertEqual(cfg.getRoleColorSpace("cie_xyz_d65_interchange"), "CIE-XYZ-D65")

    def test_MergeConfigs(self):
        import os

        ociom_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'mergeconfigs', 'merged2', 'merged.ociom'
            )
        )
        merger = OCIO.ConfigMerger.CreateFromFile(ociom_file)
        params = merger.getParams(0)

        base_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'mergeconfigs', 'merged2', 'base.ocio'
            )
        )
        base_cfg = OCIO.Config.CreateFromFile(base_file)
        input_file = os.path.normpath(
            os.path.join(
                TEST_DATAFILES_DIR, 'configs', 'mergeconfigs', 'merged2', 'input.ocio'
            )
        )
        input_cfg = OCIO.Config.CreateFromFile(input_file)

        # Mute expected warnings from this merge in the log such as:
        # [OpenColorIO Warning]: The Input config contains a value that would override the Base config: file_rules: Default
        log = MuteLogging()
        with log:
            cfg = OCIO.ConfigMergingHelpers.MergeConfigs(params, base_cfg, input_cfg)

        self.assertEqual(len(cfg.getRoles()), 2)
        self.assertEqual(cfg.getRoleColorSpace("cie_xyz_d65_interchange"), "ap0")

        cs = cfg.getColorSpace("ACES2065-1")
        self.assertEqual(cs.getFamily(), "ACES~Linear")
        self.assertEqual(cs.getDescription(), "from input")

    def test_MergeColorSpace(self):

        params = OCIO.ConfigMergingParameters.Create()
        base = OCIO.Config.CreateFromBuiltinConfig("cg-config-latest")
        cs = OCIO.ColorSpace(name="my_cs")
        cs.setTransform(OCIO.FixedFunctionTransform(OCIO.FIXED_FUNCTION_XYZ_TO_xyY), OCIO.COLORSPACE_DIR_TO_REFERENCE)

        cfg2 = OCIO.ConfigMergingHelpers.MergeColorSpace(params, base, cs)

        cs2 = cfg2.getColorSpace("my_cs")
        ff = cs2.getTransform(OCIO.COLORSPACE_DIR_TO_REFERENCE)
        self.assertEqual(ff.getStyle(), OCIO.FIXED_FUNCTION_XYZ_TO_xyY)


if __name__ == "__main__":
    unittest.main()
