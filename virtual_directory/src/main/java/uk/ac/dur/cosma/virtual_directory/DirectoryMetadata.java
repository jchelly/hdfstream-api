package uk.ac.dur.cosma.virtual_directory;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Map;
import java.util.LinkedHashMap;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageTypeException;
import org.msgpack.core.MessageSizeException;


public class DirectoryMetadata {

    // Maximum number of file descriptions to load
    private final int MAX_FILES = 1024;
    
    // Description of this directory
    public String description = null;

    // Short descriptions of files in this directory
    public LinkedHashMap<String, String> file_labels = null;

    public DirectoryMetadata(String real_path) {

	try (FileInputStream fis = new FileInputStream(real_path);
             MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(fis)) {

	    // Should have a map with one or two entries
	    int nr_entries = unpacker.unpackMapHeader();
	    if((nr_entries < 1) || (nr_entries > 2))
		throw new IOException("Metadata map has unexpected size");
	    for(int entry_nr=0; entry_nr<nr_entries; entry_nr+=1) {
		String field = unpacker.unpackString();
		switch(field) {
		case "description":
		    description = unpacker.unpackString();
		    break;
		case "file_labels":
		    int nr_labels = unpacker.unpackMapHeader();
		    for(int label_nr=0; label_nr<nr_labels; label_nr+=1) {
			String file_name = unpacker.unpackString();
			String file_label = unpacker.unpackString();
			file_labels.put(file_name, file_label);
		    }
		    break;
		default:
		    throw new IOException("Unexpected field name");
		}
	    }
	    
	} catch (IOException e) {
	    // We couldn't read the file
	    description = "Unable to read directory metadata";
	    file_labels = null;
	} catch (MessageSizeException e) {
	    // An object is too large
	    description = "Unexpected object size in directory metadata";
	    file_labels = null;
	} catch (MessageTypeException e) {
	    // Wrong data type
	    description = "Unexpected data type in directory metadata";
	    file_labels = null;
        }
    }
}
