package uk.ac.dur.cosma.hdfstream;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;

public class StreamCopier {

    public static void copyStream(InputStream instream, OutputStream outstream, int buffer_size) throws IOException {
        byte[] buf = new byte[buffer_size];
        int bytes_read;
        do {
            bytes_read = instream.read(buf);
            if(bytes_read > 0) outstream.write(buf, 0, bytes_read);
        } while(bytes_read >= 0);
    }

    public static byte[] copyStreamAndReturnIfSmall(InputStream instream, OutputStream outstream, int buffer_size, int max_size) throws IOException {
        byte[] buf = new byte[buffer_size];
        int bytes_read;
        int max_left_to_buffer = max_size;
        ByteArrayOutputStream smallCache = new ByteArrayOutputStream();
        do {
            bytes_read = instream.read(buf);
            if(bytes_read > 0){
                outstream.write(buf, 0, bytes_read);
                int nr_to_buffer = Math.min(max_left_to_buffer, bytes_read);
                if(nr_to_buffer > 0)smallCache.write(buf, 0, nr_to_buffer);
                max_left_to_buffer -= bytes_read;
            }
        } while(bytes_read >= 0);
        if(max_left_to_buffer >= 0) {
            return smallCache.toByteArray();
        } else {
            return null;
        }
    }
}
