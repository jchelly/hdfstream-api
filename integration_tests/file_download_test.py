#!/bin/env python
#
# Test downloading files
#

import os
import os.path
import hashlib
import multiprocessing as mp
import hdfstream

from utils import get_filenames

chunk_size = 1024*1024


def hash_stream(f):
    """
    Read and hash all bytes until end of stream then return hash.
    """
    m = hashlib.md5()
    while True:
        data = f.read(chunk_size)
        if len(data) == 0:
            break
        else:
            m.update(data)
    return m.hexdigest()


def test_file_download(server, virtual_path, real_path):
    """
    Check that downloaded file is identical to the file on disk
    """

    # Connect to the service
    connection = hdfstream.Connection(server)

    # Initialize hash
    download_hash = hashlib.md5()

    # Request the file and hash it
    download_hash = hash_stream(connection.open_file(virtual_path, 'rb'))

    # Now hash the real file
    file_hash = hashlib.md5()
    with open(real_path, "rb") as infile:
        file_hash = hash_stream(infile)

    if file_hash != download_hash:
        raise RuntimeError("File hashes don't match!")
    else:
        print(f"File: {virtual_path} - OK")


def run_download_test(server, nr_processes, virtual_base_dir, filesystem_base_dir):
    """
    Test multiple parallel downloads
    """

    # Get directory listing from the server
    root = hdfstream.RemoteDirectory(server, virtual_base_dir)
    filenames = get_filenames(root)

    # Make lists of real and virtual filenames
    virtual_paths = [virtual_base_dir+"/"+filename for filename in filenames]
    real_paths    = [filesystem_base_dir+"/"+filename for filename in filenames]

    # Create array of args for process pool
    args = [(server, vp, rp) for i, (vp, rp) in enumerate(zip(virtual_paths, real_paths))]

    # Run the test
    with mp.Pool(nr_processes) as p:
        p.starmap(test_file_download, args)


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(description="Test downloading complete files")
    parser.add_argument("server", type=str, help="Address of the server (e.g. 'https://localhost:8443/hdfstream')")
    parser.add_argument("virtual_base_dir", type=str, help="Virtual path to the directory to use")
    parser.add_argument("filesystem_base_dir", type=str, help="Real path to the directory to use")
    parser.add_argument("nr_processes", type=int, help="Number of parallel processes sending requests")
    args = parser.parse_args()

    # Run the test
    run_download_test(args.server, args.nr_processes, args.virtual_base_dir, args.filesystem_base_dir)
