package uk.ac.dur.cosma.hdfstream;

import java.util.concurrent.atomic.AtomicLong;

public class RequestCounter {

    public static final int FILE = 0;
    public static final int DIRECTORY = 1;
    public static final int MSGPACK = 2;

    private AtomicLong[] nrRequests;
    private AtomicLong[] nrBytes;

    public RequestCounter() {
        nrRequests = new AtomicLong[3];
        nrBytes = new AtomicLong[3];
        for(int i=0; i<3; i+=1) {
            nrRequests[i] = new AtomicLong();
            nrBytes[i] = new AtomicLong();
        }
    }

    public void logRequest(int kind, long size) {
        nrRequests[kind].incrementAndGet();
        nrBytes[kind].addAndGet(size);
    }

    public long getCount(int kind) {
        return nrRequests[kind].get();
    }

    public long getBytes(int kind) {
        return nrBytes[kind].get();
    }
}
