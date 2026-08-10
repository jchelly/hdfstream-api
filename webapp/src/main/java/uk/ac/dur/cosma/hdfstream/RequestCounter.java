package uk.ac.dur.cosma.hdfstream;

import java.time.Duration;
import java.time.LocalDate;
import java.util.concurrent.atomic.LongAdder;
import java.util.concurrent.ConcurrentHashMap;
import java.util.ArrayList;

public class RequestCounter {

    public static final int FILE = 0;
    public static final int DIRECTORY = 1;
    public static final int MSGPACK = 2;

    private final ArrayList<ConcurrentHashMap<Long,LongAdder>> nrRequests = new ArrayList<ConcurrentHashMap<Long,LongAdder>>();
    private final ArrayList<ConcurrentHashMap<Long,LongAdder>> nrBytes = new ArrayList<ConcurrentHashMap<Long,LongAdder>>();

    public RequestCounter() {
        for(int i=0; i<3; i+=1) {
            nrRequests.add(new ConcurrentHashMap<Long,LongAdder>());
            nrBytes.add(new ConcurrentHashMap<Long,LongAdder>());
        }
    }

    private long currentBucket() {
        return LocalDate.now().toEpochDay();
    }

    public void logRequest(int kind, long size) {
        long now = currentBucket();
        nrRequests.get(kind).computeIfAbsent(now, k -> new LongAdder()).increment();
        nrBytes.get(kind).computeIfAbsent(now, k -> new LongAdder()).add(size);
    }

    public long getCount(int kind, int nrDays) {
        long now = currentBucket();
        long cutoff = now - nrDays + 1;
        return nrRequests.get(kind).entrySet().stream().filter(e -> e.getKey() >= cutoff).mapToLong(e -> e.getValue().sum()).sum();
    }

    public long getCount(int kind) {
        return getCount(kind, 36500); // Last 100 years!
    }

    public long getBytes(int kind, int nrDays) {
        long now = currentBucket();
        long cutoff = now - nrDays + 1;
        return nrBytes.get(kind).entrySet().stream().filter(e -> e.getKey() >= cutoff).mapToLong(e -> e.getValue().sum()).sum();
    }

    public long getBytes(int kind) {
        return getBytes(kind, 36500);
    }
}
