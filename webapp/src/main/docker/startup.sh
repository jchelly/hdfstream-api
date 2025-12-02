#!/bin/bash -e

# Generate the config file
python3 /opt/hdfstream/scan_directory.py /opt/hdfstream/data/ / /opt/hdfstream/config.csv

# Start the service
cd /usr/local/tomcat/bin && catalina.sh
