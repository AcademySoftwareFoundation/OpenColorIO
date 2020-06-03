# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class TransformsTest(unittest.TestCase):

    def test_interface(self):

        ### AllocationTransform ###
        at = OCIO.AllocationTransform()
        self.assertEqual(OCIO.ALLOCATION_UNIFORM, at.getAllocation())
        at.setAllocation(OCIO.ALLOCATION_LG2)
        self.assertEqual(OCIO.ALLOCATION_LG2, at.getAllocation())
        self.assertEqual(0, at.getNumVars())
        at.setVars([0.1, 0.2, 0.3])
        self.assertEqual(3, at.getNumVars())
        newvars = at.getVars()
        self.assertAlmostEqual(0.2, newvars[1], delta=1e-8)

        at2 = OCIO.AllocationTransform(OCIO.ALLOCATION_LG2,
                                       [0.1, 0.2, 0.3],
                                       OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(OCIO.ALLOCATION_LG2, at2.getAllocation())
        self.assertEqual(3, at2.getNumVars())
        newvars2 = at2.getVars()
        for i in range(0, 3):
            self.assertAlmostEqual(float(i+1)/10.0, newvars2[i], delta=1e-7)
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, at2.getDirection())

        at3 = OCIO.AllocationTransform(allocation=OCIO.ALLOCATION_LG2,
                                       vars=[0.1, 0.2, 0.3],
                                       direction=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(OCIO.ALLOCATION_LG2, at3.getAllocation())
        self.assertEqual(3, at3.getNumVars())
        newvars3 = at3.getVars()
        for i in range(0, 3):
            self.assertAlmostEqual(float(i+1)/10.0, newvars3[i], delta=1e-7)
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, at3.getDirection())

        ### Base Transform method tests ###
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, at.getDirection())
        at.setDirection(OCIO.TRANSFORM_DIR_UNKNOWN)
        self.assertEqual(OCIO.TRANSFORM_DIR_UNKNOWN, at.getDirection())

        ### CDLTransform ###
        cdl = OCIO.CDLTransform()
        CC = "<ColorCorrection id=\"foo\">\n"
        CC += "    <SOPNode>\n"
        CC += "        <Description>this is a description</Description>\n"
        CC += "        <Slope>1.1 1.2 1.3</Slope>\n"
        CC += "        <Offset>2.1 2.2 2.3</Offset>\n"
        CC += "        <Power>3.1 3.2 3.3</Power>\n"
        CC += "    </SOPNode>\n"
        CC += "    <SatNode>\n"
        CC += "        <Saturation>0.7</Saturation>\n"
        CC += "    </SatNode>\n"
        CC += "</ColorCorrection>"
        # Don't want to deal with getting the correct path so this runs
        #cdlfile = OCIO.CDLTransform().CreateFromFile("../OpenColorIO/src/jniglue/tests/org/OpenColorIO/test.cc", "foo")
        #self.assertEqual(CC, cdlfile.getXML())
        cdl.setXML(CC)
        self.assertEqual(CC, cdl.getXML())
        match = cdl.createEditableCopy()
        match.setOffset([1.0, 1.0, 1.0])
        self.assertEqual(False, cdl.equals(match))
        cdl.setSlope([0.1, 0.2, 0.3])
        cdl.setOffset([1.1, 1.2, 1.3])
        cdl.setPower([2.1, 2.2, 2.3])
        cdl.setSat(0.5)
        CC2 = "<ColorCorrection id=\"foo\">\n"
        CC2 += "    <SOPNode>\n"
        CC2 += "        <Description>this is a description</Description>\n"
        CC2 += "        <Slope>0.1 0.2 0.3</Slope>\n"
        CC2 += "        <Offset>1.1 1.2 1.3</Offset>\n"
        CC2 += "        <Power>2.1 2.2 2.3</Power>\n"
        CC2 += "    </SOPNode>\n"
        CC2 += "    <SatNode>\n"
        CC2 += "        <Saturation>0.5</Saturation>\n"
        CC2 += "    </SatNode>\n"
        CC2 += "</ColorCorrection>"
        self.assertEqual(CC2, cdl.getXML())
        cdl.setSOP([1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9])
        newsop = cdl.getSOP()
        self.assertAlmostEqual(1.5, newsop[4], delta=1e-8)
        slope = cdl.getSlope()
        self.assertAlmostEqual(1.2, slope[1], delta=1e-7)
        offset = cdl.getOffset()
        self.assertAlmostEqual(1.6, offset[2], delta=1e-7)
        power = cdl.getPower()
        self.assertAlmostEqual(1.7, power[0], delta=1e-7)
        self.assertAlmostEqual(0.5, cdl.getSat(), delta=1e-8)
        luma = cdl.getSatLumaCoefs()
        self.assertAlmostEqual(0.2126, luma[0], delta=1e-8)
        self.assertAlmostEqual(0.7152, luma[1], delta=1e-8)
        self.assertAlmostEqual(0.0722, luma[2], delta=1e-8)
        cdl.setID("foobar123")
        self.assertEqual("foobar123", cdl.getID())
        cdl.setDescription("bar")
        self.assertEqual("bar", cdl.getDescription())

        cdl2 = OCIO.CDLTransform([0.1, 0.2, 0.3],
                                 [1.1, 1.2, 1.3],
                                 [2.1, 2.2, 2.3],
                                 0.5,
                                 OCIO.TRANSFORM_DIR_INVERSE,
                                 'foobar123', 'bar')
        slope2 = cdl2.getSlope()
        offset2 = cdl2.getOffset()
        power2 = cdl2.getPower()
        luma2 = cdl2.getSatLumaCoefs()
        for i in range(0, 3):
            self.assertAlmostEqual(float(i+1)/10.0, slope2[i], delta=1e-7)
            self.assertAlmostEqual(float(i+1)/10.0+1, offset2[i], delta=1e-7)
            self.assertAlmostEqual(float(i+1)/10.0+2, power2[i], delta=1e-7)
        self.assertAlmostEqual(0.5, cdl2.getSat(), delta=1e-8)
        self.assertAlmostEqual(0.2126, luma2[0], delta=1e-8)
        self.assertAlmostEqual(0.7152, luma2[1], delta=1e-8)
        self.assertAlmostEqual(0.0722, luma2[2], delta=1e-8)
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, cdl2.getDirection())
        self.assertEqual('foobar123', cdl2.getID())
        self.assertEqual('bar', cdl2.getDescription())

        cdl3 = OCIO.CDLTransform(slope=[0.1, 0.2, 0.3],
                                 offset=[1.1, 1.2, 1.3],
                                 power=[2.1, 2.2, 2.3],
                                 sat=0.5,
                                 direction=OCIO.TRANSFORM_DIR_INVERSE,
                                 id='foobar123', description='bar')
        slope3 = cdl2.getSlope()
        offset3 = cdl2.getOffset()
        power3 = cdl2.getPower()
        luma3 = cdl2.getSatLumaCoefs()
        for i in range(0, 3):
            self.assertAlmostEqual(float(i+1)/10.0, slope3[i], delta=1e-7)
            self.assertAlmostEqual(float(i+1)/10.0+1, offset3[i], delta=1e-7)
            self.assertAlmostEqual(float(i+1)/10.0+2, power3[i], delta=1e-7)
        self.assertAlmostEqual(0.5, cdl3.getSat(), delta=1e-8)
        self.assertAlmostEqual(0.2126, luma3[0], delta=1e-8)
        self.assertAlmostEqual(0.7152, luma3[1], delta=1e-8)
        self.assertAlmostEqual(0.0722, luma3[2], delta=1e-8)
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, cdl3.getDirection())
        self.assertEqual('foobar123', cdl3.getID())
        self.assertEqual('bar', cdl3.getDescription())

        ### ColorSpaceTransform ###
        ct = OCIO.ColorSpaceTransform()
        ct.setSrc("foo")
        self.assertEqual("foo", ct.getSrc())
        ct.setDst("bar")
        self.assertEqual("bar", ct.getDst())

        ### DisplayTransform ###
        dt = OCIO.DisplayTransform()
        dt.setInputColorSpaceName("lin18")
        self.assertEqual("lin18", dt.getInputColorSpaceName())
        dt.setLinearCC(ct)
        foo = dt.getLinearCC()
        dt.setColorTimingCC(cdl)
        blah = dt.getColorTimingCC()
        dt.setChannelView(at)
        wee = dt.getChannelView()
        dt.setDisplay("sRGB")
        self.assertEqual("sRGB", dt.getDisplay())
        dt.setView("foobar")
        self.assertEqual("foobar", dt.getView())
        cdl.setXML(CC)
        dt.setDisplayCC(cdl)
        cdldt = dt.getDisplayCC()
        self.assertEqual(CC, cdldt.getXML())
        dt.setLooksOverride("darkgrade")
        self.assertEqual("darkgrade", dt.getLooksOverride())
        dt.setLooksOverrideEnabled(True)
        self.assertEqual(True, dt.getLooksOverrideEnabled())

        dt2 = OCIO.DisplayTransform("lin18", "sRGB", "foobar",
                                    OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual("lin18", dt2.getInputColorSpaceName())
        self.assertEqual("sRGB", dt2.getDisplay())
        self.assertEqual("foobar", dt2.getView())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, dt2.getDirection())

        dt3 = OCIO.DisplayTransform(inputColorSpaceName="lin18",
                                    display="sRGB", view="foobar",
                                    direction=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual("lin18", dt3.getInputColorSpaceName())
        self.assertEqual("sRGB", dt3.getDisplay())
        self.assertEqual("foobar", dt3.getView())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, dt3.getDirection())

        ### ExponentTransform ###
        et = OCIO.ExponentTransform()
        et.setValue([0.1, 0.2, 0.3, 0.4])
        evals = et.getValue()
        self.assertAlmostEqual(0.3, evals[2], delta=1e-7)

        ### FileTransform ###
        ft = OCIO.FileTransform()
        ft.setSrc("foo")
        self.assertEqual("foo", ft.getSrc())
        ft.setCCCId("foobar")
        self.assertEqual("foobar", ft.getCCCId())
        ft.setInterpolation(OCIO.INTERP_NEAREST)
        self.assertEqual(OCIO.INTERP_NEAREST, ft.getInterpolation())
        self.assertEqual(24, ft.getNumFormats())
        self.assertEqual("flame", ft.getFormatNameByIndex(0))
        self.assertEqual("3dl", ft.getFormatExtensionByIndex(0))

        ### GroupTransform ###
        gt = OCIO.GroupTransform()
        gt.appendTransform(et)
        gt.appendTransform(ft)
        self.assertEqual(2, gt.getNumTransforms())
        foo = gt.getTransform(0)
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, foo.getDirection())

        ### LogTransform ###
        lt = OCIO.LogTransform()
        lt.setBase(10.0)
        self.assertEqual(10.0, lt.getBase())

        ### LookTransform ###
        lkt = OCIO.LookTransform()
        lkt.setSrc("foo")
        self.assertEqual("foo", lkt.getSrc())
        lkt.setDst("bar")
        self.assertEqual("bar", lkt.getDst())
        lkt.setLooks("bar;foo")
        self.assertEqual("bar;foo", lkt.getLooks())

        ### MatrixTransform ###
        mt = OCIO.MatrixTransform()
        mmt = mt.createEditableCopy()
        mt.setMatrix([0.1, 0.2, 0.3, 0.4,
                     0.5, 0.6, 0.7, 0.8,
                     0.9, 1.0, 1.1, 1.2,
                     1.3, 1.4, 1.5, 1.6])
        mt.setOffset([0.1, 0.2, 0.3, 0.4])
        self.assertEqual(False, mt.equals(mmt))
        m44_1 = mt.getMatrix()
        offset_1 = mt.getOffset()
        self.assertAlmostEqual(0.3, m44_1[2], delta=1e-7)
        self.assertAlmostEqual(0.2, offset_1[1], delta=1e-7)
        m44_2, offset_2 = mt.Fit([0.1, 0.1, 0.1, 0.1],
                                 [0.9, 0.9, 0.9, 0.9],
                                 [0.0, 0.0, 0.0, 0.0],
                                 [1.1, 1.1, 1.1, 1.1])

        self.assertAlmostEqual(1.375, m44_2[0], delta=1e-7)
        m44_3, offset_2 = mt.Identity()
        self.assertAlmostEqual(0.0, m44_3[1], delta=1e-7)
        m44_2, offset_2 = mt.Sat(0.5, [0.2126, 0.7152, 0.0722])
        self.assertAlmostEqual(0.3576, m44_2[1], delta=1e-7)
        m44_2, offset_2 = mt.Scale([0.9, 0.8, 0.7, 1.])
        self.assertAlmostEqual(0.9, m44_2[0], delta=1e-7)
        m44_2, offset_2 = mt.View([1, 1, 1, 0], [0.2126, 0.7152, 0.0722])
        self.assertAlmostEqual(0.0722, m44_2[2], delta=1e-7)

        mt4 = OCIO.MatrixTransform([0.1, 0.2, 0.3, 0.4,
                                    0.5, 0.6, 0.7, 0.8,
                                    0.9, 1.0, 1.1, 1.2,
                                    1.3, 1.4, 1.5, 1.6],
                                   [0.1, 0.2, 0.3, 0.4],
                                   OCIO.TRANSFORM_DIR_INVERSE)
        m44_4 = mt4.getMatrix()
        offset_4 = mt4.getOffset()
        for i in range(0, 16):
            self.assertAlmostEqual(float(i+1)/10.0, m44_4[i], delta=1e-7)
        for i in range(0, 4):
            self.assertAlmostEqual(float(i+1)/10.0, offset_4[i], delta=1e-7)
        self.assertEqual(mt4.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        mt5 = OCIO.MatrixTransform(matrix=[0.1, 0.2, 0.3, 0.4,
                                           0.5, 0.6, 0.7, 0.8,
                                           0.9, 1.0, 1.1, 1.2,
                                           1.3, 1.4, 1.5, 1.6],
                                   offset=[0.1, 0.2, 0.3, 0.4],
                                   direction=OCIO.TRANSFORM_DIR_INVERSE)
        m44_5 = mt5.getMatrix()
        offset_5 = mt5.getOffset()
        for i in range(0, 16):
            self.assertAlmostEqual(float(i+1)/10.0, m44_5[i], delta=1e-7)
        for i in range(0, 4):
            self.assertAlmostEqual(float(i+1)/10.0, offset_5[i], delta=1e-7)
        self.assertEqual(mt5.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
