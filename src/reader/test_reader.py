#!/bin/env python

import subprocess
import numpy as np

import msgpack
import msgpack_numpy as m
m.patch()
import numpy as np


def send_string(f, s):
    data = (s+"\0").encode()
    slen = np.uint64(len(data))
    f.write(slen)
    f.write(data)


class DatasetReader:

    def __init__(self):
        """
        Start the reader process
        """
        executable = "../../build/src/reader/dataset_reader"
        self.process = subprocess.Popen(executable, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def read_dataset(self, filename, datasetname, start, count):
        """
        Read a dataset slice and return a numpy array
        """

        cmd = np.int32(1)
        self.process.stdin.write(cmd)

        rank = np.int32(len(start))
        start = np.asarray(start).astype(np.uint64)[0:rank]
        count = np.asarray(count).astype(np.uint64)[0:rank]

        # Send file and dataset names to the process
        send_string(self.process.stdin, filename)
        send_string(self.process.stdin, datasetname)
        # Send rank
        self.process.stdin.write(rank)
        # Send start and count
        self.process.stdin.write(start)
        self.process.stdin.write(count)
        # Send buffer size to use
        buffer_size = np.uint64(1024)
        self.process.stdin.write(buffer_size)
        self.process.stdin.flush()

        # Receive total size of response
        buf = self.process.stdout.read(8)
        nr_bytes = np.frombuffer(buf, dtype=np.uint64, count=1)[0]
        if nr_bytes == 0:
            raise RuntimeError("Zero sized response!")

        # Allocate buffer to receive the full response
        all_data = np.ndarray(nr_bytes, dtype=np.uint8)
        offset = 0

        # Receive data blocks
        while nr_bytes > 0:

            # Read size of next block
            buf = self.process.stdout.read(8)
            block_size = int(np.frombuffer(buf, dtype=np.uint64, count=1)[0])
            if block_size == 0:
                raise RuntimeError("Zero sized block!")

            # Read the block
            data = self.process.stdout.read(block_size)
            data = np.frombuffer(data, dtype=np.uint8, count=block_size)
            all_data[offset:offset+block_size] = data
            offset += block_size
            nr_bytes -= block_size

        return msgpack.unpackb(all_data)


if __name__ == "__main__":

    filename="/cosma7/data/Eagle/ScienceRuns/Planck1/L0025N0188/PE/REFERENCE/data/snapshot_028_z000p000/snap_028_z000p000.0.hdf5"
    datasetname="PartType1/Coordinates"

    start = (0,0)
    count = (10000,3)

    dr = DatasetReader()

    data = dr.read_dataset(filename, datasetname, start, count)

    print(data)
    print(data.shape)
    print(data.dtype)

