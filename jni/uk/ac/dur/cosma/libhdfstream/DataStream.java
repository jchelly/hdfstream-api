package uk.ac.dur.cosma.libhdfstream;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.lang.RuntimeException;
import java.lang.AutoCloseable;

public class DataStream extends InputStream implements AutoCloseable {

    private HDFStream hdfstream;

    private long stream_ptr = 0;
    private long stream_bufsize = 0;
    private native void c_free(long ptr);
    private native long c_read_chunk(long ptr, byte buffer[]);

    // Buffer used when acting as an InputStream
    private byte stream_buffer[] = null;
    private int buffered_bytes = 0;
    private int buffer_offset = 0;
    private boolean end_of_stream = false;
    private boolean closed = false;

    public DataStream(long ptr, long buffersize, HDFStream hs) {
	stream_ptr = ptr;
	stream_bufsize = buffersize;
        hdfstream = hs;
    }

    // Directly read into supplied buffer. Used to implement read().
    private long readChunk(byte buffer[]) throws IOException {
        if(buffer.length < stream_bufsize)
            throw new IOException("Buffer size is too small in readChunk()");
        long nr_bytes_read = c_read_chunk(stream_ptr, buffer);
        if(nr_bytes_read < 0) {
            // Something went wrong
            close();
            throw new IOException("Unable to read data stream");
        } else if(nr_bytes_read==0) {
            // This is the end of the stream
            end_of_stream = true;
            close();
            return -1;
        } else {
            // We read a new chunk of data
            return nr_bytes_read;
        }
    }

    public int read() throws IOException {
        byte b[] = new byte[1];
        read(b);
        return b[0];
    }

    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    public int read(byte[] b, int off, int len) throws IOException {

        // Check if we're shutting down
        if(hdfstream.shuttingDown())throw new IOException("Server is shutting down");

        // Check that this is not the end of the stream
        if(end_of_stream)return -1;

        // Check that the stream was not closed
        if(closed)throw new IOException("Attempt to read from closed stream!");

        // Handle the case where we can receive directly into the buffer and avoid a copy
        if((stream_buffer == null) && (b.length >= stream_bufsize) && (off == 0) && (len >= stream_bufsize)) {
            return (int) readChunk(b);
        }

        // Ensure the buffer is allocated
        if(stream_buffer == null)stream_buffer = new byte[(int) stream_bufsize];

        // If we have no data in the buffer, try to read some
        if(buffer_offset >= buffered_bytes) {
            int nr_bytes_read = (int) readChunk(stream_buffer);
            if(nr_bytes_read < 0) {
                // This is the end of the stream
                return -1;
            } else {
                // We read a new chunk of data
                buffered_bytes = nr_bytes_read;
                buffer_offset = 0;
            }
        }

        // Determine how many bytes will be returned
        int nr_bytes_available = buffered_bytes - buffer_offset;
        int nr_bytes_read = nr_bytes_available;
        if(nr_bytes_read > len)nr_bytes_read = len;

        // Copy data to the output buffer
        for(int i=0; i<nr_bytes_read; i+=1) {
            b[off+i] = stream_buffer[buffer_offset+i];
        }

        // Advance buffer offset past the bytes we returned
        buffer_offset += nr_bytes_read;

        return nr_bytes_read;
    }

    public int available() throws IOException {
        if(end_of_stream || closed) {
            return 0;
        } else {
            return buffered_bytes - buffer_offset;
        }
    }

    public void close() {
        if(!closed) {
            closed = true;
            c_free(stream_ptr);
        }
    }
}
