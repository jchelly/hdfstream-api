#!/bin/env python
#
# Test downloading files
#

import glob
import os.path
import hashlib
import tarfile
import multiprocessing as mp

import hdfstream

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


def hash_tar_contents(server, virtual_path):
    """
    Return hashes of the files in a virtual directory tar archive
    """

    # Connect to the service
    connection = hdfstream.Connection(server)

    # Dict to store results
    hashes = {}

    # Request the tar file
    stream = connection.open_file(virtual_path, 'rb')

    # Loop over files in the archive and hash contents
    with tarfile.open(fileobj=stream, mode="r|") as tf:
        while member := tf.next():
            hashes[member.name] = hash_stream(tf.extractfile(member))

    return hashes


def test_tar_download(server, virtual_path, real_path_map):
    """
    Check that hashes of tarred files agree with the real file system
    """

    # Get hashes of files in the tar archive
    virtual_hashes = hash_tar_contents(server, virtual_path)

    # Check that we have the expected set of filenames
    if sorted(list(virtual_hashes)) != sorted(list(real_path_map)):
        print("Virtual: ", sorted(list(virtual_hashes)))
        print("Real: ", sorted(list(real_path_map)))
        raise RuntimeError("Filenames in tar file are incorrect!")

    # Check that hashes agree with the real files
    for virtual_name, real_name in real_path_map.items():

        # Hash the file directly
        with open(real_name, "rb") as infile:
            real_hash = hash_stream(infile)

        # Check against hash from the tar file
        if real_hash != virtual_hashes[virtual_name]:
            raise RuntimeError("Hash of extracted file is wrong!")

    nr_files = len(virtual_hashes)
    print(f"Tar file: {virtual_path} - {nr_files} files OK")


def test_snapshot_download(server, snap_nr, virtual_base_dir, filesystem_base_dir):
    """
    Check that we can download an EAGLE snapshot correctly
    """

    # Find directory name and redshift label on disk
    real_dir = f"{filesystem_base_dir.rstrip('/')}/snapshot_{snap_nr:03d}_z???p???"
    real_dir = glob.glob(real_dir)[0]
    zlabel = real_dir[-8:]

    # Get corresponding virtual directory name
    virtual_dir = f"{virtual_base_dir.rstrip('/')}/snapshot_{snap_nr:03d}_{zlabel}"

    # Get names of the files on the real filesystem
    real_filenames = glob.glob(f"{real_dir}/snap_{snap_nr:03d}_{zlabel}.*.hdf5")
    assert len(real_filenames) > 0

    # Get corresponding virtual snapshot names
    virtual_filenames = []
    for real_filename in real_filenames:
        basename = os.path.basename(real_filename)
        virtual_filenames.append(f"{virtual_dir}/{basename}")

    # Make dict to map real to virtual names
    name_map = {}
    for rf, vf in zip(real_filenames, virtual_filenames):
        name_map[vf] = rf

    test_tar_download(server, virtual_dir, name_map)


def run_snapshot_download_test(server, nr_processes, virtual_base_dir, filesystem_base_dir):
    """
    Test downloading multiple EAGLE snapshots simultaneously
    """
    args = [(server, snap_nr, virtual_base_dir, filesystem_base_dir) for snap_nr in range(26,29)]
    with mp.Pool(nr_processes) as p:
        p.starmap(test_snapshot_download, args)


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(description="Test downloading directories as tar files")
    parser.add_argument("server", type=str, help="Address of the server (e.g. 'https://localhost:8443/hdfstream')")
    parser.add_argument("virtual_base_dir", type=str, help="Virtual path to the directory to use")
    parser.add_argument("filesystem_base_dir", type=str, help="Real path to the directory to use")
    parser.add_argument("nr_processes", type=int, help="Number of parallel processes sending requests")
    args = parser.parse_args()

    # Run the test
    run_snapshot_download_test(args.server, args.nr_processes, args.virtual_base_dir, args.filesystem_base_dir)
