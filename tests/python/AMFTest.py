# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os
import unittest

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_DATAFILES_DIR


# Canonical ACES AMF samples (from the aces-amf library) that use ACES 2.0
# transform IDs and resolve against the ACES 2 studio builtin config. See
# tests/data/files/amf/README.md.
VALID_ACES2_SAMPLES = [
    '01_minimal.amf',
    '02_idt_only_vfx_pull.amf',
    '03_idt_odt_viewing_pipeline.amf',
    '04_basic_grading_single_lmt.amf',
    '05_complex_grading_multiple_lmts.amf',
    '06_cdl_in_acescct.amf',
    '07_lut_based_look.amf',
    '08_mixed_cdl_and_lut.amf',
    '09_applied_non_applied_sequence.amf',
    '10_clip_specific_metadata.amf',
    '11_multi_author.amf',
    '12_primary_with_archived_looks.amf',
    '13_url_encoded_filenames.amf',
    '14_complex_metadata.amf',
    '15_hdr_pipeline_p3_st2084.amf',
    '16_gamut_compress_lmt.amf',
    '24_aces_2_0_basic.amf',
    '25_aces_2_0_with_lmt.amf',
    '30_cdl_reference_from_ccc.amf',
    '35_aces_2_0_full_grading.amf',
    '36_aces_2_0_hdr_p3.amf',
    '37_aces_2_0_multiple_outputs.amf',
]


class AMFTest(unittest.TestCase):

    def _amf(self, name):
        return os.path.join(TEST_DATAFILES_DIR, 'amf', name)

    def test_canonical_aces2_samples(self):
        """
        Every canonical ACES 2.0 AMF sample loads, produces a valid config with
        an active display/view, and builds a usable processor.
        """
        for sample in VALID_ACES2_SAMPLES:
            with self.subTest(sample=sample):
                config, info = OCIO.Config.CreateFromAMF(self._amf(sample))
                config.validate()
                self.assertTrue(info.inputColorSpaceName)

                # AMFInfo reports the resolved display/view; the pipeline must
                # build a processor end to end.
                self.assertTrue(info.displayName)
                self.assertTrue(info.viewName)

                # 01/02 have no output transform, so display/view fall back to
                # the config default ("None"). Every other sample must resolve
                # a real output display, guarding against a dropped-resolution
                # regression that silently falls back.
                if sample not in ('01_minimal.amf', '02_idt_only_vfx_pull.amf'):
                    self.assertNotEqual(info.displayName, 'None')

                dvt = OCIO.DisplayViewTransform(src=info.inputColorSpaceName,
                                                display=info.displayName,
                                                view=info.viewName)
                self.assertIsNotNone(config.getProcessor(dvt))

    def test_rejects_amf_v1_schema(self):
        """Only AMF schema v2.0+ is supported; a v1.0 document is rejected."""
        with self.assertRaises(OCIO.Exception):
            OCIO.Config.CreateFromAMF(self._amf('negative_v1_schema.amf'))

    def test_rejects_aces1_transforms(self):
        """
        A v2.0-schema AMF that references ACES 1.x transform IDs fails to load,
        because those transforms are not in the ACES 2 builtin config.
        """
        with self.assertRaises(OCIO.Exception):
            OCIO.Config.CreateFromAMF(self._amf('26_aces_1_3_explicit.amf'))

    def test_missing_file_raises(self):
        with self.assertRaises(OCIO.Exception):
            OCIO.Config.CreateFromAMF(self._amf('does_not_exist.amf'))
