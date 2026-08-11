import org.junit.jupiter.api.Test;
import uk.ac.dur.cosma.hdfstream.RequestCounter;
import static org.junit.jupiter.api.Assertions.*;

public class TestRequestCounter {
    @Test
    public void testLog() throws Exception {
        RequestCounter rc = new RequestCounter();
        rc.logRequest(0,0);
    }

    public void runTestForKind(int kind) throws Exception {
        RequestCounter rc = new RequestCounter();
        rc.logRequest(kind, 100);
        rc.logRequest(kind, 200);
        rc.logRequest(kind, 300);
        assertEquals((long) 600, rc.getBytes(kind));
        assertEquals((long) 3, rc.getCount(kind));
    }

    @Test
    public void testFile() throws Exception {
        runTestForKind(RequestCounter.FILE);
    }

    @Test
    public void testDirectory() throws Exception {
        runTestForKind(RequestCounter.DIRECTORY);
    }

    @Test
    public void testMsgPack() throws Exception {
        runTestForKind(RequestCounter.MSGPACK);
    }

    public void runTestMultiKind() throws Exception {
        RequestCounter rc = new RequestCounter();
        rc.logRequest(RequestCounter.FILE, 1000);
        rc.logRequest(RequestCounter.FILE, 2000);
        rc.logRequest(RequestCounter.FILE, 3000);
        rc.logRequest(RequestCounter.DIRECTORY, 100);
        rc.logRequest(RequestCounter.DIRECTORY, 200);
        rc.logRequest(RequestCounter.MSGPACK, 5);
        rc.logRequest(RequestCounter.MSGPACK, 5);
        rc.logRequest(RequestCounter.MSGPACK, 10);
        rc.logRequest(RequestCounter.MSGPACK, 10);
        assertEquals((long) 6000, RequestCounter.FILE);
        assertEquals((long) 3, RequestCounter.FILE);
        assertEquals((long) 300, RequestCounter.DIRECTORY);
        assertEquals((long) 2, RequestCounter.DIRECTORY);
        assertEquals((long) 20, RequestCounter.MSGPACK);
        assertEquals((long) 4, RequestCounter.MSGPACK);
    }
}
