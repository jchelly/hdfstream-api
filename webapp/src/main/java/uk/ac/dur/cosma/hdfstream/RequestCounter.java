package uk.ac.dur.cosma.hdfstream;

import com.github.benmanes.caffeine.cache.Cache;
import com.github.benmanes.caffeine.cache.Caffeine;

import java.time.Duration;
import java.time.LocalDate;
import java.util.concurrent.atomic.LongAdder;
import java.util.HashMap;


public class RequestCounter {

    public static final int FILE = 0;
    public static final int DIRECTORY = 1;
    public static final int MSGPACK = 2;

    // Keep counts for the last MAX_DAYS days
    private static int MAX_DAYS = 7;
    private final HashMap<Integer,Cache<Long,LongAdder>> nrRequests = new HashMap<Integer,Cache<Long,LongAdder>>();
    private final HashMap<Integer,Cache<Long,LongAdder>> nrBytes = new HashMap<Integer,Cache<Long,LongAdder>>();

    public RequestCounter() {
        for(int i=0; i<3; i+=1) {
            nrRequests.put(i, Caffeine.newBuilder()
                           .expireAfterWrite(Duration.ofDays(MAX_DAYS))
                           .build());
            nrBytes.put(i, Caffeine.newBuilder()
                        .expireAfterWrite(Duration.ofDays(MAX_DAYS))
                        .build());
        }
    }

    private long currentBucket() {
        return LocalDate.now().toEpochDay();
    }

    public void logRequest(int kind, long size) {
        nrRequests.get(kind).get(currentBucket(), k -> new LongAdder()).increment();
        nrBytes.get(kind).get(currentBucket(), k -> new LongAdder()).add(size);
    }

    public long getCount(int kind) {
        return nrRequests.get(kind).asMap()
            .values()
            .stream()
            .mapToLong(LongAdder::sum)
            .sum();
    }

    public long getBytes(int kind) {
        return nrBytes.get(kind).asMap()
            .values()
            .stream()
            .mapToLong(LongAdder::sum)
            .sum();
    }
}
