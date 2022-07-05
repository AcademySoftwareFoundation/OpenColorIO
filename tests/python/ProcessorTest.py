# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os
import unittest
import PyOpenColorIO as OCIO

from UnitTestUtils import TEST_DATAFILES_DIR

class ProcessorTest(unittest.TestCase):

    def test_is_noop(self):
        # Test isNoOp() function.

        cfg = OCIO.Config().CreateRaw()
        ec = OCIO.ExposureContrastTransform()

        p = cfg.getProcessor(ec)
        self.assertTrue(p.isNoOp())

        # Make the transform dynamic.
        ec.makeContrastDynamic()
        p = cfg.getProcessor(ec)
        self.assertFalse(p.isNoOp())

    def test_has_channel_crosstalk(self):
        # Test hasChannelCrosstalk() function.

        cfg = OCIO.Config().CreateRaw()
        m = OCIO.MatrixTransform([1., 0., 0., 0.,
                                  0., 1., 0., 0.,
                                  0., 0., 2., 0.,
                                  0., 0., 0., 1.])

        p = cfg.getProcessor(m)
        self.assertFalse(p.hasChannelCrosstalk())

        m = OCIO.MatrixTransform([1., 0., 0., 0.,
                                  1., 1., 0., 0.,
                                  0., 0., 2., 0.,
                                  0., 0., 0., 1.])
        p = cfg.getProcessor(m)
        self.assertTrue(p.hasChannelCrosstalk())

    def test_cache_id(self):
        # Test getCacheID() function.
        cfg = OCIO.Config().CreateRaw()
        ec = OCIO.ExposureContrastTransform()

        # Make the transform dynamic.
        ec.makeContrastDynamic()
        p = cfg.getProcessor(ec)
        self.assertEqual(p.getCacheID(), '818420192074e6466468d59c9ea38667')

    def test_create_group_transform(self):
        # Test createGroupTransform() function.

        cfg = OCIO.Config().CreateRaw()
        group = OCIO.GroupTransform()
        m = OCIO.MatrixTransform(offset = [1., 1., 1., 0.])
        group.appendTransform(m)

        p = cfg.getProcessor(group)
        procGroup = p.createGroupTransform()
        self.assertEqual(len(procGroup), 1)
        iterProcGroup = iter(procGroup)
        t1 = next(iterProcGroup)
        self.assertEqual(t1.getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)

        ec = OCIO.ExposureContrastTransform(exposure=1.1, contrast=1.2)
        group.appendTransform(ec)
        p = cfg.getProcessor(group)
        procGroup = p.createGroupTransform()
        self.assertEqual(len(procGroup), 2)
        iterProcGroup = iter(procGroup)
        t1 = next(iterProcGroup)
        self.assertEqual(t1.getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)
        t2 = next(iterProcGroup)
        self.assertEqual(t2.getTransformType(),
                         OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)

    def test_dynamic_properties(self):
        # Test dynamic property related functions.

        cfg = OCIO.Config().CreateRaw()
        ec = OCIO.ExposureContrastTransform()
        ec.setContrast(1.1)
        ec.setExposure(1.2)
        p = cfg.getProcessor(ec)
        # Processor created from non-dynamic transform is not dynamic.
        self.assertFalse(p.isDynamic())

        # Make the transform dynamic.
        ec.makeContrastDynamic()
        p = cfg.getProcessor(ec)
        # Processor is dynamic.
        self.assertTrue(p.isDynamic())
        # Contrast is dynamic.
        self.assertTrue(p.hasDynamicProperty(OCIO.DYNAMIC_PROPERTY_CONTRAST))
        # Exposure is not dynamic.
        self.assertFalse(p.hasDynamicProperty(OCIO.DYNAMIC_PROPERTY_EXPOSURE))
        # Only dynamic Dynamic Properties (i.e. isDynamic returns true) may be modified after the
        # processor is created. As Dynamic Properties are decoupled between instances and
        # processor types, the value change only affects this instance so a corresponding CPU
        # processing instance will not have the new value.
        dpc = p.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_CONTRAST)
        # Accessing a Dynamic Property that does not exist triggers an exception.
        with self.assertRaises(OCIO.Exception):
            p.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_EXPOSURE)

        self.assertEqual(dpc.getType(), OCIO.DYNAMIC_PROPERTY_CONTRAST)
        # Initial contrast value.
        self.assertEqual(dpc.getDouble(), 1.1)
        dpc.setDouble(1.15)
        self.assertEqual(dpc.getDouble(), 1.15)
        # Value can be accessed only using the appropriate accessor. DYNAMIC_PROPERTY_CONTRAST is
        # a double, so get/setDouble should be used. Trying to get a different type throws an
        # error.
        with self.assertRaises(OCIO.Exception):
            dpc.getGradingPrimary()

    def test_optimize_processor(self):
        # Test getOptimizedProcessor() function.

        cfg = OCIO.Config().CreateRaw()
        group = OCIO.GroupTransform()
        m = OCIO.MatrixTransform(offset = [1., 1., 1., 0.])
        group.appendTransform(m)
        m = OCIO.MatrixTransform(offset = [2., 1., 0.5, 0.])
        group.appendTransform(m)

        p = cfg.getProcessor(group)
        # Optimize processor.
        pOptimized = p.getOptimizedProcessor(OCIO.OPTIMIZATION_LOSSLESS)
        procGroup = pOptimized.createGroupTransform()

        self.assertEqual(len(procGroup), 1)
        iterProcGroup = iter(procGroup)
        t1 = next(iterProcGroup)
        self.assertEqual(t1.getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)
        self.assertEqual(t1.getOffset(), [3, 2, 1.5, 0])

    def test_format_meta_data(self):
        # Test FormatMetadata related functions.

        cfg = OCIO.Config.CreateRaw()
        test_file = os.path.join(TEST_DATAFILES_DIR, 'clf', 'xyz_to_rgb.clf')
        file_tr = OCIO.FileTransform(src=test_file)
        proc = cfg.getProcessor(file_tr)
        # Remove the no-ops, since they are useless here.
        proc = proc.getOptimizedProcessor(OCIO.OPTIMIZATION_NONE)

        # For CTF, ProcessList metadata is added to processor FormatMetadata.
        proc_fmd = proc.getFormatMetadata()
        self.assertEqual(proc_fmd.getID(), '1d399a07-1936-49b8-9225-e6824790358c')
        proc_fmd_children = proc_fmd.getChildElements()
        self.assertEqual(len(proc_fmd_children), 1)
        child = next(proc_fmd_children)
        self.assertEqual(child.getElementName(), 'Description')
        self.assertEqual(child.getElementValue(),
                         'Example with a Matrix and Lut1D')

        # Each transform in the CTF file has a FormatMetadata. There are 3 transforms in this file.
        proc_tfmd = proc.getTransformFormatMetadata()
        self.assertEqual(len(proc_tfmd), 3)

        tfmd = next(proc_tfmd)
        self.assertEqual(tfmd.getID(), '72f85641-8c80-4bd6-b96b-a2f67ccb948a')
        tfmd_children = tfmd.getChildElements()
        self.assertEqual(len(tfmd_children), 1)
        child = next(tfmd_children)
        self.assertEqual(child.getElementName(), 'Description')
        self.assertEqual(child.getElementValue(),
                         'XYZ to sRGB matrix')

        tfmd = next(proc_tfmd)

        tfmd = next(proc_tfmd)
        self.assertEqual(tfmd.getID(), '4ed2af07-9319-430a-b18a-43368163c808')
        tfmd_children = tfmd.getChildElements()
        self.assertEqual(len(tfmd_children), 1)
        child = next(tfmd_children)
        self.assertEqual(child.getElementName(), 'Description')
        self.assertEqual(child.getElementValue(),
                         'x^1/1.8, with 0.95 and 0.9 scaling for G and B')
