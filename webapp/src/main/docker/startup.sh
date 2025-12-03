#!/bin/bash -e

# Check if we have an env var with a virtual directory prefix
if [[ -z "${HDFSTREAM_PREFIX}" ]]; then
    # Default if not set
    prefix="Data"
else
    prefix="${HDFSTREAM_PREFIX}"
fi

# Generate the config file
python3 /opt/hdfstream/scan_directory.py /opt/hdfstream/data/ "${prefix}" /opt/hdfstream/config.csv

# Start the service. Use exec so that shutdown signal from docker reaches tomcat.
exec /usr/local/tomcat/bin/catalina.sh run
