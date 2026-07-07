#!/bin/env python

import requests
import msgpack
import time
import numpy as np
import multiprocessing as mp
import hdfstream
import h5py

import urllib3
urllib3.disable_warnings()

from utils import get_filenames, get_datasets

def request_slice(session, server, filename, dataset, start, count, rng):
    """
    Request a dataset slice from the server

    Here we don't use the hdfstream module because we want to be able to
    introduce some deliberate mistakes in the http request and not have
    them caught until they reach the server.
    """

    expect_success = True
    remove_params = []

    # Randomly introduce errors
    i = rng.integers(20)
    if i == 0:
        # Invalid filename
        filename += "not-a-real-file"
        expect_success = False
    elif i == 1:
        # Invalid dataset
        dataset += "/not-a-dataset"
        expect_success = False
    elif i == 2:
        # Delete a required parameter
        remove_params.append("object")
        expect_success = False
    elif i == 3:
        # Out of range (presumably) count
        count[0] = (1 << 30)
        expect_success = False
    elif i == 4:
        # Out of range (presumably) start
        start[0] = (1 << 30)
        expect_success = False

    # Set http request parameters
    slice_string = ",".join([str(s)+":"+str(s+c) for s,c in zip(start,count)])
    params = {
        "object" : dataset,
        "slice"  : slice_string,
    }
    for rp in remove_params:
        del params[rp]

    # Construct the URL for the required file
    url = f"{server}/msgpack/{filename}"

    # Request the data
    response = session.get(url, params=params, stream=True, verify=False)

    # Check for unexpected results
    if response.ok and not(expect_success):
        raise RuntimeError(f"Unexpected success! (i={i}, file={filename}, dataset={dataset}, start={start}, count={count})")
    if not(response.ok) and expect_success:
        raise RuntimeError(f"Unexpected failure! (i={i}, file={filename}, dataset={dataset}, start={start}, count={count})")

    # Sometimes we don't bother downloading the result, or just download some of it
    download = rng.integers(10)
    if download == 0:
        # Don't read response body
        del response
        return None
    elif download == 1:
        # Try to read part of response (maybe!)
        for data in response.iter_content():
            if rng.integers(2) == 0:
                break
        del response
        return None

    # Get the type of the response
    media_type = response.headers.get("Content-Type")

    if response.ok:
        # Request succeeded, so decode it
        return msgpack.unpack(response.raw, object_hook=hdfstream.decoding.decode_hook)
    else:
        # Failed requests return an error message as the body of the response
        if media_type == "application/x-msgpack":
            err = msgpack.unpack(response.raw)["error"]
            raise IOError(f"Request failed, msgpack error: {err}")
        else:
            raise IOError(f"Request failed, text error: {response.text}")


def pick_test_datasets(server, virtual_base_dir):
    """
    Identify some files and datasets to use for the bad requests test
    """

    test_datasets = [] # Will contain (filename, datasetname, shape) tuples

    # Get directory listing from the server
    root = hdfstream.RemoteDirectory(server, virtual_base_dir)
    filenames = get_filenames(root)

    # Initialize the random number generator in a repeatable way
    rng = np.random.default_rng(seed=0)

    # Loop over random files to access
    nr_files    = 100  # Number of files to read
    nr_datasets = 10   # Number of random datasets to read from each file
    for file_nr in range(nr_files):

        # Pick a random file and open it
        filename = filenames[rng.integers(len(filenames))]
        h5file = root[filename]

        # Get list of datasets in this file
        dataset_names = get_datasets(h5file)

        # Pick some random datasets to store
        for dataset_nr in range(nr_datasets):
            dataset_name = dataset_names[rng.integers(len(dataset_names))]
            dataset_shape = h5file[dataset_name].shape
            test_datasets.append((virtual_base_dir+"/"+filename, dataset_name, dataset_shape))

    return test_datasets


def bad_requests_test(server, test_datasets, process_nr, duration, verbose):
    """
    Repeatedly request slices of random datasets from the list
    """

    connection = hdfstream.Connection.new(server, user=None)

    # Initialize the random number generator in a repeatable way
    rng = np.random.default_rng(seed=process_nr)

    req_nr = 0
    t0 = time.time()
    while time.time() < (t0+duration):

        # Pick a dataset
        i = rng.integers(len(test_datasets))
        filename, dataset, shape = test_datasets[i]

        # Pick a random slice along the first dimension
        max_size = min(shape[0], 1000000)
        size_to_read = rng.integers(max_size+1)
        offset_to_read = rng.integers(shape[0]-size_to_read+1)
        assert(size_to_read+offset_to_read <= shape[0])

        # Construct start and count request parameters
        start = [0,]*len(shape)
        count = [max_size,]+list(shape[1:])

        # Send the request
        try:
            data = request_slice(connection.session, server, filename, dataset, start, count, rng)
        except IOError as e:
            if(verbose):
                print(f"Request {req_nr} failed, {str(e).strip()}")
        else:
            if data is not None:
                if(verbose):
                    print(f"Request {req_nr} OK")
            else:
                if(verbose):
                    print(f"Request {req_nr} OK, but download aborted")

        req_nr += 1
    print(f"Process {process_nr} send {req_nr} requests")


def run_bad_requests_test(server, virtual_base_dir, filesystem_base_dir, nr_processes, duration, verbose):
    """
    Run the bad requests test on multiple processes in parallel
    """

    print("Fetching list of test datasets")
    test_datasets = pick_test_datasets(server, virtual_base_dir)

    print("Running tests")
    args = [(server, test_datasets, i, duration, verbose) for i in range(nr_processes)]
    with mp.Pool(nr_processes) as p:
        p.starmap(bad_requests_test, args)
    print("Done.")


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(description="Test sending bad requests to the server")
    parser.add_argument("server", type=str, help="Address of the server (e.g. 'https://localhost:8443/hdfstream')")
    parser.add_argument("virtual_base_dir", type=str, help="Virtual path to the directory to use")
    parser.add_argument("filesystem_base_dir", type=str, help="Real path to the directory to use")
    parser.add_argument("nr_processes", type=int, help="Number of parallel processes sending requests")
    parser.add_argument("duration", type=int, help="Duration of the test in seconds")
    parser.add_argument("--verbose", action="store_true", help="Show result of every request")

    args = parser.parse_args()

    # Run the test
    run_bad_requests_test(args.server, args.virtual_base_dir, args.filesystem_base_dir,
                          args.nr_processes, args.duration, args.verbose)
