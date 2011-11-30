
import junit.framework.TestCase;
import org.OpenColorIO.*;

public class ContextTest extends TestCase {
    
    protected void setUp() {
    }
    
    protected void tearDown() {
    }
    
    public void test_interface() {
        Context cont = new Context().Create();
        cont.setSearchPath("testing123");
        cont.setWorkingDir("/dir/123");
        assertEquals("$af84c0ff921e48843d711a761e05b80f", cont.getCacheID());
        assertEquals("testing123", cont.getSearchPath());
        assertEquals("/dir/123", cont.getWorkingDir());
        cont.setStringVar("TeSt", "foobar");
        assertEquals("foobar", cont.getStringVar("TeSt"));
        assertEquals(1, cont.getNumStringVars());
        assertEquals("TeSt", cont.getStringVarNameByIndex(0));
        cont.loadEnvironment();
        assertNotSame(0, cont.getNumStringVars());
        cont.setStringVar("TEST1", "foobar");
        assertEquals("/foo/foobar/bar",
                     cont.resolveStringVar("/foo/${TEST1}/bar"));
        try {
            cont.setSearchPath("testing123");
            String foo = cont.resolveFileLocation("test.lut");
            System.out.println(foo);
        } catch (ExceptionMissingFile e) {
            //System.out.println(e);
        }
    }
    
}
