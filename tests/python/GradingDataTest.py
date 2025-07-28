# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import assertEqualRGBM, assertEqualPrimary, assertEqualRGBMSW, \
    assertEqualTone, assertEqualBSpline, assertEqualRGBCurve, assertEqualHueCurve, \
    assertAlmostEqualVector

class GradingDataTest(unittest.TestCase):

    def test_rgbm(self):
        """
        Test the GradingRGBM struct.
        """

        # Constructor without parameters.
        rgbm1 = OCIO.GradingRGBM()
        self.assertEqual(0, rgbm1.red)
        self.assertEqual(0, rgbm1.green)
        self.assertEqual(0, rgbm1.blue)
        self.assertEqual(0, rgbm1.master)

        # Constructor with parameters.
        rgbm2 = OCIO.GradingRGBM(1, 2, 3, 4)
        self.assertEqual(1, rgbm2.red)
        self.assertEqual(2, rgbm2.green)
        self.assertEqual(3, rgbm2.blue)
        self.assertEqual(4, rgbm2.master)

        rgbm2.red += 1
        rgbm2.green += 1
        rgbm2.blue += 1
        rgbm2.master += 1
        self.assertEqual(2, rgbm2.red)
        self.assertEqual(3, rgbm2.green)
        self.assertEqual(4, rgbm2.blue)
        self.assertEqual(5, rgbm2.master)

        self.assertNotEqual(rgbm1, rgbm2)
        rgbm1.red = rgbm2.red
        rgbm1.green = rgbm2.green
        rgbm1.blue = rgbm2.blue
        rgbm1.master = rgbm2.master
        assertEqualRGBM(self, rgbm1, rgbm2)

        rgbm1.red += 1
        with self.assertRaises(AssertionError):
            assertEqualRGBM(self, rgbm1, rgbm2)

        rgbm3 = OCIO.GradingRGBM()
        rgbm2 = rgbm3
        self.assertEqual(0, rgbm2.red)
        rgbm3.red = 2
        self.assertEqual(2, rgbm2.red)
        rgbm2.red = 0
        self.assertEqual(0, rgbm3.red)

        with self.assertRaises(TypeError):
            OCIO.GradingRGBM(0)

        with self.assertRaises(TypeError):
            OCIO.GradingRGBM(0, 0)
            
        # Constructor with named parameters.
        rgbm3 = OCIO.GradingRGBM(blue=1, green=2, master=3, red=4)
        self.assertEqual(4, rgbm3.red)
        self.assertEqual(2, rgbm3.green)
        self.assertEqual(1, rgbm3.blue)
        self.assertEqual(3, rgbm3.master)

        # Constructor with named parameters, some missing.
        with self.assertRaises(TypeError):
            OCIO.GradingRGBM(master=.3, red=.4)

        # Check comparison operators
        rgbm1 = OCIO.GradingRGBM()
        rgbm2 = OCIO.GradingRGBM()
        self.assertEqual(rgbm1, rgbm2)
        rgbm1.red = 2
        self.assertNotEqual(rgbm1, rgbm2)

    def test_primary(self):
        """
        Test the GradingPrimary struct.
        """

        rgbm0 = OCIO.GradingRGBM(0, 0, 0, 0)
        rgbm1 = OCIO.GradingRGBM(1, 1, 1, 1)
        # Constructor.
        primaryLog = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        assertEqualRGBM(self, rgbm0, primaryLog.brightness)
        assertEqualRGBM(self, rgbm1, primaryLog.contrast)
        assertEqualRGBM(self, rgbm1, primaryLog.gamma)
        assertEqualRGBM(self, rgbm0, primaryLog.offset)
        assertEqualRGBM(self, rgbm0, primaryLog.exposure)
        assertEqualRGBM(self, rgbm0, primaryLog.lift)
        assertEqualRGBM(self, rgbm1, primaryLog.gain)
        self.assertEqual(-0.2, primaryLog.pivot)
        self.assertEqual(1, primaryLog.saturation)
        # Check that the default values do not clamp.
        self.assertEqual(primaryLog.NoClampWhite, primaryLog.clampWhite)
        self.assertEqual(primaryLog.NoClampBlack, primaryLog.clampBlack)
        self.assertEqual(1, primaryLog.pivotWhite)
        self.assertEqual(0, primaryLog.pivotBlack)

        primaryLin = OCIO.GradingPrimary(OCIO.GRADING_LIN)
        with self.assertRaises(AssertionError):
            assertEqualPrimary(self, primaryLog, primaryLin)

        primaryLog.pivot = 0.18
        assertEqualPrimary(self, primaryLog, primaryLin)

        primaryVideo = OCIO.GradingPrimary(OCIO.GRADING_VIDEO)
        assertEqualPrimary(self, primaryLog, primaryVideo)

        with self.assertRaises(TypeError):
            OCIO.GradingPrimary()

        with self.assertRaises(AttributeError):
            OCIO.GradingPrimary(OCIO.TRANSFOR_DIRECTION_FORWARD)

        with self.assertRaises(TypeError):
            OCIO.GradingPrimary(0)

        newGamma = OCIO.GradingRGBM(1.1, 1.2, 1.3, 1)
        primaryLog.gamma = newGamma
        assertEqualRGBM(self, newGamma, primaryLog.gamma)

        # Check comparison operators
        primaryLog1 = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        primaryLog2 = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        self.assertEqual(primaryLog1, primaryLog2)
        primaryLog1.saturation = 0.5
        self.assertNotEqual(primaryLog1, primaryLog2)

    def test_bspline(self):
        """
        Test the GradingBSplineCurve: creation, control point modification, validation.
        """

        bs = OCIO.GradingBSplineCurve(5)
        cpts = bs.getControlPoints()
        # First control point (i.e. cpts[0]) defaults to {0, 0}.
        cpts[1] = OCIO.GradingControlPoint(0.1, 0.5)
        cpts[2] = OCIO.GradingControlPoint(0.4, 0.6)
        cpts[3] = OCIO.GradingControlPoint(0.6, 0.7)
        cpts[4] = OCIO.GradingControlPoint(1, 1)
        self.assertIsNone(bs.validate())
        self.assertEqual(bs.getSplineType(), OCIO.B_SPLINE)
        self.assertEqual(bs.slopesAreDefault(), True)

        # Move point 4 before point 3 on the x axis so that the control points are not anymore
        # monotonic. Then, it must throw an exception.
        cpts[4] = OCIO.GradingControlPoint(0.5, 1)
        with self.assertRaises(OCIO.Exception):
            bs.validate()

        # Restore valid data.
        cpts[4] = OCIO.GradingControlPoint(1, 1)

        # Test constructing via splineType or curveType.
        spl1 = OCIO.GradingBSplineCurve(5, OCIO.DIAGONAL_B_SPLINE)
        self.assertEqual(spl1.getSplineType(), OCIO.DIAGONAL_B_SPLINE)
        spl2 = OCIO.GradingBSplineCurve(5, OCIO.HUE_HUE)
        self.assertEqual(spl2.getSplineType(), OCIO.HUE_HUE_B_SPLINE)
        spl2.setSplineType(OCIO.PERIODIC_1_B_SPLINE)
        self.assertEqual(spl2.getSplineType(), OCIO.PERIODIC_1_B_SPLINE)

        # Create a similar bspline curve with alternate constructor.
        bs2 = OCIO.GradingBSplineCurve([0, 0, 0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1])
        cpts2 = bs2.getControlPoints()
        assertEqualBSpline(self, bs, bs2)
        self.assertEqual(bs2.getSplineType(), OCIO.B_SPLINE)
        self.assertEqual(bs, bs2)

        bs2 = OCIO.GradingBSplineCurve([0, 0, 0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1],
            OCIO.LUM_SAT)
        cpts2 = bs2.getControlPoints()
        # NB: This comparison function is only on the control points.
        assertEqualBSpline(self, bs, bs2)
        self.assertEqual(bs2.getSplineType(), OCIO.HORIZONTAL1_B_SPLINE)
        self.assertEqual(bs2.slopesAreDefault(), True)
        # The class op== detects that the curve type is different.
        self.assertNotEqual(bs, bs2)

        # Curve with fewer control points.
        bs3 = OCIO.GradingBSplineCurve(4)
        cpts3 = bs3.getControlPoints()
        cpts3[1] = OCIO.GradingControlPoint(0.1, 0.5)
        cpts3[2] = OCIO.GradingControlPoint(0.4, 0.6)
        cpts3[3] = OCIO.GradingControlPoint(0.6, 0.7)

        with self.assertRaises(AssertionError):
            assertEqualBSpline(self, bs, bs3)

        # Curve with more control points.
        bs4 = OCIO.GradingBSplineCurve(6)
        cpts4 = bs4.getControlPoints()
        cpts4[1] = OCIO.GradingControlPoint(0.1, 0.5)
        cpts4[2] = OCIO.GradingControlPoint(0.4, 0.6)
        cpts4[3] = OCIO.GradingControlPoint(0.6, 0.7)
        cpts4[4] = OCIO.GradingControlPoint(1, 1)
        cpts4[5] = OCIO.GradingControlPoint(1.2, 1.1)

        with self.assertRaises(AssertionError):
            assertEqualBSpline(self, bs, bs4)

        # Curve with the same number of control points but point at index 2 differs.
        bs5 = OCIO.GradingBSplineCurve(5)
        cpts5 = bs5.getControlPoints()
        cpts5[1] = OCIO.GradingControlPoint(0.1, 0.5)
        # Different x value.
        cpts5[2] = OCIO.GradingControlPoint(x=0.5, y=0.6)
        cpts5[3] = OCIO.GradingControlPoint(x=0.6, y=0.7)
        cpts5[4] = OCIO.GradingControlPoint(1, 1)

        with self.assertRaises(AssertionError):
            assertEqualBSpline(self, bs, bs5)

        # Fix point at index 2.
        cpts5[2] = OCIO.GradingControlPoint(y=0.6, x=0.4)
        assertEqualBSpline(self, bs, bs5)

        # Check comparison operators
        cpts1 = OCIO.GradingControlPoint(1, 1)
        cpts2 = OCIO.GradingControlPoint(1, 1)
        self.assertEqual(cpts1, cpts2)
        cpts1.x = 0.5
        self.assertNotEqual(cpts1, cpts2)

        bs1 = OCIO.GradingBSplineCurve([0, 0, 0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1])
        bs2 = OCIO.GradingBSplineCurve([0, 0, 0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1])
        self.assertEqual(bs1, bs2)
        bs1.getControlPoints()[2] = OCIO.GradingControlPoint(0.1, 0.4)
        self.assertNotEqual(cpts1, cpts2)

        # Slopes.
        s = [0.9, 0.8, 0.7, 0.6, 0.5]
        bs1.setSlopes(s)
        s1 = bs1.getSlopes()
        assertAlmostEqualVector(self, s, s1, delta=1e-6)
        with self.assertRaises(OCIO.Exception):
            # Length of slopes must match control points.
            bs1.setSlopes(s[2:])
        # The comparison now fails since the slopes are not equal.
        self.assertNotEqual(bs1, bs2)

    def test_rgbcurve(self):
        """
        Test the GradingRGBCurve, creation, default value, modification.
        """

        # Check default values.

        rgbLin = OCIO.GradingRGBCurve(OCIO.GRADING_LIN)

        defLin = OCIO.GradingBSplineCurve(3)
        cpts = defLin.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(-7, -7)
        cpts[1] = OCIO.GradingControlPoint(0, 0)
        cpts[2] = OCIO.GradingControlPoint(7, 7)
        assertEqualBSpline(self, rgbLin.red, defLin)
        assertEqualBSpline(self, rgbLin.green, defLin)
        assertEqualBSpline(self, rgbLin.blue, defLin)
        assertEqualBSpline(self, rgbLin.master, defLin)

        rgbLog = OCIO.GradingRGBCurve(OCIO.GRADING_LOG)

        defLog = OCIO.GradingBSplineCurve(3)
        cpts = defLog.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(0, 0)
        cpts[1] = OCIO.GradingControlPoint(0.5, 0.5)
        cpts[2] = OCIO.GradingControlPoint(1, 1)
        assertEqualBSpline(self, rgbLog.red, defLog)
        assertEqualBSpline(self, rgbLog.green, defLog)
        assertEqualBSpline(self, rgbLog.blue, defLog)
        assertEqualBSpline(self, rgbLog.master, defLog)
        with self.assertRaises(AssertionError):
            assertEqualBSpline(self, rgbLog.master, defLin)

        rgbVideo = OCIO.GradingRGBCurve(OCIO.GRADING_VIDEO)
        assertEqualRGBCurve(self, rgbLog, rgbVideo)
        
        # Check comparison operators
        rgbc1 = OCIO.GradingRGBCurve(OCIO.GRADING_LIN)
        rgbc2 = OCIO.GradingRGBCurve(OCIO.GRADING_LIN)
        self.assertEqual(rgbc1, rgbc2)
        self.assertEqual(rgbc1.isIdentity(), True)
        rgbc1.red.getControlPoints()[1] = OCIO.GradingControlPoint(0.4, 0.5)
        self.assertNotEqual(rgbc1, rgbc2)
        self.assertEqual(rgbc1.isIdentity(), False)
        rgbc1.validate()

        # Check full constructor.
        bs1 = OCIO.GradingBSplineCurve([0, 0, 0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1])
        bs2 = OCIO.GradingBSplineCurve([0.1, 0.5, 0.4, 0.6, 0.6, 0.7, 1, 1.1])
        rgbc1 = OCIO.GradingRGBCurve(bs1, bs2, bs1, bs2)
        rgbc1.validate()
        self.assertEqual(rgbc1.isIdentity(), False)
        assertEqualBSpline(self, rgbc1.green, bs2)
        self.assertEqual(rgbc1.green, bs2)

    def test_huecurve(self):
        """
        Test the GradingHueCurve, creation, default value, modification.
        """

        # Check default values.

        hueLin = OCIO.GradingHueCurve(OCIO.GRADING_LIN)

        defLin = OCIO.GradingBSplineCurve(3)
        cpts = defLin.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(-7, -7)
        cpts[1] = OCIO.GradingControlPoint(0, 0)
        cpts[2] = OCIO.GradingControlPoint(7, 7)
        assertEqualBSpline(self, hueLin.lum_lum, defLin)

        hueLog = OCIO.GradingHueCurve(OCIO.GRADING_LOG)

        defLog = OCIO.GradingBSplineCurve(3)
        cpts = defLog.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(0, 0)
        cpts[1] = OCIO.GradingControlPoint(0.5, 0.5)
        cpts[2] = OCIO.GradingControlPoint(1, 1)
        assertEqualBSpline(self, hueLog.lum_lum, defLog)
        with self.assertRaises(AssertionError):
            assertEqualBSpline(self, hueLog.lum_lum, defLin)
        self.assertEqual(hueLog.getDrawCurveOnly(), False)

        hueVideo = OCIO.GradingHueCurve(OCIO.GRADING_VIDEO)
        assertEqualHueCurve(self, hueLog, hueVideo)
        
        # Check comparison operators

        huec1 = OCIO.GradingHueCurve(OCIO.GRADING_LIN)
        huec2 = OCIO.GradingHueCurve(OCIO.GRADING_LIN)
        self.assertEqual(huec1, huec2)

        huec1.setDrawCurveOnly(True)
        self.assertNotEqual(huec1, huec2)
        self.assertEqual(huec1.isIdentity(), True)
        huec1.setDrawCurveOnly(False)
        self.assertEqual(huec1, huec2)

        self.assertEqual(huec1.isIdentity(), True)
        huec1.sat_lum.getControlPoints()[1] = OCIO.GradingControlPoint(0.4, 0.8)
        self.assertNotEqual(huec1, huec2)
        self.assertEqual(huec1.isIdentity(), False)
        huec1.validate()

        # Check full constructor.
        hh = OCIO.GradingBSplineCurve(5, OCIO.HUE_HUE)
        hs = OCIO.GradingBSplineCurve(5, OCIO.HUE_SAT)
        hl = OCIO.GradingBSplineCurve(5, OCIO.HUE_LUM)
        ls = OCIO.GradingBSplineCurve([0.1, 0.8, 0.9, 0.7], OCIO.LUM_SAT)
        ss = OCIO.GradingBSplineCurve(5, OCIO.SAT_SAT)
        ll = OCIO.GradingBSplineCurve(5, OCIO.LUM_LUM)
        sl = OCIO.GradingBSplineCurve(5, OCIO.SAT_LUM)
        hfx = OCIO.GradingBSplineCurve(5, OCIO.HUE_FX)
        hcrv = OCIO.GradingHueCurve(hh, hs, hl, ls, ss, ll, sl, hfx)
        hcrv.validate()
        self.assertEqual(hcrv.isIdentity(), False)
        assertEqualBSpline(self, hcrv.lum_sat, ls)
        self.assertEqual(hcrv.lum_sat, ls)

    def test_rgbmsw(self):
        """
        Test the GradingRGBMSW struct.
        """

        # Constructor without parameters.
        rgbm1 = OCIO.GradingRGBMSW()
        self.assertEqual(1, rgbm1.red)
        self.assertEqual(1, rgbm1.green)
        self.assertEqual(1, rgbm1.blue)
        self.assertEqual(1, rgbm1.master)
        self.assertEqual(0, rgbm1.start)
        self.assertEqual(1, rgbm1.width)

        # Constructor with parameters.
        rgbm2 = OCIO.GradingRGBMSW(1, 2, 3, 4, 5, 6)
        self.assertEqual(1, rgbm2.red)
        self.assertEqual(2, rgbm2.green)
        self.assertEqual(3, rgbm2.blue)
        self.assertEqual(4, rgbm2.master)
        self.assertEqual(5, rgbm2.start)
        self.assertEqual(6, rgbm2.width)

        rgbm2.red += 1
        rgbm2.green += 1
        rgbm2.blue += 1
        rgbm2.master += 1
        rgbm2.start += 1
        rgbm2.width += 1
        self.assertEqual(2, rgbm2.red)
        self.assertEqual(3, rgbm2.green)
        self.assertEqual(4, rgbm2.blue)
        self.assertEqual(5, rgbm2.master)
        self.assertEqual(6, rgbm2.start)
        self.assertEqual(7, rgbm2.width)

        self.assertNotEqual(rgbm1, rgbm2)
        rgbm1.red = rgbm2.red
        rgbm1.green = rgbm2.green
        rgbm1.blue = rgbm2.blue
        rgbm1.master = rgbm2.master
        rgbm1.start = rgbm2.start
        rgbm1.width = rgbm2.width
        assertEqualRGBM(self, rgbm1, rgbm2)

        rgbm1.red += 1
        with self.assertRaises(AssertionError):
            assertEqualRGBMSW(self, rgbm1, rgbm2)

        rgbm3 = OCIO.GradingRGBMSW()
        rgbm2 = rgbm3
        self.assertEqual(1, rgbm2.red)
        rgbm3.red = 2
        self.assertEqual(2, rgbm2.red)
        rgbm2.red = 0
        self.assertEqual(0, rgbm3.red)

        rgbm4 = OCIO.GradingRGBMSW(2, 3)
        self.assertEqual(1, rgbm4.red)
        self.assertEqual(1, rgbm4.green)
        self.assertEqual(1, rgbm4.blue)
        self.assertEqual(1, rgbm4.master)
        self.assertEqual(2, rgbm4.start)
        self.assertEqual(3, rgbm4.width)

        with self.assertRaises(TypeError):
            OCIO.GradingRGBMSW(0)

        with self.assertRaises(TypeError):
            OCIO.GradingRGBMSW(0, 0, 0)

        # Constructor with named parameters.
        rgbm5 = OCIO.GradingRGBMSW(blue=1, master=2, green=3, start=4, width=5, red=6)
        self.assertEqual(6, rgbm5.red)
        self.assertEqual(3, rgbm5.green)
        self.assertEqual(1, rgbm5.blue)
        self.assertEqual(2, rgbm5.master)
        self.assertEqual(4, rgbm5.start)
        self.assertEqual(5, rgbm5.width)

        # Constructor with named parameters.
        rgbm5 = OCIO.GradingRGBMSW(width=3, start=4)
        self.assertEqual(1, rgbm5.red)
        self.assertEqual(1, rgbm5.green)
        self.assertEqual(1, rgbm5.blue)
        self.assertEqual(1, rgbm5.master)
        self.assertEqual(4, rgbm5.start)
        self.assertEqual(3, rgbm5.width)

        # Constructor with named parameters, some missing.
        with self.assertRaises(TypeError):
            OCIO.GradingRGBMSW(green=3, start=4)

        # Check comparison operators
        rgbm1 = OCIO.GradingRGBMSW(1, 2, 3, 4, 5, 6)
        rgbm2 = OCIO.GradingRGBMSW(1, 2, 3, 4, 5, 6)
        self.assertEqual(rgbm1, rgbm2)
        rgbm1.red = 2
        self.assertNotEqual(rgbm1, rgbm2)

    def test_tone(self):
        """
        Test the GradingTone struct creation.
        """

        rgbmB = OCIO.GradingRGBMSW(1, 1, 1, 1, 0.4, 0.4)
        rgbmW = OCIO.GradingRGBMSW(1, 1, 1, 1, 0.4, 0.5)
        rgbmS = OCIO.GradingRGBMSW(1, 1, 1, 1, 0.5, 0)
        rgbmH = OCIO.GradingRGBMSW(1, 1, 1, 1, 0.3, 1)
        rgbmM = OCIO.GradingRGBMSW(1, 1, 1, 1, 0.4, 0.6)
        # Constructor.
        tone = OCIO.GradingTone(OCIO.GRADING_LOG)
        assertEqualRGBMSW(self, rgbmB, tone.blacks)
        assertEqualRGBMSW(self, rgbmW, tone.whites)
        assertEqualRGBMSW(self, rgbmS, tone.shadows)
        assertEqualRGBMSW(self, rgbmH, tone.highlights)
        assertEqualRGBMSW(self, rgbmM, tone.midtones)
        self.assertEqual(1, tone.scontrast)

        with self.assertRaises(AttributeError):
            OCIO.GradingTone(OCIO.TRANSFOR_DIRECTION_FORWARD)

        with self.assertRaises(TypeError):
            OCIO.GradingTone(0)

        newMidtones = OCIO.GradingRGBMSW(1.1, 1.2, 1.3, 1, 0.2, 1.1)
        tone.midtones = newMidtones
        assertEqualRGBM(self, newMidtones, tone.midtones)

        # Check comparison operators
        tone1 = OCIO.GradingTone(OCIO.GRADING_LOG)
        tone2 = OCIO.GradingTone(OCIO.GRADING_LOG)
        self.assertEqual(tone1, tone2)
        tone1.midtones = newMidtones
        self.assertNotEqual(tone1, tone2)