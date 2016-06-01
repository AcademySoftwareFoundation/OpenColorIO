
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class TransformsTest(unittest.TestCase):
    
    def test_interface(self):
        
        ### AllocationTransform ###
        at = OCIO.AllocationTransform()
        self.assertEqual(OCIO.Constants.ALLOCATION_UNIFORM, at.getAllocation())
        at.setAllocation(OCIO.Constants.ALLOCATION_LG2)
        self.assertEqual(OCIO.Constants.ALLOCATION_LG2, at.getAllocation())
        self.assertEqual(0, at.getNumVars())
        at.setVars([0.1, 0.2, 0.3])
        self.assertEqual(3, at.getNumVars())
        newvars = at.getVars()
        self.assertAlmostEqual(0.2, newvars[1], delta=1e-8)
        
        ### Base Transform method tests ###
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_FORWARD, at.getDirection())
        at.setDirection(OCIO.Constants.TRANSFORM_DIR_UNKNOWN)
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_UNKNOWN, at.getDirection())
        
        ### CDLTransform ###
        cdl = OCIO.CDLTransform()
        CC = "<ColorCorrection id=\"foo\">"
        CC += "<SOPNode>"
        CC += "<Description>this is a descipt</Description>"
        CC += "<Slope>1.1 1.2 1.3</Slope><Offset>2.1 2.2 2.3</Offset>"
        CC += "<Power>3.1 3.2 3.3</Power>"
        CC += "</SOPNode>"
        CC += "<SatNode>"
        CC += "<Saturation>0.7</Saturation>"
        CC += "</SatNode>"
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
        CC2 = "<ColorCorrection id=\"foo\">"
        CC2 += "<SOPNode>"
        CC2 += "<Description>this is a descipt</Description>"
        CC2 += "<Slope>0.1 0.2 0.3</Slope>"
        CC2 += "<Offset>1.1 1.2 1.3</Offset>"
        CC2 += "<Power>2.1 2.2 2.3</Power>"
        CC2 += "</SOPNode>"
        CC2 += "<SatNode>"
        CC2 += "<Saturation>0.5</Saturation>"
        CC2 += "</SatNode>""</ColorCorrection>"
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
        
        ### ClampTransform ###
        ct = OCIO.ExponentTransform()
        ct.setMin([0.1, 0.2, 0.3, 0.4])
        evals = et.getMin()
        self.assertAlmostEqual(0.3, evals[2], delta=1e-7)
        ct.setMax([0.1, 0.2, 0.3, 0.4])
        evals = et.getMax()
        self.assertAlmostEqual(0.3, evals[2], delta=1e-7)
        
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
        ft.setInterpolation(OCIO.Constants.INTERP_NEAREST)
        self.assertEqual(OCIO.Constants.INTERP_NEAREST, ft.getInterpolation())
        self.assertEqual(16, ft.getNumFormats())
        self.assertEqual("flame", ft.getFormatNameByIndex(0))
        self.assertEqual("3dl", ft.getFormatExtensionByIndex(0))
        
        ### GroupTransform ###
        gt = OCIO.GroupTransform()
        gt.push_back(et)
        gt.push_back(ft)
        self.assertEqual(2, gt.size())
        self.assertEqual(False, gt.empty())
        foo = gt.getTransform(0)
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_FORWARD, foo.getDirection())
        gt.clear()
        self.assertEqual(0, gt.size())
        
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
        mt.setValue([0.1, 0.2, 0.3, 0.4,
                     0.5, 0.6, 0.7, 0.8,
                     0.9, 1.0, 1.1, 1.2,
                     1.3, 1.4, 1.5, 1.6],
                    [0.1, 0.2, 0.3, 0.4])
        self.assertEqual(False, mt.equals(mmt))
        m44_1, offset_1 = mt.getValue()
        self.assertAlmostEqual(0.3, m44_1[2], delta=1e-7)
        self.assertAlmostEqual(0.2, offset_1[1], delta=1e-7)
        mt.setMatrix([1.1, 1.2, 1.3, 1.4,
                      1.5, 1.6, 1.7, 1.8,
                      1.9, 2.0, 2.1, 2.2,
                      2.3, 2.4, 2.5, 2.6])
        m44_2 = mt.getMatrix()
        self.assertAlmostEqual(1.3, m44_2[2], delta=1e-7)
        mt.setOffset([1.1, 1.2, 1.3, 1.4])
        offset_2 = mt.getOffset()
        self.assertAlmostEqual(1.4, offset_2[3])
        mt.Fit([0.1, 0.1, 0.1, 0.1],
               [0.9, 0.9, 0.9, 0.9],
               [0.0, 0.0, 0.0, 0.0],
               [1.1, 1.1, 1.1, 1.1])
        m44_3 = mt.getMatrix()
        self.assertAlmostEqual(1.3, m44_3[2], delta=1e-7)
        m44_3, offset_2 = mt.Identity()
        self.assertAlmostEqual(0.0, m44_3[1], delta=1e-7)
        m44_2, offset_2 = mt.Sat(0.5, [0.2126, 0.7152, 0.0722])
        self.assertAlmostEqual(0.3576, m44_2[1], delta=1e-7)
        m44_2, offset_2 = mt.Scale([0.9, 0.8, 0.7, 1.])
        self.assertAlmostEqual(0.9, m44_2[0], delta=1e-7)
        m44_2, offset_2 = mt.View([1, 1, 1, 0], [0.2126, 0.7152, 0.0722])
        self.assertAlmostEqual(0.0722, m44_2[2], delta=1e-7)
        
        ### TruelightTransform ###
        """
        tt = OCIO.TruelightTransform()
        tt.setConfigRoot("/some/path")
        self.assertEqual("/some/path", tt.getConfigRoot())
        tt.setProfile("profileA")
        self.assertEqual("profileA", tt.getProfile())
        tt.setCamera("incam")
        self.assertEqual("incam", tt.getCamera())
        tt.setInputDisplay("dellmon")
        self.assertEqual("dellmon", tt.getInputDisplay())
        tt.setRecorder("blah")
        self.assertEqual("blah", tt.getRecorder())
        tt.setPrint("kodasomething")
        self.assertEqual("kodasomething", tt.getPrint())
        tt.setLamp("foobar")
        self.assertEqual("foobar", tt.getLamp())
        tt.setOutputCamera("somecam")
        self.assertEqual("somecam", tt.getOutputCamera())
        tt.setDisplay("sRGB")
        self.assertEqual("sRGB", tt.getDisplay())
        tt.setCubeInput("log")
        self.assertEqual("log", tt.getCubeInput())
        """

