package uk.ac.dur.cosma.hdfstream;

import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageFormat;
import org.msgpack.value.ValueType;

import java.io.IOException;


public class SliceInfo {

    // Impose some limits on request sizes
    public static final int MAX_STRING_LENGTH=8*1024*1024;
    public static final int MAX_SLICES=16777216;
    public static final int MAX_DIMENSIONS=8;

    // Arrays which define the slices
    public int nr_slices;
    public int rank;
    public long starts[];
    public long counts[];

    // Make a new, empty list of slices
    public SliceInfo() {
        nr_slices = -1;
        rank = -1;
        starts = null;
        counts = null;
    }

    /*
      Parse a string specifying one or more dataset slices.

      For each dimension we specify a range with start:stop, where start and
      stop are integers. Elements start to stop-1 inclusive will be returned.

      For multidimensional arrays, dimensions are separated by commas.
      Multiple N-d slices are separated by semicolons.
    */
    public SliceInfo(String slice_descriptor) throws InvalidSliceException {

        // Check the string length
        if(slice_descriptor.length() > MAX_STRING_LENGTH) {
            throw new InvalidSliceException("Slice string is too long!");
        }

        // Separate n-d slices are separated by semicolons
        String[] slices = slice_descriptor.split(";", -1);
        nr_slices = slices.length;
        if(nr_slices > MAX_SLICES) {
            throw new InvalidSliceException("Too many slices!");
        }

        // Loop over slices
        int offset = 0;
        for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {

            String[] dims;
            if(slices[slice_nr].trim().length() == 0) {
                // Special case for scalars: an empty string indicates a 0-d slice
                dims = new String[0];
            } else {
                // Otherwise, dimensions are comma separated
                dims = slices[slice_nr].split(",", -1);
            }

            // Check number of dimensions
            if(dims.length > MAX_DIMENSIONS) {
                throw new InvalidSliceException("Too many dimensions!");
            }

            // Allocate starts and counts parameters needed by the C API
            if(slice_nr == 0) {
                rank = dims.length;
                starts = new long[nr_slices*rank];
                counts = new long[nr_slices*rank];
            } else {
                if(rank != dims.length)throw new InvalidSliceException("Slices must all have the same number of dimensions!");
            }

            // Extract start and end values for each dimension
            for(int dim_nr=0; dim_nr<rank; dim_nr+=1) {
                String[] fields = dims[dim_nr].split(":", -1);
                long start, end;
                if(fields.length == 1) {
                    // Single element in this dimension
                    try {
                        start = Long.parseLong(fields[0].trim());
                    } catch (NumberFormatException e) {
                        throw new InvalidSliceException("Unable to parse start index as integer");
                    }
                    end = start + 1;
                } else if(fields.length == 2){
                    // Range of elements in this dimension
                    try {
                        start = Long.parseLong(fields[0].trim());
                        end = Long.parseLong(fields[1].trim());
                    } catch (NumberFormatException e) {
                        throw new InvalidSliceException("Unable to parse start/end index as integer");
                    }
                } else {
                    // Invalid slice
                    throw new InvalidSliceException("Unable to interpret slice specification!");
                }
                // Don't accept python style negative indexes
                if((start < 0) || (end < 0) || (end - start < 0))throw new InvalidSliceException("Slice indexes must not be negative");
                starts[offset] = start;
                counts[offset] = end - start;
                offset += 1;
            }
        }
    }

    /*
      Unpack an array of msgpack encoded slices

      This is an array with one element per dimension.
      For each dimension we have a two element (start index, end index) array.
      End indexes are exclusive, using the python indexing convention.

      In the first dimension the start and end may themselves be arrays.
      If so, they must be the same size and their size is the number of
      slices to read. Otherwise, the start and end are msgpack integers.
     */
    public static SliceInfo fromUnpacker(MessageUnpacker unpacker) throws InvalidSliceException, IOException {

        // Check that we have something to unpack
        if(!unpacker.hasNext())throw new InvalidSliceException("No messagepack slice data to unpack");

        // Check the type of the next object from the unpacker
        ValueType type = MsgpackUtils.getNextType(unpacker);
        if(type == ValueType.STRING) {

            // We have a string slice specifier
            return new SliceInfo(MsgpackUtils.unpackLimitedString(unpacker, MAX_STRING_LENGTH));

        } else if(type == ValueType.ARRAY) {

            /* Decode start and end info in each dimension */
            DimensionArray darray = new DimensionArray(unpacker);
            /* Create the slice object */
            return darray.getSliceInfo();

        } else {
            // Don't know what to do with this!
            throw new InvalidSliceException("slice parameter must be a string or an array");
        }
    }

    /*
      Generate the msgpack encoded form of this SliceInfo object.

      Test case slices expressed as strings can be converted to
      msgpack to test handling of post requests.
    */
    public void pack(MessagePacker packer) throws IOException {
        packer.packArrayHeader(rank);
        for(int dim_nr=0; dim_nr<rank; dim_nr+=1) {
            packer.packArrayHeader(2);
            if(dim_nr == 0) {
                /* First dimension, so we have one start per slice */
                if(nr_slices == 1) {
                    /* We have one slice only */
                    packer.packLong(starts[0]);
                } else {
                    /* We have multiple slices */
                    packer.packArrayHeader(nr_slices);
                    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1)
                        packer.packLong(starts[slice_nr*rank+dim_nr]);
                }
                /* Check if we have multiple different counts */
                boolean counts_is_array = false;
                for(int slice_nr=1; slice_nr<nr_slices; slice_nr+=1) {
                    if(counts[slice_nr*rank] != counts[0])
                        counts_is_array = true;
                }
                if(counts_is_array) {
                    // Have an array of counts
                    packer.packArrayHeader(nr_slices);
                    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1)
                        packer.packLong(counts[slice_nr*rank+dim_nr]);
                } else {
                    // Have just one count
                    packer.packLong(counts[0]);
                }
            } else {
                /* Not the first dimension, so all starts and counts are the same */
                packer.packLong(starts[dim_nr]);
                packer.packLong(counts[dim_nr]);
            }
        }
    }
}
