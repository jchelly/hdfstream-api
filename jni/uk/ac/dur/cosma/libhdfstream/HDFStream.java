package uk.ac.dur.cosma.libhdfstream;

import java.io.IOException;
import java.io.File;
import java.nio.ByteBuffer;
import java.nio.file.StandardCopyOption;
import java.nio.file.Files;


public class HDFStream {

    static {
        System.loadLibrary("hdfstream_jni");
    }

    private long ptr;
    private int refCount = 1;
    private volatile boolean stopping = false;
    public int maxDims = -1;
    public int maxSlices = -1;

    private native int c_get_max_slices();
    private native int c_get_max_dims();
    private native long c_new(int nr_processes, int max_open_files, int max_open_datasets,
                              int file_cache_check_interval, int file_cache_expiry_interval);
    private native long c_new_with_executable(int nr_processes, String executable,
                                              int max_open_files, int max_open_datasets,
                                              int file_cache_check_interval, int file_cache_expiry_interval);
    private native void c_free(long ptr);
    private native long c_open_slices(long ptr, String file_name, String dataset_name,
                                      int nr_slices, int rank, long start[], long count[],
                                      long buffer_size);
    private native long c_open_object(long ptr, String file_name, String object_name,
				     int max_depth, long buffer_size, long data_size_limit);

    private native void c_cache_info(long ptr, int worker_nr, int fields[]);

    public HDFStream(int nr_processes, int max_open_files, int max_open_datasets,
                     int file_cache_check_interval, int file_cache_expiry_interval) {
        ptr = c_new(nr_processes, max_open_files, max_open_datasets, file_cache_check_interval, file_cache_expiry_interval);
        if(ptr==0)throw new RuntimeException("Failed to start process pool");
        maxDims = c_get_max_dims();
        maxSlices = c_get_max_slices();
        refCount = 1;
    }

    public HDFStream(int nr_processes, String executable, int max_open_files, int max_open_datasets,
                     int file_cache_check_interval, int file_cache_expiry_interval) {
        if(executable==null)throw new RuntimeException("Null executable name passed to HDFStream constructor");
        ptr = c_new_with_executable(nr_processes, executable, max_open_files, max_open_datasets, file_cache_check_interval, file_cache_expiry_interval);
        if(ptr==0)throw new RuntimeException("Failed to start process pool");
        maxDims = c_get_max_dims();
        maxSlices = c_get_max_slices();
        refCount = 1;
    }

    public void free() {
        if(ptr==0)throw new RuntimeException("Process pool is not allocated");
        c_free(ptr);
        ptr = 0;
    }

    public synchronized void acquireReference() throws IOException {
        if((refCount <= 0) || stopping) {
            // We already shut down
            throw new IOException("Server is shutting down");
        } else {
            // Count the new reference
            refCount += 1;
        }
    }

    public synchronized void releaseReference() {
        if(refCount <= 0) {
            // We already shut down. Indicates wrong sequence of acquire/release calls.
            throw new RuntimeException("Reference count already zero before releasing");
        } else {
            // Free a reference
            refCount -= 1;
            // Check if we reached zero. New references cannot be acquired after this.
            if(refCount==0)free();
        }
    }

    public synchronized int getReferenceCount() {
        return refCount;
    }

    public synchronized void shutDown() {
        stopping = true;
    }

    public boolean shuttingDown() {
        return stopping;
    }

    public DataStream openDatasetSlices(String file_name, String dataset_name,
                                        int nr_slices, int rank, long start[], long count[],
                                        long buffer_size) throws IOException{
        if(ptr==0)throw new RuntimeException("Process pool is not allocated");
        if(file_name==null)throw new IOException("Null file name passed to openDatasetSlice");
        if(dataset_name==null)throw new IOException("Null dataset name passed to openDatasetSlice");
        if(start==null)throw new IOException("Null start parameter passed to openDatasetSlice");
        if(count==null)throw new IOException("Null count parameter passed to openDatasetSlice");
        if(start.length != count.length)throw new IOException("Inconsistent start and length passed to openDatasetSlice");
        if(nr_slices < 1)throw new IOException("Non-positive number of slices in openDatasetSlice");
        if(nr_slices > maxSlices)throw new IOException("Too many slices in openDatasetSlice");
        if(rank < 0)throw new IOException("Negative rank in openDatasetSlice");
        if(rank > maxDims)throw new IOException("Rank too large in openDatasetSlice");
        if(start.length != nr_slices*rank)throw new IOException("Length of start parameter not equal to nr_slices*rank in openDatasetSlice");
        if(count.length != nr_slices*rank)throw new IOException("Length of count parameter not equal to nr_slices*rank in openDatasetSlice");
        for(int i=0; i<nr_slices*rank; i+=1) {
            if(start[i] < 0)throw new IOException("Negative start parameter in openDatasetSlice");
            if(count[i] < 0)throw new IOException("Negative count parameter in openDatasetSlice");
        }
        long stream_ptr = c_open_slices(ptr, file_name, dataset_name, nr_slices,
                                        rank, start, count, buffer_size);
	if(stream_ptr==0)throw new IOException("Failed to open dataset slice");
	return new DataStream(stream_ptr, buffer_size, this);
    }

    public DataStream openObject(String file_name, String object_name, int max_depth,
                                long buffer_size, long data_size_limit) throws IOException {
        if(ptr==0)throw new RuntimeException("Process pool is not allocated");
        if(file_name==null)throw new IOException("Null file name passed to openObject");
        if(object_name==null)throw new IOException("Null object name passed to openObject");
	long stream_ptr = c_open_object(ptr, file_name, object_name, max_depth, buffer_size, data_size_limit);
	if(stream_ptr==0)throw new IOException("Failed to open object");
	return new DataStream(stream_ptr, buffer_size, this);
    }

    public HDFStreamCacheInfo getCacheInfo(int worker_nr) {
        if(ptr==0)throw new RuntimeException("Process pool is not allocated");
        int fields[] = new int[3];
        c_cache_info(ptr, worker_nr, fields);
        return new HDFStreamCacheInfo(fields);
    }
}
