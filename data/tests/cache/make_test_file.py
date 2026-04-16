#!/bin/env python
#
# Create files containing several groups
#
import h5py

nr_files = 5
nr_groups = 5
for file_nr in range(nr_files):
    filename = f"test_data{file_nr}.hdf5"
    with h5py.File(filename, "w") as f:
        for group_nr in range(nr_groups):

            # Create a group
            groupname = f"Group{group_nr}"
            g = f.create_group(groupname)
            g.attrs["file_nr"] = file_nr
            g.attrs["group_nr"] = group_nr

            # Create a subgroup
            subgroupname = f"Group{group_nr}/SubGroup{group_nr}"
            sg = f.create_group(subgroupname)

            # Create a dataset
            g[f"Dataset{group_nr}"] = (int(group_nr),)
