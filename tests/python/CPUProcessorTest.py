# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import unittest

logger = logging.getLogger(__name__)

try:
    import numpy as np
except ImportError:
    logger.warning(
        "NumPy could not be imported. "
        "Test case will lack significant coverage!"
    )
    np = None

import PyOpenColorIO as OCIO


class CPUProcessorTest(unittest.TestCase):
    FLOAT_DELTA = 1e+5
    UINT_DELTA = 1

    @classmethod
    def setUpClass(cls):
        # ---------------------------------------------------------------------
        # OCIO objects
        # ---------------------------------------------------------------------
        cls.config = OCIO.Config()
        cls.proc_fwd = cls.config.getProcessor(
            OCIO.MatrixTransform.Scale([0.5, 0.5, 0.5, 1.0])
        )
        cls.proc_inv = cls.config.getProcessor(
            OCIO.MatrixTransform.Scale([2.0, 2.0, 2.0, 1.0])
        )

        # BIT_DEPTH_F32
        cls.default_cpu_proc_fwd = cls.proc_fwd.getDefaultCPUProcessor()
        cls.default_cpu_proc_inv = cls.proc_inv.getDefaultCPUProcessor()

        # BIT_DEPTH_F16
        cls.half_cpu_proc_fwd = cls.proc_fwd.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_F16, 
            OCIO.BIT_DEPTH_F16, 
            OCIO.OPTIMIZATION_DEFAULT
        )
        cls.half_cpu_proc_inv = cls.proc_inv.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_F16, 
            OCIO.BIT_DEPTH_F16, 
            OCIO.OPTIMIZATION_DEFAULT
        )

        # BIT_DEPTH_UINT16
        cls.uint16_cpu_proc_fwd = cls.proc_fwd.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_UINT16, 
            OCIO.BIT_DEPTH_UINT16, 
            OCIO.OPTIMIZATION_DEFAULT
        )
        cls.uint16_cpu_proc_inv = cls.proc_inv.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_UINT16, 
            OCIO.BIT_DEPTH_UINT16, 
            OCIO.OPTIMIZATION_DEFAULT
        )

        # BIT_DEPTH_UINT8
        cls.uint8_cpu_proc_fwd = cls.proc_fwd.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_UINT8, 
            OCIO.BIT_DEPTH_UINT8, 
            OCIO.OPTIMIZATION_DEFAULT
        )
        cls.uint8_cpu_proc_inv = cls.proc_inv.getOptimizedCPUProcessor(
            OCIO.BIT_DEPTH_UINT8, 
            OCIO.BIT_DEPTH_UINT8, 
            OCIO.OPTIMIZATION_DEFAULT
        )

        # ---------------------------------------------------------------------
        # Test image data
        # ---------------------------------------------------------------------
        if not np:
            # NumPy not found. Only test lists.
            log_range = [float(2**i) for i in range(-15, 15)]

            cls.float_rgb_list = (
                [-65504.0] + 
                [-x for x in reversed(log_range)] +
                [0.0] + 
                log_range + 
                [65504.0]
            )

            cls.float_rgba_list = list(cls.float_rgb_list)
            for i in reversed(range(3, len(cls.float_rgb_list)+1, 3)):
                cls.float_rgba_list.insert(i, 1.0)

            return

        log_range = np.logspace(-15, 15, 30, base=2)

        # BIT_DEPTH_F32
        cls.float_rgb_1d = np.concatenate((
            [-65504.0], 
            np.flipud(-log_range), 
            [0.0], 
            log_range, 
            [65504.0]
        )).astype(np.float32)
        cls.float_rgb_2d = cls.float_rgb_1d.reshape([21, 3])
        cls.float_rgb_3d = cls.float_rgb_1d.reshape([7, 3, 3])

        cls.float_rgba_1d = np.insert(
            cls.float_rgb_1d, 
            range(3, cls.float_rgb_1d.size+1, 3), 
            1.0
        )
        cls.float_rgba_2d = cls.float_rgba_1d.reshape([21, 4])
        cls.float_rgba_3d = cls.float_rgba_1d.reshape([7, 3, 4])

        cls.float_rgb_list = cls.float_rgb_1d.tolist()
        cls.float_rgba_list = cls.float_rgba_1d.tolist()

        # BIT_DEPTH_F16
        cls.half_rgb_1d = cls.float_rgb_1d.astype(np.float16)
        cls.half_rgb_2d = cls.float_rgb_2d.astype(np.float16)
        cls.half_rgb_3d = cls.float_rgb_3d.astype(np.float16)

        cls.half_rgba_1d = cls.float_rgba_1d.astype(np.float16)
        cls.half_rgba_2d = cls.float_rgba_2d.astype(np.float16)
        cls.half_rgba_3d = cls.float_rgba_3d.astype(np.float16)

        # BIT_DEPTH_UINT16
        cls.uint16_rgb_1d = np.linspace(
            0, 2**16-1, 
            num=63, 
            endpoint=True
        ).astype(np.uint16)
        cls.uint16_rgb_2d = cls.uint16_rgb_1d.reshape([21, 3])
        cls.uint16_rgb_3d = cls.uint16_rgb_1d.reshape([7, 3, 3])

        cls.uint16_rgba_1d = np.insert(
            cls.uint16_rgb_1d, 
            range(3, cls.uint16_rgb_1d.size+1, 3), 
            cls.uint16_rgb_1d.max()
        )
        cls.uint16_rgba_2d = cls.uint16_rgba_1d.reshape([21, 4])
        cls.uint16_rgba_3d = cls.uint16_rgba_1d.reshape([7, 3, 4])

        # BIT_DEPTH_UINT8
        cls.uint8_rgb_1d = np.linspace(
            0, 2**8-1, 
            num=63, 
            endpoint=True).astype(np.uint8)
        cls.uint8_rgb_2d = cls.uint8_rgb_1d.reshape([21, 3])
        cls.uint8_rgb_3d = cls.uint8_rgb_1d.reshape([7, 3, 3])

        cls.uint8_rgba_1d = np.insert(
            cls.uint8_rgb_1d, 
            range(3, cls.uint8_rgb_1d.size+1, 3), 
            cls.uint8_rgb_1d.max()
        )
        cls.uint8_rgba_2d = cls.uint8_rgba_1d.reshape([21, 4])
        cls.uint8_rgba_3d = cls.uint8_rgba_1d.reshape([7, 3, 4])

    def test_is_no_op(self):
        self.assertFalse(self.default_cpu_proc_fwd.isNoOp())
        self.assertFalse(self.default_cpu_proc_inv.isNoOp())
        self.assertFalse(self.half_cpu_proc_fwd.isNoOp())
        self.assertFalse(self.half_cpu_proc_inv.isNoOp())
        self.assertFalse(self.uint16_cpu_proc_fwd.isNoOp())
        self.assertFalse(self.uint16_cpu_proc_inv.isNoOp())
        self.assertFalse(self.uint8_cpu_proc_fwd.isNoOp())
        self.assertFalse(self.uint8_cpu_proc_inv.isNoOp())

        no_op_proc = self.config.getProcessor(OCIO.MatrixTransform.Identity())
        no_op_cpu_proc = no_op_proc.getDefaultCPUProcessor()
        self.assertTrue(no_op_cpu_proc.isNoOp())

    def test_is_identity(self):
        self.assertFalse(self.default_cpu_proc_fwd.isIdentity())
        self.assertFalse(self.default_cpu_proc_inv.isIdentity())
        self.assertFalse(self.half_cpu_proc_fwd.isIdentity())
        self.assertFalse(self.half_cpu_proc_inv.isIdentity())
        self.assertFalse(self.uint16_cpu_proc_fwd.isIdentity())
        self.assertFalse(self.uint16_cpu_proc_inv.isIdentity())
        self.assertFalse(self.uint8_cpu_proc_fwd.isIdentity())
        self.assertFalse(self.uint8_cpu_proc_inv.isIdentity())

        ident_proc = self.config.getProcessor(OCIO.MatrixTransform.Identity())
        ident_cpu_proc = ident_proc.getDefaultCPUProcessor()
        self.assertTrue(ident_cpu_proc.isIdentity())

    def test_has_channel_crosstalk(self):
        self.assertFalse(self.default_cpu_proc_fwd.hasChannelCrosstalk())
        self.assertFalse(self.default_cpu_proc_inv.hasChannelCrosstalk())
        self.assertFalse(self.half_cpu_proc_fwd.hasChannelCrosstalk())
        self.assertFalse(self.half_cpu_proc_inv.hasChannelCrosstalk())
        self.assertFalse(self.uint16_cpu_proc_fwd.hasChannelCrosstalk())
        self.assertFalse(self.uint16_cpu_proc_inv.hasChannelCrosstalk())
        self.assertFalse(self.uint8_cpu_proc_fwd.hasChannelCrosstalk())
        self.assertFalse(self.uint8_cpu_proc_inv.hasChannelCrosstalk())

        hsv_proc = self.config.getProcessor(
            OCIO.FixedFunctionTransform(OCIO.FIXED_FUNCTION_RGB_TO_HSV)
        )
        hsv_cpu_proc = hsv_proc.getDefaultCPUProcessor()
        self.assertTrue(hsv_cpu_proc.hasChannelCrosstalk())

    def test_cache_id(self):
        # Processor cache IDs differ
        self.assertNotEqual(
            self.default_cpu_proc_fwd.getCacheID(),
            self.default_cpu_proc_inv.getCacheID(),
        )

        # Re-creating the same processor has a mtching cache ID
        proc = self.config.getProcessor(
            OCIO.MatrixTransform.Scale([0.5, 0.5, 0.5, 1.0])
        )
        cpu_proc = proc.getDefaultCPUProcessor()

        self.assertEqual(
            self.default_cpu_proc_fwd.getCacheID(),
            cpu_proc.getCacheID()
        )

    def test_bit_depth(self):
        # BIT_DEPTH_F32
        self.assertEqual(
            self.default_cpu_proc_fwd.getInputBitDepth(), 
            OCIO.BIT_DEPTH_F32
        )
        self.assertEqual(
            self.default_cpu_proc_fwd.getOutputBitDepth(), 
            OCIO.BIT_DEPTH_F32
        )

        # BIT_DEPTH_F16
        self.assertEqual(
            self.half_cpu_proc_fwd.getInputBitDepth(), 
            OCIO.BIT_DEPTH_F16
        )
        self.assertEqual(
            self.half_cpu_proc_fwd.getOutputBitDepth(), 
            OCIO.BIT_DEPTH_F16
        )

        # BIT_DEPTH_UINT16
        self.assertEqual(
            self.uint16_cpu_proc_fwd.getInputBitDepth(), 
            OCIO.BIT_DEPTH_UINT16
        )
        self.assertEqual(
            self.uint16_cpu_proc_fwd.getOutputBitDepth(), 
            OCIO.BIT_DEPTH_UINT16
        )

        # BIT_DEPTH_UINT8
        self.assertEqual(
            self.uint8_cpu_proc_fwd.getInputBitDepth(), 
            OCIO.BIT_DEPTH_UINT8
        )
        self.assertEqual(
            self.uint8_cpu_proc_fwd.getOutputBitDepth(), 
            OCIO.BIT_DEPTH_UINT8
        )

    def test_dynamic_property(self):
        tr = OCIO.ExposureContrastTransform(
            style=OCIO.EXPOSURE_CONTRAST_LINEAR,
            dynamicExposure=True
        )

        proc = self.config.getProcessor(tr)
        cpu_proc = proc.getDefaultCPUProcessor()

        self.assertTrue(cpu_proc.isDynamic())
        self.assertTrue(cpu_proc.hasDynamicProperty(OCIO.DYNAMIC_PROPERTY_EXPOSURE))

        # Validate default +0 stops exposure
        self.assertEqual(
            cpu_proc.applyRGB([1.0, 1.0, 1.0]),
            [1.0, 1.0, 1.0]
        )

        # Change dynamic exposure to +1 stops and validate
        dyn_prop = cpu_proc.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_EXPOSURE)
        dyn_prop.setDouble(1.0)

        self.assertEqual(
            cpu_proc.applyRGB([1.0, 1.0, 1.0]),
            [2.0, 2.0, 2.0]
        )

    def test_apply(self):
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        # Wrap buffer in ImageDesc
        arr = self.float_rgb_3d.copy()
        image = OCIO.PackedImageDesc(arr, 7, 3, 3)

        # Forward transform modifies values in place
        self.default_cpu_proc_fwd.apply(image)

        for i in range(arr.size):
            self.assertAlmostEqual(
                arr.flat[i], 
                self.float_rgb_3d.flat[i] * 0.5,
                delta=self.FLOAT_DELTA
            )

        # Inverse transform roundtrips values in place
        self.default_cpu_proc_inv.apply(image)

        for i in range(arr.size):
            self.assertAlmostEqual(
                arr.flat[i], 
                self.float_rgb_3d.flat[i],
                delta=self.FLOAT_DELTA
            )

    def test_apply_src_dst(self):
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        # Wrap buffers in ImageDesc
        src_arr = self.float_rgb_3d.copy()
        src_image = OCIO.PackedImageDesc(src_arr, 7, 3, 3)
        dst_arr1 = np.zeros_like(self.float_rgb_3d)
        dst_image1 = OCIO.PackedImageDesc(dst_arr1, 7, 3, 3)
        dst_arr2 = np.zeros_like(self.float_rgb_3d)
        dst_image2 = OCIO.PackedImageDesc(dst_arr2, 7, 3, 3)

        # Forward transform modifies values in dst1 (src is unchanged)
        self.default_cpu_proc_fwd.apply(src_image, dst_image1)

        for i in range(src_arr.size):
            self.assertAlmostEqual(
                src_arr.flat[i],
                self.float_rgb_3d.flat[i],
                delta=self.FLOAT_DELTA
            )
            self.assertAlmostEqual(
                dst_arr1.flat[i],
                self.float_rgb_3d.flat[i] * 0.5,
                delta=self.FLOAT_DELTA
            )

        # Inverse transform roundtrips values in dst2 (dst1 is unchanged)
        self.default_cpu_proc_inv.apply(dst_image1, dst_image2)

        for i in range(src_arr.size):
            self.assertAlmostEqual(
                dst_arr1.flat[i],
                self.float_rgb_3d.flat[i] * 0.5,
                delta=self.FLOAT_DELTA
            )
            self.assertAlmostEqual(
                dst_arr2.flat[i], 
                self.float_rgb_3d.flat[i],
                delta=self.FLOAT_DELTA
            )

    def test_apply_rgb_list(self):
        # Forward transform returns modified values
        fwd_result = self.default_cpu_proc_fwd.applyRGB(self.float_rgb_list)

        for i in range(len(fwd_result)):
            self.assertAlmostEqual(
                fwd_result[i], 
                self.float_rgb_list[i] * 0.5,
                delta=self.FLOAT_DELTA
            )

        # Inverse transform returns roundtrip values
        inv_result = self.default_cpu_proc_inv.applyRGB(fwd_result)

        for i in range(len(fwd_result)):
            self.assertAlmostEqual(
                inv_result[i], 
                self.float_rgb_list[i],
                delta=self.FLOAT_DELTA
            )

    def test_apply_rgb_buffer(self):
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        for arr, cpu_proc_fwd, cpu_proc_inv in [
            (
                self.float_rgb_1d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.float_rgb_2d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.float_rgb_3d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.half_rgb_1d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.half_rgb_2d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.half_rgb_3d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.uint16_rgb_1d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint16_rgb_2d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint16_rgb_3d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint8_rgb_1d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
            (
                self.uint8_rgb_2d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
            (
                self.uint8_rgb_3d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
        ]:
            # Process duplicate array
            arr_copy = arr.copy()

            # Forward transform modifies values in place
            cpu_proc_fwd.applyRGB(arr_copy)

            for i in range(arr_copy.size):
                if arr.dtype in (np.float32, np.float16):
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i] * 0.5,
                        delta=self.FLOAT_DELTA
                    )
                else:
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i] // 2,
                        delta=self.UINT_DELTA
                    )

            # Inverse transform roundtrips values in place
            cpu_proc_inv.applyRGB(arr_copy)

            for i in range(arr_copy.size):
                if arr.dtype in (np.float32, np.float16):
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i],
                        delta=self.FLOAT_DELTA
                    )
                else:
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i],
                        delta=self.UINT_DELTA
                    )

    def test_apply_rgba_list(self):
        # Forward transform returns modified values
        fwd_result = self.default_cpu_proc_fwd.applyRGBA(self.float_rgba_list)

        for i in range(len(fwd_result)):
            if i % 4 == 3:
                # Alpha is unchanged
                self.assertEqual(fwd_result[i], self.float_rgba_list[i])
            else:
                self.assertAlmostEqual(
                    fwd_result[i], 
                    self.float_rgba_list[i] * 0.5,
                    delta=self.FLOAT_DELTA
                )

        # Inverse transform returns roundtrip values
        inv_result = self.default_cpu_proc_inv.applyRGBA(fwd_result)

        for i in range(len(fwd_result)):
            if i % 4 == 3:
                # Alpha is unchanged
                self.assertEqual(fwd_result[i], self.float_rgba_list[i])
            else:
                self.assertAlmostEqual(
                    inv_result[i], 
                    self.float_rgba_list[i],
                    delta=self.FLOAT_DELTA
                )

    def test_apply_rgba_buffer(self):
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        for arr, cpu_proc_fwd, cpu_proc_inv in [
            (
                self.float_rgba_1d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.float_rgba_2d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.float_rgba_3d, 
                self.default_cpu_proc_fwd, 
                self.default_cpu_proc_inv
            ),
            (
                self.half_rgba_1d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.half_rgba_2d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.half_rgba_3d, 
                self.half_cpu_proc_fwd, 
                self.half_cpu_proc_inv
            ),
            (
                self.uint16_rgba_1d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint16_rgba_2d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint16_rgba_3d, 
                self.uint16_cpu_proc_fwd, 
                self.uint16_cpu_proc_inv
            ),
            (
                self.uint8_rgba_1d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
            (
                self.uint8_rgba_2d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
            (
                self.uint8_rgba_3d, 
                self.uint8_cpu_proc_fwd, 
                self.uint8_cpu_proc_inv
            ),
        ]:
            # Process duplicate array
            arr_copy = arr.copy()

            # Forward transform modifies values in place
            cpu_proc_fwd.applyRGBA(arr_copy)

            for i in range(arr_copy.size):
                if i % 4 == 3:
                    # Alpha is unchanged
                    self.assertEqual(arr_copy.flat[i], arr.flat[i])
                elif arr.dtype in (np.float32, np.float16):
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i] * 0.5,
                        delta=self.FLOAT_DELTA
                    )
                else:
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i] // 2,
                        delta=self.UINT_DELTA
                    )

            # Inverse transform roundtrips values in place
            cpu_proc_inv.applyRGBA(arr_copy)

            for i in range(arr_copy.size):
                if i % 4 == 3:
                    # Alpha is unchanged
                    self.assertEqual(arr_copy.flat[i], arr.flat[i])
                elif arr.dtype in (np.float32, np.float16):
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i],
                        delta=self.FLOAT_DELTA
                    )
                else:
                    self.assertAlmostEqual(
                        arr_copy.flat[i], 
                        arr.flat[i],
                        delta=self.UINT_DELTA
                    )
