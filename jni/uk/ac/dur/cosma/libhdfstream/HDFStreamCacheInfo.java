package uk.ac.dur.cosma.libhdfstream;

public class HDFStreamCacheInfo {
    public int process_state;
    public int nr_file_cache_hits;
    public int nr_file_cache_misses;

    public HDFStreamCacheInfo(int fields[]) {
        process_state = fields[0];
        nr_file_cache_hits = fields[1];
        nr_file_cache_misses = fields[2];
    }

    public int getState() {
        return process_state;
    }

    public int getHits() {
        return nr_file_cache_hits;
    }

    public int getMisses() {
        return nr_file_cache_misses;
    }
}
