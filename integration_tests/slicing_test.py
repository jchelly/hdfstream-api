#!/bin/env python
#
# Test sending many simultaneous requests for HDF5 data to the server.
# Compares the results to directly reading with h5py.
#
# This requires that the server is running and we have a set of files
# where the virtual names match the real names, aside from some prefix.
#

import time

import numpy as np
import multiprocessing as mp

import hdfstream
import h5py

hdfstream.verify_cert(False)

import get_users as gu


def get_filenames(directory, path=""):
    """
    Recursively extract all filenames from a RemoteDirectory.
    """
    # Store files in this directory
    result = []
    for filename in directory.files:
        result.append(path+filename)

    # Search subdirectories
    for dir_name, dir_obj in directory.directories.items():
        result += get_filenames(dir_obj, path=path+dir_name+"/")

    return result


def get_datasets(group, path=""):
    """
    Recursively extract all datasets in a RemoteGroup
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, hdfstream.RemoteDataset):
            result.append(path+name)
        else:
            result += get_datasets(obj, path=path+name+"/")
    return result


def slicing_test(process_nr, server, virtual_base_dir, filesystem_base_dir, duration, user, password):
    """
    Request random datasets from the server
    """

    # Get directory listing from the server
    root = hdfstream.RemoteDirectory(server, virtual_base_dir, user=user, password=password)
    filenames = get_filenames(root)

    # Initialize the random number generator in a repeatable way
    rng = np.random.default_rng(seed=process_nr)

    # Loop over random files to access
    nr_datasets = 5   # Number of random datasets to read from each file
    nr_slices   = 20  # Number of random slices to read from each dataset
    t0 = time.time()
    while time.time() < t0 + duration:

        # Pick a file at random
        filename = filenames[rng.integers(len(filenames))]

        # Open the file's root HDF5 group
        h5file = root[filename]

        # Open the file directly with h5py
        h5file_check = h5py.File(filesystem_base_dir+"/"+filename, "r")

        # Get list of datasets in the file
        dataset_names = get_datasets(h5file)

        # Loop over datasets to read
        for dataset_nr in range(nr_datasets):

            # Open a dataset
            datasetname = dataset_names[rng.integers(len(dataset_names))]
            dset = h5file[datasetname]

            # Loop over dataset slices to read
            for slice_nr in range(nr_slices):

                # Pick a random slice along the first dimension
                max_size = min(dset.shape[0], 1000000)
                size_to_read = rng.integers(max_size+1)
                offset_to_read = rng.integers(dset.shape[0]-size_to_read+1)
                assert(size_to_read+offset_to_read <= dset.shape[0])

                # Fetch the slice
                slice_data = dset[offset_to_read:offset_to_read+size_to_read,...]

                # Read the same data with h5py
                h5py_data = h5file_check[datasetname][offset_to_read:offset_to_read+size_to_read,...]

                # Report if values match
                if np.all(slice_data==h5py_data):
                    print(filename, datasetname, size_to_read, offset_to_read," OK")
                else:
                    raise RuntimeError("Values do not match!")

        # Close the real HDF5 file
        h5file_check.close()

    print(f"Process {process_nr} completed.")


def run_slicing_test(nr_processes, server, virtual_base_dir, filesystem_base_dir, duration):
    """
    Run multiple instances of the slicing test in parallel
    """
    print("Slicing test started.")
    args = [(i, server, virtual_base_dir, filesystem_base_dir, duration, gu.get_name(i), gu.get_password(i)) for i in range(nr_processes)]
    with mp.Pool(nr_processes) as p:
        p.starmap(slicing_test, args)
    print("Slicing test done.")


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(description="Test retreiving data via the hdfstream python module")
    parser.add_argument("server", type=str, help="Address of the server (e.g. 'https://localhost:8443/hdfstream')")
    parser.add_argument("virtual_base_dir", type=str, help="Virtual path to the directory to use")
    parser.add_argument("filesystem_base_dir", type=str, help="Real path to the directory to use")
    parser.add_argument("nr_processes", type=int, help="Number of parallel processes sending requests")
    parser.add_argument("duration", type=int, help="Duration of the test in seconds")
    args = parser.parse_args()

    # Get username and password
    gu.init()

    # Run the test
    run_slicing_test(args.nr_processes, args.server, args.virtual_base_dir, args.filesystem_base_dir, args.duration)
