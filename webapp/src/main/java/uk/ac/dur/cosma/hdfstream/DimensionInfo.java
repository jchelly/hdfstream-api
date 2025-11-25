package uk.ac.dur.cosma.hdfstream;

import org.msgpack.core.MessageUnpacker;
import org.msgpack.value.ValueType;
import org.msgpack.core.MessageTypeException;

import java.io.IOException;


/*
  Class to decode msgpack slice information for one dimension

  For each dimension we have a two element array with [start, count].

  start may be an integer or an array of integers. If it's an array,
  that indicates that we have multiple slices and there is one slice
  for each element.

  count may be an integer or an array of integers. If it's an array,
  it must be the same shape as start. Otherwise we assume that we have
  the same count for every slice.
*/
public class DimensionInfo {

    public long[] start = null;
    public long[] count = null;

    public DimensionInfo(MessageUnpacker unpacker) throws InvalidSliceException, IOException {

        try {

            // Read the array header. Should have two elements.
            int n = unpacker.unpackArrayHeader();
            if(n != 2)throw new InvalidSliceException("Expected two element array with start:end indexes for slice(s)");

            // Decode the start indexes as an array of long
            start = MsgpackUtils.unpackLongArrayOrScalar(unpacker, SliceInfo.MAX_SLICES);

            // Decode the counts as an array of long
            count = MsgpackUtils.unpackLongArrayOrScalar(unpacker, SliceInfo.MAX_SLICES);

            // Do some sanity checks
            if((count.length != 1) && (start.length != count.length))
                throw new InvalidSliceException("Count array must be the same length as start array if not scalar");
            if((count.length == 0) || (start.length == 0))
                throw new InvalidSliceException("Start and count arrays must not have zero length");

        } catch (MessageTypeException e) {
            throw new InvalidSliceException("Unexpected msgpack data type decoding slice");
        }
    }
}
