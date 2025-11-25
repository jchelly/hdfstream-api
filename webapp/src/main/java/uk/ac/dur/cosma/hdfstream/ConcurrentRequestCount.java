package uk.ac.dur.cosma.hdfstream;

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

    ConcurrentHashMap<String, Semaphore> user;
    int max_requests;

    public ConcurrentRequestCount(int max_requests) {

        /* Record number of concurrent requests allowed */
        this.max_requests = max_requests;

        /* Create an empty map of {username : semaphore} pairs */
        user = new ConcurrentHashMap<String, Semaphore>();
    }

    public void acquire(String username) {

        /* Empty string name indicates not authenticated */
        String name = (username != null) ? username : "";

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
