import java.nio.ByteOrder;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.value.Value;
import org.msgpack.value.ValueType;

public class NDArray {

    public String type = null;
    public String kind = null;
    public long[] shape = null;
    public long nbytes = -1;
    private byte[] data = null;
    private ByteBuffer buffer = null;
    private String[] strings = null;
    boolean is_nd = false;
    boolean is_vlen = false;

    private int data_class;
    private static final int DATA_CLASS_STRING  = 0;
    private static final int DATA_CLASS_INTEGER = 1;
    private static final int DATA_CLASS_REAL    = 2;
    private static final int DATA_CLASS_OTHER   = 3;

    int element_size;

    // Unpack a msgpack stream representing an array
    public NDArray(MessageUnpacker unpacker) throws IOException {

        // An ndarray object is stored as a map where the keys are strings
        int nr_entries = unpacker.unpackMapHeader();
        for(int entry_nr=0; entry_nr<nr_entries; entry_nr+=1) {
            String name = unpacker.unpackString();;
            switch(name) {
            case "nd":
                is_nd = unpacker.unpackBoolean();
                break;
            case "vlen":
                is_vlen = unpacker.unpackBoolean();
                break;
            case "type":
                type = unpacker.unpackString();;
                break;
            case "kind":
                kind = unpacker.unpackString();;
                break;
            case "shape":
                int nr_dims = unpacker.unpackArrayHeader();
                shape = new long[nr_dims];
                for(int dim_nr=0; dim_nr<nr_dims; dim_nr+=1) {
                    shape[dim_nr] = unpacker.unpackLong();
                }
                break;
            case "nbytes":
                nbytes = unpacker.unpackLong();
                break;
            case "data":
                if(is_nd) {
                    // This is an array of fixed size types stored as a binary blob.
                    // We should have an array of msgpack_bin objects. First
                    // unpack them all.
                    int nr_bins = unpacker.unpackArrayHeader();
                    byte[][] bins = new byte[nr_bins][];
                    for(int bin_nr=0; bin_nr<nr_bins; bin_nr+=1) {
                        int nr_bytes = unpacker.unpackBinaryHeader();
                        bins[bin_nr] = unpacker.readPayload(nr_bytes);
                    }
                    // Compute the total size
                    int total_size = 0;
                    for(int bin_nr=0; bin_nr<nr_bins; bin_nr+=1) {
                        total_size += bins[bin_nr].length;
                    }
                    // Check size matches nbytes field
                    if(total_size != nbytes)throw new IOException("Length of data field does not match nbytes!");
                    // Allocate output array
                    data = new byte[total_size];
                    // Copy the data from the bin objects
                    int offset = 0;
                    for(byte[] bin : bins) {
                        System.arraycopy(bin, 0, data, offset, bin.length);
                        offset += bin.length;
                    }
                } else if(is_vlen) {
                    // This is an array of a variable length type
                    int nr = unpacker.unpackArrayHeader();
                    strings = new String[nr];
                    for(int i=0; i<nr; i+=1) {
                        Value v = unpacker.unpackValue();
                        if(v.getValueType() == ValueType.STRING) {
                            strings[i] = v.asStringValue().asString();
                        } else {
                            throw new IOException("Unsupported vlen type");
                        }
                    }
                } else {
                    // This type is not implemented
                    unpacker.skipValue();
                }
                break;
            }
        }

        if(is_nd) {

            // Wrap the data in a bytebuffer
            buffer = ByteBuffer.wrap(data);

            // Determine endian-ness, if specified, and remove marker
            String data_type = type;
            if(type.startsWith("<")) {
                buffer.order(ByteOrder.LITTLE_ENDIAN);
                data_type = data_type.substring(1);
            } else if (type.startsWith(">")) {
                buffer.order(ByteOrder.BIG_ENDIAN);
                data_type = data_type.substring(1);
            }

            // Try to interpret the data type info
            if(data_type.startsWith("S")) {
                // This is a string
                data_class = DATA_CLASS_STRING;
            } else if(data_type.startsWith("f")) {
                // This is a flolat or double
                data_class = DATA_CLASS_REAL;
            } else if(data_type.startsWith("i")) {
                // This is a signed integer
                data_class = DATA_CLASS_INTEGER;
            } else if(data_type.startsWith("u")) {
                // This is an unsigned integer
                data_class = DATA_CLASS_INTEGER;
            } else {
                // Don't know how to interpret this
                data_class = DATA_CLASS_OTHER;
            }

            // Try to get the size of the data type
            try {
                element_size = Integer.parseInt(data_type.substring(1));
            } catch (NumberFormatException e) {
                data_class = DATA_CLASS_OTHER;
            }
        }
    }

