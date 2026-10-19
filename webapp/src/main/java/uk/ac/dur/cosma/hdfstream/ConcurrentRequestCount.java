package uk.ac.dur.cosma.hdfstream;

import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Semaphore;

/*
  Class to limit the number of concurrent requests per user.
  This is to prevent one user from tying up all of the processes
  in the pool.
*/
public class ConcurrentRequestCount {

    private ConcurrentHashMap<String, Semaphore> user;
    private int max_requests;
    private int nr_parts;

    public static String splitName(String name, int n) {
        if(n < 1)return name;
        String[] parts = name.split("\\.");
        n = Math.min(n, parts.length);
        return String.join(".", Arrays.copyOf(parts, n));
    }

    public ConcurrentRequestCount(int max_requests, int nr_parts) {

        /* Record number of concurrent requests allowed */
        this.max_requests = max_requests;
        this.nr_parts = nr_parts;

        /* Create an empty map of {username : semaphore} pairs */
        user = new ConcurrentHashMap<String, Semaphore>();
    }

    public void acquire(String username) {

        /* Empty string name indicates not authenticated */
        String name = (username != null) ? username : "";

        /* Only use the first n components of the name */
        name = splitName(name, nr_parts);

        /* max_requests=0 indicates that no limit should be applied */
        if(max_requests==0)return;

        /* Get user specific semaphore, creating it if necessary */
        Semaphore sem = user.computeIfAbsent(name, s -> new Semaphore(max_requests));

        /* Acquire a permit from the semaphore (may block) */
        sem.acquireUninterruptibly();
    }

    public void release(String username) {

        /* Empty string name indicates not authenticated */
        String name = (username != null) ? username : "";

        /* Only use the first n components of the name */
        name = splitName(name, nr_parts);

        /* max_requests=0 indicates that no limit should be applied */
        if(max_requests==0)return;

        /* Release a user specific permit */
        user.get(name).release();
    }

    /* Report number of running queries per user. Only works if a limit is set. */
    public HashMap<String, Integer> getCounts() {

        HashMap<String, Integer> result = new HashMap<String, Integer>();
        if(max_requests > 0) {
            for (Map.Entry<String, Semaphore> e : user.entrySet()) {
                int nr_permits = e.getValue().availablePermits();
                int nr_requests = max_requests - nr_permits;
                if(nr_requests > 0)
                    result.put(e.getKey(), nr_requests);
            }
        }
        return result;
    }
}
