
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class TransformsTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        
        //// AllocationTransform ////
        AllocationTransform at = new AllocationTransform().Create();
        assertEquals(Allocation.ALLOCATION_UNIFORM, at.getAllocation());
        at.setAllocation(Allocation.ALLOCATION_LG2);
        assertEquals(Allocation.ALLOCATION_LG2, at.getAllocation());
        assertEquals(0, at.getNumVars());
        float[] vars = new float[]{0.1f, 0.2f, 0.3f};
        at.setVars(3, vars);
        assertEquals(3, at.getNumVars());
        float newvars[] = new float[3];
        at.getVars(newvars);
        assertEquals(0.2f, newvars[1], 1e-8);
        
        // Base Transform method tests
        assertEquals(TransformDirection.TRANSFORM_DIR_FORWARD, at.getDirection());
        at.setDirection(TransformDirection.TRANSFORM_DIR_UNKNOWN);
        assertEquals(TransformDirection.TRANSFORM_DIR_UNKNOWN, at.getDirection());
        
        //// CDLTransform ////
        CDLTransform cdl = new CDLTransform().Create();
        String CC = ""
        +"<ColorCorrection id=\"foo\">"
        +"<SOPNode>"
        +"<Description>this is a descipt</Description>"
        +"<Slope>1.1 1.2 1.3</Slope>"
        +"<Offset>2.1 2.2 2.3</Offset>"
        +"<Power>3.1 3.2 3.3</Power>"
        +"</SOPNode>"
        +"<SatNode><Saturation>0.7</Saturation></SatNode>"
        +"</ColorCorrection>";
        // Don't want to deal with getting the correct path so this runs
        //CDLTransform cdlfile = new CDLTransform().CreateFromFile("../OpenColorIO/src/jniglue/tests/org/OpenColorIO/test.cc", "foo");
        //assertEquals(CC, cdlfile.getXML());
        cdl.setXML(CC);
        assertEquals(CC, cdl.getXML());
        //CDLTransform match = new CDLTransform().Create();
        CDLTransform match = (CDLTransform)cdl.createEditableCopy();
        match.setOffset(new float[]{1.0f, 1.0f, 1.f});
        assertEquals(false, cdl.equals(match));
        cdl.setSlope(new float[]{0.1f, 0.2f, 0.3f});
        cdl.setOffset(new float[]{1.1f, 1.2f, 1.3f});
        cdl.setPower(new float[]{2.1f, 2.2f, 2.3f});
        cdl.setSat(0.5f);
        String CC2 = ""
        +"<ColorCorrection id=\"foo\">"
        +"<SOPNode>"
        +"<Description>this is a descipt</Description>"
        +"<Slope>0.1 0.2 0.3</Slope>"
        +"<Offset>1.1 1.2 1.3</Offset>"
        +"<Power>2.1 2.2 2.3</Power>"
        +"</SOPNode>"
        +"<SatNode><Saturation>0.5</Saturation></SatNode>"
        +"</ColorCorrection>";
        assertEquals(CC2, cdl.getXML());
        cdl.setSOP(new float[]{1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f});
        float newsop[] = new float[9];
        cdl.getSOP(newsop);
        assertEquals(1.5f, newsop[4], 1e-8);
        float slope[] = new float[3];
        cdl.getSlope(slope);
        assertEquals(1.2f, slope[1], 1e-8);
        float offset[] = new float[3];
        cdl.getOffset(offset);
        assertEquals(1.6f, offset[2], 1e-8);
        float power[] = new float[3];
        cdl.getPower(power);
        assertEquals(1.7f, power[0], 1e-8);
        assertEquals(0.5f, cdl.getSat(), 1e-8);
        float luma[] = new float[3];
        cdl.getSatLumaCoefs(luma);
        assertEquals(0.2126f, luma[0], 1e-8);
        assertEquals(0.7152f, luma[1], 1e-8);
        assertEquals(0.0722f, luma[2], 1e-8);
        cdl.setID("foobar123");
        assertEquals("foobar123", cdl.getID());
        cdl.setDescription("bar");
        assertEquals("bar", cdl.getDescription());
        
        //// ClampTransform ////
        ClampTransform clampt = new ClampTransform().Create();
        float evals[] = new float[4];
        clampt.setMin(new float[]{0.1f, 0.2f, 0.3f, 0.4f});
        clampt.getMin(evals);
        assertEquals(0.3f, evals[2], 1e-8);
        clampt.setMax(new float[]{0.1f, 0.2f, 0.3f, 0.4f});
        clampt.getMax(evals);
        assertEquals(0.3f, evals[2], 1e-8);
        
        //// ColorSpaceTransform ////
        ColorSpaceTransform ct = new ColorSpaceTransform().Create();
        ct.setSrc("foo");
        assertEquals("foo", ct.getSrc());
        ct.setDst("bar");
        assertEquals("bar", ct.getDst());
        
        //// DisplayTransform ////
        DisplayTransform dt = new DisplayTransform().Create();
        dt.setInputColorSpaceName("lin18");
        assertEquals("lin18", dt.getInputColorSpaceName());
        //public native void setLinearCC(Transform cc);
        //public native Transform getLinearCC();
        //public native void setColorTimingCC(Transform cc);
        //public native Transform getColorTimingCC();
        //public native void setChannelView(Transform transform);
        //public native Transform getChannelView();
        dt.setDisplay("sRGB");
        assertEquals("sRGB", dt.getDisplay());
        dt.setView("foobar");
        assertEquals("foobar", dt.getView());
        cdl.setXML(CC);
        dt.setDisplayCC(cdl);
        CDLTransform cdldt = (CDLTransform)dt.getDisplayCC();
        assertEquals(CC, cdldt.getXML());
        dt.setLooksOverride("darkgrade");
        assertEquals("darkgrade", dt.getLooksOverride());
        dt.setLooksOverrideEnabled(true);
        assertEquals(true, dt.getLooksOverrideEnabled());
        
        //// ExponentTransform ////
        ExponentTransform et = new ExponentTransform().Create();
        et.setValue(new float[]{0.1f, 0.2f, 0.3f, 0.4f});
        float evals[] = new float[4];
        et.getValue(evals);
        assertEquals(0.3f, evals[2], 1e-8);
        
        //// FileTransform ////
        FileTransform ft = new FileTransform().Create();
        ft.setSrc("foo");
        assertEquals("foo", ft.getSrc());
        ft.setCCCId("foobar");
        assertEquals("foobar", ft.getCCCId());
        ft.setInterpolation(Interpolation.INTERP_NEAREST);
        assertEquals(Interpolation.INTERP_NEAREST, ft.getInterpolation());
        assertEquals(16, ft.getNumFormats());
        assertEquals("flame", ft.getFormatNameByIndex(0));
        assertEquals("3dl", ft.getFormatExtensionByIndex(0));
        
        //// GroupTransform ////
        GroupTransform gt = new GroupTransform().Create();
        gt.push_back(et);
        gt.push_back(ft);
        assertEquals(2, gt.size());
        assertEquals(false, gt.empty());
        Transform foo = gt.getTransform(0);
        assertEquals(TransformDirection.TRANSFORM_DIR_FORWARD, foo.getDirection());
        gt.clear();
        assertEquals(0, gt.size());
        
        //// LogTransform ////
        LogTransform lt = new LogTransform().Create();
        lt.setBase(10.f);
        assertEquals(10.f, lt.getBase());
        
        //// LookTransform ////
        LookTransform lkt = new LookTransform().Create();
        lkt.setSrc("foo");
        assertEquals("foo", lkt.getSrc());
        lkt.setDst("bar");
        assertEquals("bar", lkt.getDst());
        lkt.setLooks("bar;foo");
        assertEquals("bar;foo", lkt.getLooks());
        
        //// MatrixTransform ////
        MatrixTransform mt = new MatrixTransform().Create();
        MatrixTransform mmt = (MatrixTransform)mt.createEditableCopy();
        mt.setValue(new float[]{0.1f, 0.2f, 0.3f, 0.4f,
                                0.5f, 0.6f, 0.7f, 0.8f,
                                0.9f, 1.0f, 1.1f, 1.2f,
                                1.3f, 1.4f, 1.5f, 1.6f},
                    new float[]{0.1f, 0.2f, 0.3f, 0.4f});
        assertEquals(false, mt.equals(mmt));
        float m44_1[] = new float[16];
        float offset_1[] = new float[4];
        mt.getValue(m44_1, offset_1);
        assertEquals(0.3f, m44_1[2]);
        assertEquals(0.2f, offset_1[1]);
        mt.setMatrix(new float[]{1.1f, 1.2f, 1.3f, 1.4f,
                                 1.5f, 1.6f, 1.7f, 1.8f,
                                 1.9f, 2.0f, 2.1f, 2.2f,
                                 2.3f, 2.4f, 2.5f, 2.6f});
        float m44_2[] = new float[16];
        mt.getMatrix(m44_2);
        assertEquals(1.3f, m44_2[2]);
        mt.setOffset(new float[]{1.1f, 1.2f, 1.3f, 1.4f});
        float offset_2[] = new float[4];
        mt.getOffset(offset_2);
        assertEquals(1.4f, offset_2[3]);
        mt.Fit(m44_2, offset_2,
               new float[]{0.1f, 0.1f, 0.1f, 0.1f},
               new float[]{0.9f, 0.9f, 0.9f, 0.9f},
               new float[]{0.0f, 0.0f, 0.0f, 0.0f},
               new float[]{1.1f, 1.1f, 1.1f, 1.1f});
        float m44_3[] = new float[16];
        mt.getMatrix(m44_3);
        assertEquals(1.3f, m44_3[2]);
        mt.Identity(m44_3, offset_2);
        assertEquals(0.f, m44_3[1]);
        mt.Sat(m44_2, offset_2, 0.5f, new float[]{0.2126f, 0.7152f, 0.0722f});
        assertEquals(0.3576f, m44_2[1]);
        mt.Scale(m44_2, offset_2, new float[]{0.9f, 0.8f, 0.7f, 1.f});
        assertEquals(0.9f, m44_2[0]);
        mt.View(m44_2, offset_2, new int[]{1, 1, 1, 0}, new float[]{0.2126f, 0.7152f, 0.0722f});
        assertEquals(0.0722f, m44_2[2]);
        
        //// TruelightTransform ////
        TruelightTransform tt = new TruelightTransform().Create();
        tt.setConfigRoot("/some/path");
        assertEquals("/some/path", tt.getConfigRoot());
        tt.setProfile("profileA");
        assertEquals("profileA", tt.getProfile());
        tt.setCamera("incam");
        assertEquals("incam", tt.getCamera());
        tt.setInputDisplay("dellmon");
        assertEquals("dellmon", tt.getInputDisplay());
        tt.setRecorder("blah");
        assertEquals("blah", tt.getRecorder());
        tt.setPrint("kodasomething");
        assertEquals("kodasomething", tt.getPrint());
        tt.setLamp("foobar");
        assertEquals("foobar", tt.getLamp());
        tt.setOutputCamera("somecam");
        assertEquals("somecam", tt.getOutputCamera());
        tt.setDisplay("sRGB");
        assertEquals("sRGB", tt.getDisplay());
        tt.setCubeInput("log");
        assertEquals("log", tt.getCubeInput());
        
        try {
        } catch (Exception e) { System.out.println(e); }
        
    }
    
}
