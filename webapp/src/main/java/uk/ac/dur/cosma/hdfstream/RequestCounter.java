package uk.ac.dur.cosma.hdfstream;

import java.time.Duration;
import java.time.LocalDate;
import java.util.concurrent.atomic.LongAdder;
import java.util.concurrent.ConcurrentHashMap;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;


public class RequestCounter {

    public static final int FILE = 0;
    public static final int DIRECTORY = 1;
    public static final int MSGPACK = 2;

    private final ArrayList<ConcurrentHashMap<Long,LongAdder>> nrRequests = new ArrayList<ConcurrentHashMap<Long,LongAdder>>();
    private final ArrayList<ConcurrentHashMap<Long,LongAdder>> nrBytes = new ArrayList<ConcurrentHashMap<Long,LongAdder>>();
    private final ConcurrentHashMap<Long, Set> uniqueUsers = new ConcurrentHashMap<Long, Set>();

    private long MAX_DAYS_RETENTION = 30;
    private long lastPurge = -1;

    public RequestCounter() {
        for(int i=0; i<3; i+=1) {
            nrRequests.add(new ConcurrentHashMap<Long,LongAdder>());
            nrBytes.add(new ConcurrentHashMap<Long,LongAdder>());
        }
    }

    public void purgeUserList() {
        long now = currentDay();
        if(lastPurge==now)
            return;
        uniqueUsers.keySet().removeIf(day -> day < (now - MAX_DAYS_RETENTION));
        lastPurge = now;
    }

    private long currentDay() {
        return LocalDate.now().toEpochDay();
    }

    public void logRequest(int kind, long size) {
        long now = currentDay();
        nrRequests.get(kind).computeIfAbsent(now, k -> new LongAdder()).increment();
        nrBytes.get(kind).computeIfAbsent(now, k -> new LongAdder()).add(size);
    }

    public void logUser(String username) {
        long now = currentDay();
        uniqueUsers.computeIfAbsent(now, k -> Collections.newSetFromMap(new ConcurrentHashMap<String, Boolean>())).add(username);
        purgeUserList();
    }

    /*
      Get the number of requests in the last nrDays days.
      nrDays=1 gives today's requests only.
     */
    public long getCount(int kind, int nrDays) {
        long now = currentDay();
        long cutoff = now - nrDays + 1;
        return nrRequests.get(kind).entrySet().stream().filter(e -> e.getKey() >= cutoff).mapToLong(e -> e.getValue().sum()).sum();
    }

    public long getCount(int kind) {
        return getCount(kind, 36500); // Last 100 years!
    }

    public long getBytes(int kind, int nrDays) {
        long now = currentDay();
        long cutoff = now - nrDays + 1;
        return nrBytes.get(kind).entrySet().stream().filter(e -> e.getKey() >= cutoff).mapToLong(e -> e.getValue().sum()).sum();
    }

    public long getBytes(int kind) {
        return getBytes(kind, 36500);
    }

    /*
      Get the number of unique users in the last nrDays days.
      nrDays=1 gives today's users only.
     */
    public int getUniqueUsers(int nrDays) {

        Set<String> allUsers = new HashSet();
        long now = currentDay();
        for(long i=now-nrDays+1; i<=now; i+=1) {
            Set<String> usersToday = uniqueUsers.get(i);
            if(usersToday != null) {
                allUsers.addAll(usersToday);
            }
        }
        return allUsers.size();
    }
}
