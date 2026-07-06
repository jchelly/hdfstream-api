#!/bin/env python
#
# Test downloading HDF5 file structure information
#

import numpy as np
import multiprocessing as mp
import h5py
import hdfstream


def get_remote_datasets(group, path=""):
    """
    Recursively extract all datasets in a RemoteGroup
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, hdfstream.RemoteDataset):
            result.append(path+name)
        else:
            result += get_remote_datasets(obj, path=path+name+"/")
    return result


def get_remote_groups(group, path=""):
    """
    Recursively extract all sub-groups in a RemoteGroup
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, hdfstream.RemoteGroup):
            result.append(path+name)
            result += get_remote_groups(obj, path=path+name+"/")
    return result


def get_local_datasets(group, path=""):
    """
    Recursively extract all datasets in a h5py.Group
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, h5py.Dataset):
            result.append(path+name)
        else:
            result += get_local_datasets(obj, path=path+name+"/")
    return sorted(result)


def get_local_groups(group, path=""):
    """
    Recursively extract all sub-groups in a h5py.Group
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, h5py.Group):
            result.append(path+name)
            result += get_local_groups(obj, path=path+name+"/")
    return sorted(result)


def compare_metadata(local_root, remote_root):
    """
    Check that we get the same metadata from hdfstream and h5py.

    local_root  - the h5py.Group to read
    remote_root - the hdfstream.RemoteGroup to read

    These objects should refer to the same underlying file.
    """

    assert isinstance(remote_root, hdfstream.RemoteGroup)
    assert not isinstance(local_root, hdfstream.RemoteGroup)

    # Check that we have the same set of groups
    local_groups = get_local_groups(local_root)
    remote_groups = get_remote_groups(remote_root)
    if local_groups != remote_groups:
        raise RuntimeError("Group names do not match!")

    print(f"{len(local_groups)} groups match")

    # Check that we have the same datasets
    local_datasets = get_local_datasets(local_root)
    remote_datasets = get_remote_datasets(remote_root)
    if local_datasets != remote_datasets:
        raise RuntimeError("Dataset names do not match!")

    # Check dataset shapes
    for name in local_datasets:
        local_shape = local_root[name].shape
        remote_shape = remote_root[name].shape
        if local_shape != remote_shape:
            raise RuntimeError("Dataset shapes do not match!")

    # Check dataset types
    for name in local_datasets:
        local_dtype = local_root[name].dtype
        remote_dtype = remote_root[name].dtype
        if local_dtype.kind != remote_dtype.kind:
            raise RuntimeError("Data type kinds do not match!")
        if local_dtype.itemsize != remote_dtype.itemsize:
            raise RuntimeError("Data type sizes do not match!")

    print(f"{len(local_datasets)} datasets match")

    # Check attributes
    nr_attrs = 0
    for name in local_datasets + local_groups:
        local_attrs = local_root[name].attrs
        remote_attrs = remote_root[name].attrs
        # Check we have the same attribute names
        if sorted(list(local_attrs)) != sorted(list(remote_attrs)):
            raise RuntimeError("Attribute names do not match!")
        # Check attribute values
        for attr_name in local_attrs:
            local_attr = np.asarray(local_attrs[attr_name])
            remote_attr = np.asarray(remote_attrs[attr_name])
            if local_attr.shape != remote_attr.shape:
                raise RuntimeError("Attribute shapes do not match!")
            if np.any(local_attr != remote_attr):
                raise RuntimeError("Attribute values do not match!")
        nr_attrs += len(local_attrs)

    print(f"{nr_attrs} attributes match")


def test_file_metadata(server, process_nr, virtual_root, virtual_name, real_name):
    """
    Run metadata test on a single file
    """

    # Initialize the random number generator in a repeatable way
    rng = np.random.default_rng(seed=process_nr)

    # Get list of groups in the file
    remote_dir = hdfstream.RemoteDirectory(server, virtual_root)
    remote_root = remote_dir[virtual_name]["/"]
    group_names = get_remote_groups(remote_root)

    # Open the local file
    local_root = h5py.File(real_name, "r")

    # Run test on subgroups
    nr_groups = 10
    for group_nr in range(nr_groups):

        # Pick a random subgroup to test
        group_name = group_names[rng.integers(len(group_names))]

        # Check metadata agrees
        compare_metadata(local_root[group_name], remote_root[group_name])

    # Run test using the root group
    compare_metadata(local_root, remote_root)


def test_eagle_snapshot_metadata(server, process_nr):
    """
    Run the metadata test on files in an EAGLE snapshot
    """
    virtual_names = "EAGLE/Fiducial_models/RefL0012N0188/snapshot_028_z000p000/snap_028_z000p000.{file_nr}.hdf5"
    virtual_names = [virtual_names.format(file_nr=file_nr) for file_nr in range(16)]
    real_names = "./EAGLE/Fiducial_models/RefL0012N0188/snapshot_028_z000p000/snap_028_z000p000.{file_nr}.hdf5"
    real_names = [real_names.format(file_nr=file_nr) for file_nr in range(16)]

    # Initialize the random number generator in a repeatable way
    rng = np.random.default_rng(seed=process_nr)

    # Compare random files from the snapshot between h5py and hdfstream
    nr_reps = 20
    for _ in range(nr_reps):
        i = rng.integers(len(virtual_names))
        test_file_metadata(server, process_nr, "EAGLE", virtual_names[i], real_names[i])


def run_metadata_test(server, nr_processes):
    """
    Run multiple instances of the metadata test in parallel
    """
    args = [(server, i) for i in range(nr_processes)]
    with mp.Pool(nr_processes) as p:
        p.starmap(test_eagle_snapshot_metadata, args)
    print("Metadata test done.")


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(description="Test downloading HDF5 metadata")
    parser.add_argument("server", type=str, help="Address of the server (e.g. 'https://localhost:8443/hdfstream')")
    parser.add_argument("nr_processes", type=int, help="Number of parallel processes sending requests")
    args = parser.parse_args()

    # Run the test
    run_metadata_test(args.server, args.nr_processes)
