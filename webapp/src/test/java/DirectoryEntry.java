import java.util.LinkedHashMap;
import java.io.IOException;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.Map;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageFormat;
import org.msgpack.value.ValueType;

/*
  Class to decode files and directories.
*/
public class DirectoryEntry {

    // Type of this file (not set for directories)
    public String type = null;

    // Sub-directories (not set for files)
    public LinkedHashMap<String,DirectoryEntry> directories = null;

    // Files (not set for files)
    public LinkedHashMap<String,DirectoryEntry> files = null;

    // Description (not set for files)
    String description = null;

    // Labels (not set for files)
    public LinkedHashMap<String,String> labels = null;

    // Last modification time
    public long last_modified = -1;

    // Size
    public long size = -1;

    // Unpack a msgpack stream representing a HDF5 object
    public DirectoryEntry(MessageUnpacker unpacker) throws IOException {

        // A directory entry is stored as a map where the keys are strings
        int nr_entries = unpacker.unpackMapHeader();
        for(int entry_nr=0; entry_nr<nr_entries; entry_nr+=1) {

            // Get the name of this entry
            String name = unpacker.unpackString();

            // Get the value associated with this entry
            switch(name) {
            case "type":
                type = unpacker.unpackString();;
                break;
            case "size":
                size = unpacker.unpackLong();
                break;
            case "last_modified":
                last_modified = unpacker.unpackLong();
                break;
            case "description": {
                ValueType val_type = unpacker.getNextFormat().getValueType();
                if(val_type == ValueType.NIL) {
                    // Description may be nil
                    unpacker.unpackNil();
                    } else {
                    description = unpacker.unpackString();
                }
                break;
            }
            case "labels": {
                ValueType val_type = unpacker.getNextFormat().getValueType();
                if(val_type == ValueType.NIL) {
                    // Labels may be nil
                    unpacker.unpackNil();
                } else {
                    int nr_labels = unpacker.unpackMapHeader();
                    labels = new LinkedHashMap<String,String>();
                    for(int label_nr=0; label_nr<nr_labels; label_nr+=1) {
                        String label_key = unpacker.unpackString();;
                        String label_value = unpacker.unpackString();;
                        labels.put(label_key, label_value);
                    }
                }
                break;
            }
            case "files": {
                int nr_files = unpacker.unpackMapHeader();
                files = new LinkedHashMap<String,DirectoryEntry>();
                for(int file_nr=0; file_nr<nr_files; file_nr+=1) {
                    String file_name = unpacker.unpackString();;
                    DirectoryEntry file_object = new DirectoryEntry(unpacker);
                    files.put(file_name, file_object);
                }
                break;
            }
            case "directories": {
                int nr_directories = unpacker.unpackMapHeader();
                directories = new LinkedHashMap<String,DirectoryEntry>();
                for(int directory_nr=0; directory_nr<nr_directories; directory_nr+=1) {
                    String directory_name = unpacker.unpackString();
                    ValueType val_type = unpacker.getNextFormat().getValueType();
                    if(val_type == ValueType.MAP) {
                        DirectoryEntry directory_object = new DirectoryEntry(unpacker);
                        directories.put(directory_name, directory_object);
                    } else if(val_type == ValueType.NIL) {
                        unpacker.unpackNil(); // Make sure we consume the encoded null
                        directories.put(directory_name, null);
                    } else {
                        throw new IOException("Unexpected object type in directory map!");
                    }
                }
                break;
            }
            default:
                throw new IOException("Unrecognised field " + name + " when decoding directory entry!");
            }
        }
        // Sanity checks
        if(type.equals("directory")) {
            if(directories == null)throw new IOException("DirectoryEntry for directory missing directories field!");
            if(files == null)throw new IOException("DirectoryEntry for directory missing files field!");
        } else {
            if(directories != null)throw new IOException("DirectoryEntry for file should not have directories field!");
            if(files != null)throw new IOException("DirectoryEntry for file should not have files field!");
        }
    }
}