    public String getElementAsString(int index) throws IOException {

        // If it's an array of strings, return the string
        if(strings != null) {
            return strings[index];
        }

        // Otherwise we can only handle fixed size types
        if(is_nd == false)throw new IOException("vlen data not implemented");

        // Can't currently handle compound/array/vlen types etc
        if(!kind.equals("")) {
            throw new IOException("only int/float/string values are implemented");
        }

        // Convert index into a byte offset
        int offset = index*element_size;

        // Try to convert the value to a string
        buffer.position(offset);
        switch(data_class) {
        case DATA_CLASS_STRING:
            byte[] b = new byte[element_size];
            buffer.get(b, 0, element_size);
            return new String(b, StandardCharsets.UTF_8).replace("\0","");
        case DATA_CLASS_INTEGER:
        case DATA_CLASS_REAL:
            throw new IOException("can't return integer or real data as string!");
        default:
            throw new IOException("unsupported type: "+type);
        }
    }

    public int getElementAsInt(int index) throws IOException {

        // Check if it's an array of vlen strings
        if(strings != null)
            throw new IOException("Can't return string element as int");

        // Otherwise we can only handle fixed size types
        if(is_nd == false)throw new IOException("vlen data not implemented");

        // Can't currently handle compound/array/vlen types etc
        if(!kind.equals(""))throw new IOException("only int/float/string values are implemented");

        // Convert index into a byte offset
        int offset = index*element_size;

        // Try to convert the value
        buffer.position(offset);
        switch(data_class) {
        case DATA_CLASS_STRING:
        case DATA_CLASS_REAL:
            throw new IOException("Can't return string or float data as int");
        case DATA_CLASS_INTEGER:
            switch(element_size) {
            case 1:
                byte bval = buffer.get();
                return bval;
            case 2:
                short sval = buffer.getShort();
                return sval;
            case 4:
                int ival = buffer.getInt();
                return ival;
            case 8:
                throw new IOException("Can't return long value in an int");
            default:
                throw new IOException("unsupported data type size");
            }
        default:
            throw new IOException("unsupported type: "+type);
        }
    }

    public long getElementAsLong(int index) throws IOException {

        // Check if it's an array of vlen strings
        if(strings != null)
            throw new IOException("Can't return string element as long");

        // Otherwise we can only handle fixed size types
        if(is_nd == false)throw new IOException("vlen data not implemented");

        // Can't currently handle compound/array/vlen types etc
        if(!kind.equals(""))throw new IOException("only int/float/string values are implemented");

        // Convert index into a byte offset
        int offset = index*element_size;

        // Try to convert the value
        buffer.position(offset);
        switch(data_class) {
        case DATA_CLASS_STRING:
        case DATA_CLASS_REAL:
            throw new IOException("Can't return string or float data as int");
        case DATA_CLASS_INTEGER:
            switch(element_size) {
            case 1:
                byte bval = buffer.get();
                return bval;
            case 2:
                short sval = buffer.getShort();
                return sval;
            case 4:
                int ival = buffer.getInt();
                return ival;
            case 8:
                long lval = buffer.getLong();
                return lval;
            default:
                throw new IOException("unsupported data type size");
            }
        default:
            throw new IOException("unsupported type: "+type);
        }
    }

    public float getElementAsFloat(int index) throws IOException {

        // Check if it's an array of vlen strings
        if(strings != null)
            throw new IOException("Can't return string element as float");

        // Otherwise we can only handle fixed size types
        if(is_nd == false)throw new IOException("vlen data not implemented");

        // Can't currently handle compound/array/vlen types etc
        if(!kind.equals(""))throw new IOException("only int/float/string values are implemented");

        // Convert index into a byte offset
        int offset = index*element_size;

        // Try to convert the value
        buffer.position(offset);
        switch(data_class) {
        case DATA_CLASS_STRING:
        case DATA_CLASS_INTEGER:
            throw new IOException("Can't return string or int data as float");
        case DATA_CLASS_REAL:
            switch(element_size) {
            case 4:
                float fval = buffer.getFloat();
                return fval;
            case 8:
                throw new IOException("Can't return double data as float");
            default:
                throw new IOException("unsupported data type size");
            }
        default:
            throw new IOException("unsupported type: "+type);
        }
    }

    public double getElementAsDouble(int index) throws IOException {

        // Check if it's an array of vlen strings
        if(strings != null)
            throw new IOException("Can't return string element as double");

        // Otherwise we can only handle fixed size types
        if(is_nd == false)throw new IOException("vlen data not implemented");

        // Can't currently handle compound/array/vlen types etc
        if(!kind.equals(""))throw new IOException("only int/float/string values are implemented");

        // Convert index into a byte offset
        int offset = index*element_size;

        // Try to convert the value
        buffer.position(offset);
        switch(data_class) {
        case DATA_CLASS_STRING:
        case DATA_CLASS_INTEGER:
            throw new IOException("Can't return string or int data as float");
        case DATA_CLASS_REAL:
            switch(element_size) {
            case 4:
                float fval = buffer.getFloat();
                return fval;
            case 8:
                double dval = buffer.getDouble();
                return dval;
            default:
                throw new IOException("unsupported data type size");
            }
        default:
            throw new IOException("unsupported type: "+type);
        }
    }
}
