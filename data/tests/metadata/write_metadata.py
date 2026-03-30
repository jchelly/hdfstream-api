#!/bin/env python

import msgpack

metadata = {
    "description" : "metadata test directory",
    "labels"      : {
        "test_data.hdf5"    : "first test data file",
        "test_data_2d.hdf5" : "second test data file",
    }
}

data = msgpack.packb(metadata)

with open("metadata.msgpack", "wb") as f:
    f.write(data)
