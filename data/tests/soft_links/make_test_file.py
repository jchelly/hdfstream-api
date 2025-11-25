#!/bin/env python

import h5py
import numpy as np

f = h5py.File("test_data.hdf5", "w")

# Make a group with a link to it
f.create_group("Group")
f["LinkToGroup"] = h5py.SoftLink("/Group")

# Make a dataset with a link to it
x = np.arange(10, dtype=int)
f["Dataset"] = x
f["LinkToDataset"] = h5py.SoftLink("/Dataset")

# Make a dataset in the group
f["Group/DatasetInGroup"] = 2*x

# Make a broken link
#f["BrokenLink"] = h5py.SoftLink("/NoSuchDataset")

f.close()
