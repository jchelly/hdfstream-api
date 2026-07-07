#!/bin/env python

import hdfstream
import h5py


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
    Recursively extract all datasets in a group
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, (hdfstream.RemoteDataset, h5py.Dataset)):
            result.append(path+name)
        elif isinstance(obj, (hdfstream.RemoteGroup, h5py.Group)):
            result += get_datasets(obj, path=path+name+"/")
    return result


def get_groups(group, path=""):
    """
    Recursively extract all sub-groups in a group
    """
    result = []
    for name, obj in group.items():
        if isinstance(obj, (hdfstream.RemoteGroup, h5py.Group)):
            result.append(path+name)
            result += get_groups(obj, path=path+name+"/")
    return result
