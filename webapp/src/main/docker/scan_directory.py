#!/bin/env python
#
# Script to generate hdfstream config for the Docker container.
# Intended to run on container startup. Scans a mounted directory
# for files to serve.
#
import os
from pathlib import Path


def media_type(filename):
    """
    Guess the media type (mime type) given a filename
    """
    # Get the file extension
    ext = Path(filename).suffix

    # Pick a media type
    if ext == ".hdf5" or ext == ".h5":
        return "application/x-hdf5"
    elif ext == ".yml":
        return "application/yaml"
    elif ext == ".txt":
        # Text is assumed to be UTF-8 encoded
        return "text/plain;charset=UTF-8"
    else:
        # Anything unrecognised is assumed to be binary data
        return "application/octet-stream"


def scan_directory(real_directory, virtual_directory, output_file):

    with open(output_file, "w") as out:

        # Get the full, real data path
        real_directory = Path(real_directory).absolute()

        # Loop over all files in the data directory
        for real_filename in Path(real_directory).rglob("*"):
            if real_filename.is_file():
                # Get real and virtual paths to this file
                real_absolute_path = real_filename.absolute()
                relative_path = real_absolute_path.relative_to(real_directory)
                virtual_absolute_path = (Path(virtual_directory) / relative_path).relative_to("/")

                # Get size, last modification time and type
                st = os.stat(real_absolute_path)
                size = st.st_size
                modtime = st.st_mtime
                mtype = media_type(real_absolute_path)

                # Write out the config line for this file
                out.write(f"{virtual_absolute_path}, {real_absolute_path}, {int(size)}, {int(modtime)}, {mtype}\n")


if __name__ == "__main__":

    import argparse
    parser = argparse.ArgumentParser(description="Scan the specified directory and generate a hdfstream config file")
    parser.add_argument("real_directory", type=str, help="Name of the directory with the data")
    parser.add_argument("virtual_directory", type=str, help="Name of the vitual directory in the output config")
    parser.add_argument("output_file", type=str, help="Where to write the output")
    args = parser.parse_args()
    scan_directory(args.real_directory, args.virtual_directory, args.output_file)
