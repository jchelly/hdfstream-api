package uk.ac.dur.cosma.hdfstream;

import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessagePack;
import org.msgpack.core.MessageFormat;
import org.msgpack.value.ValueType;
import org.msgpack.core.MessageTypeException;

import java.io.IOException;


/*
  Class to decode msgpack slice info for all dimensions

  A DimensionArray is encoded as a msgpack array of DimensionInfo objects.
*/
public class DimensionArray {

    int nr_dims;
    public DimensionInfo dims[];

    public DimensionArray(MessageUnpacker unpacker) throws InvalidSliceException, IOException {

        /* Determine number of dimensions */
        try {
            nr_dims = unpacker.unpackArrayHeader();
        } catch (MessageTypeException e) {
            throw new InvalidSliceException("Unexpected msgpack type decoding slice");
        }
        if(nr_dims > SliceInfo.MAX_DIMENSIONS)
            throw new InvalidSliceException("Too many dimensions");

        /* Decode start and count in each dimension */
        dims = new DimensionInfo[nr_dims];
        for(int dim_nr=0; dim_nr<nr_dims; dim_nr+=1)
            dims[dim_nr] = new DimensionInfo(unpacker);

        /* Sanity check: we only allow slices to differ in the first dimension,
           so start and count for dimensions after the first must be scalars. */
        for(int dim_nr=1; dim_nr<nr_dims; dim_nr+=1) {
            if(dims[dim_nr].start.length > 1) {
                throw new InvalidSliceException("Slices may only differ in the first dimension");
            }
        }
    }

    /*
      Use the start and end in each dimension to build a SliceInfo object,
      which contains concatenated start and count arrays suitable for
      passing to the C API.

      TODO: make the C API accept slices encoded in a more compact way?
    */
    public SliceInfo getSliceInfo() {

        // Determine number of slices
        int nr_slices = 1;
        if(nr_dims > 0)nr_slices = dims[0].start.length;

        // Allocate a new slice
        SliceInfo slice = new SliceInfo();

        /* Allocate storage for slice starts and counts in each dimension */
        slice.nr_slices = nr_slices;
        slice.rank = nr_dims;
        slice.starts = new long[nr_slices*slice.rank];
        slice.counts = new long[nr_slices*slice.rank];
        /* Construct the slices using the start/end info for each dimension */
        int offset = 0;
        for(int slice_nr=0; slice_nr<slice.nr_slices; slice_nr+=1) {
            for(int dim_nr=0; dim_nr<slice.rank; dim_nr+=1) {
                /* Find the starting offset of this slice in this dimension */
                if(dims[dim_nr].start.length > 1) {
                    slice.starts[offset] = dims[dim_nr].start[slice_nr];
                } else {
                    slice.starts[offset] = dims[dim_nr].start[0];
                }
                /* Find the count of this slice in this dimension */
                if(dims[dim_nr].count.length > 1) {
                    slice.counts[offset] = dims[dim_nr].count[slice_nr];
                } else {
                    slice.counts[offset] = dims[dim_nr].count[0];
                }
                offset += 1;
            }
        }
        return slice;
    }
}
