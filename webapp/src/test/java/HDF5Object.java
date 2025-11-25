import java.util.LinkedHashMap;
import java.io.IOException;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.Map;
import java.util.List;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageFormat;
import static org.junit.jupiter.api.Assertions.*;


public class HDF5Object {

    // Type of this object
    private String hdf5_object;

    // Attributes of this object
    public LinkedHashMap<String,NDArray> attributes;

    // Target path, if this is a soft link
    public String target;

    // Any members, if this is a group
    public LinkedHashMap<String,HDF5Object> members;

    // Type, kind and shape, if this is a dataset
    public String type;
    public String kind;
    public long[] shape;

    // Raw data for this dataset
    public NDArray data;

    // Full path within the HDF5 file
    public String path;

    // Unpack a msgpack stream representing a HDF5 object
    public HDF5Object(MessageUnpacker unpacker, String path) throws IOException {

        // Store the path to this object
        this.path = path;

        // A HDF5 object is stored as a map where the keys are strings
        int nr_entries = unpacker.unpackMapHeader();
        for(int entry_nr=0; entry_nr<nr_entries; entry_nr+=1) {

            // Get the name of this entry
            String name = unpacker.unpackString();

            // Get the value associated with this entry
            switch(name) {
            case "hdf5_object":
                // This specifies the object type
                hdf5_object = unpacker.unpackString();;
                break;
            case "target":
                // This specifies the target of a soft link
                assertEquals(hdf5_object, "soft_link");
                target = unpacker.unpackString();;
                break;
            case "attributes":
                // This is a map of (name, ndarray) pairs
                assertTrue(List.of("dataset", "group").contains(hdf5_object), "Objects with attributes must be groups or datasets");
                int nr_attrs = unpacker.unpackMapHeader();
                attributes = new LinkedHashMap<String,NDArray>();
                for(int attr_nr=0; attr_nr<nr_attrs; attr_nr+=1) {
                    // Unpack the name
                    String attr_name = unpacker.unpackString();;
                    // Unpack the attribute data
                    NDArray attr_data = new NDArray(unpacker);
                    attributes.put(attr_name, attr_data);
                }
                break;
            case "members":
                // This is a map of (name, hdf5 object) pairs
                assertEquals(hdf5_object, "group");
                int nr_members = unpacker.unpackMapHeader();
                members = new LinkedHashMap<String,HDF5Object>();
                for(int member_nr=0; member_nr<nr_members; member_nr+=1) {
                    // Unpack the name
                    String member_name = unpacker.unpackString();
                    // Recursively unpack the member object
                    if(unpacker.getNextFormat() == MessageFormat.NIL) {
                        // In case we hit the recursion limit
                        unpacker.unpackNil();
                        members.put(member_name, null);
                    } else {
                        // Compute path of member object
                        String member_path;
                        if(path.equals("/"))
                            member_path = "/"+member_name;
                        else
                            member_path = path+"/"+member_name;
                        // Unpack member object
                        HDF5Object member_object = new HDF5Object(unpacker, member_path);
                        members.put(member_name, member_object);
                    }
                }
                break;
            case "type":
                // Data type for datasets
                assertEquals(hdf5_object, "dataset");
                type = unpacker.unpackString();;
                break;
            case "kind":
                // Kind for datasets
                assertEquals(hdf5_object, "dataset");
                kind = unpacker.unpackString();;
                break;
            case "shape":
                // Shape for datasets
                assertEquals(hdf5_object, "dataset");
                int nr_dims = unpacker.unpackArrayHeader();
                shape = new long[nr_dims];
                for(int dim_nr=0; dim_nr<nr_dims; dim_nr+=1) {
                    shape[dim_nr] = unpacker.unpackLong();
                }
                break;
            case "data":
                // Datasets have a list of encoded ndarrays with their contents.
                assertEquals(hdf5_object, "dataset");
                data = new NDArray(unpacker);
                break;
            }
        }

        // Sanity checks
        if(hdf5_object == null)throw new IOException("HDF5 object type not set");
    }

    public LinkedHashMap<String,HDF5Object> getMembers() {
        return members;
    }

    public LinkedHashMap<String,HDF5Object> getMembersByType(String type) {

        LinkedHashMap<String,HDF5Object> objects = new LinkedHashMap<String,HDF5Object>();
        if(members != null) {
            for (Map.Entry<String,HDF5Object> entry : members.entrySet()) {
                String name = entry.getKey();
                HDF5Object object = entry.getValue();
                if(object != null) {
                    if(object.hdf5_object.equals(type)) {
                        objects.put(name, object);
                    }
                } else {
                    // groups which have not been loaded are null
                    if(type.equals("group")) {
                        objects.put(name, null);
                    }
                }
            }
        }
        return objects;
    }

    public LinkedHashMap<String,HDF5Object> getGroups() {
        return getMembersByType("group");
    }

    public LinkedHashMap<String,HDF5Object> getDatasets() {
        return getMembersByType("dataset");
    }

    public String getType() {
        return type;
    }

    public String getShape() {
        String s = "[";
        for(int i=0; i<shape.length; i+=1) {
            s += Long.toString(shape[i]);
            if(i < shape.length-1)
                s += ",";
        }
        s += "]";
        return s;
    }

    public long[] getDimensions() {
        return shape;
    }

    public LinkedHashMap<String,NDArray> getAttributes() {
        return attributes;
    }

    public static void main(String[] args) throws Exception {
        // java -cp ./msgpack-core-0.9.6.jar:HDF5Object.jar HDF5Object
        FileInputStream input = new FileInputStream("snap.msgpack");
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(input);
        HDF5Object root = new HDF5Object(unpacker, "");
    }

    public String getPath() {
        return path;
    }

    public boolean isDataset() {
        return hdf5_object.equals("dataset");
    }

    public boolean isGroup() {
        return hdf5_object.equals("group");
    }

    public boolean isSoftLink() {
        return hdf5_object.equals("soft_link");
    }

    public boolean isDisplayable() {
        if(isDataset() && (shape.length <= 2) && kind.equals("")) {
            return true;
        } else {
            return false;
        }
    }
}
