#
# Docker commands if not using github actions
#
# Build:   docker build --tag "hdfstream-api" .
# Run:     docker run -d -p 8080:8080 -v /path/on/host:/opt/hdfstream/data:ro --name hdfstream-api hdfstream-api
# Log in:  docker exec -it hdfstream-api /bin/bash
# Stop:    docker stop hdfstream-api
# Remove:  docker container rm hdfstream-api
#
# Build dependencies:
#
# Here we start from an Ubuntu 26.04 image so HDF5 1.14 is in the repo
#
FROM ubuntu:26.04 AS builder

# Install various build tools and libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    zlib1g \
    zlib1g-dev \
    openjdk-21-jdk-headless \
    maven \
    libmsgpack-c-dev \
    libhdf5-dev \
    && rm -rf /var/lib/apt/lists/*

# Download and extract Tomcat
RUN mkdir -p /tmp/tomcat && \
    wget https://downloads.apache.org/tomcat/tomcat-9/v9.0.119/bin/apache-tomcat-9.0.119.tar.gz -O tomcat.tar.gz && \
    tar -xf tomcat.tar.gz -C /tmp/tomcat --strip-components=1

# Copy over and build the hdfstream-api source code
COPY . /hdfstream-api
RUN cd /hdfstream-api \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCONFIG_DIR=../webapp/src/main/docker/ \
    && make \
    && make test \
    && make install

#
# Set up the final image
#
# This starts from the same Ubuntu Noble image but with tomcat installed.
# Need to copy over the libraries and data set up in the builder, and
# configure tomcat to find the libraries.
#
FROM ubuntu:26.04

# We need curl to check the service is running and python to generate the config file
RUN apt-get update && apt-get install -y curl python3

ENV DEBIAN_FRONTEND=noninteractive
ENV CATALINA_HOME=/opt/tomcat
ENV PATH=$CATALINA_HOME/bin:$PATH

# Install libraries
RUN apt-get update && apt-get install -y \
    openjdk-21-jdk-headless \
    libhdf5-dev \
    libmsgpack-c-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the extracted Tomcat files from the builder stage
COPY --from=builder /tmp/tomcat $CATALINA_HOME

# Remove default webapps
RUN rm -rf ${CATALINA_HOME}/webapps/*

# Copy the built libraries from the builder
COPY --from=builder /usr/local /usr/local

# Copy the web app package over
COPY --from=builder /hdfstream-api/build/webapp/target/hdfstream.war ${CATALINA_HOME}/webapps/

# Copy file with path to libhdfstream for tomcat
COPY --from=builder /hdfstream-api/build/webapp/setenv.sh ${CATALINA_HOME}/bin/

# Copy configuration and startup scripts over
COPY --from=builder /hdfstream-api/webapp/src/main/docker/startup.sh /opt/hdfstream/
COPY --from=builder /hdfstream-api/webapp/src/main/docker/scan_directory.py /opt/hdfstream/

# Expose Tomcat’s default port
EXPOSE 8080

# Generate configuration and start the service.
# Assumes data files are mounted under /opt/hdfstream/data/.
CMD ["/opt/hdfstream/startup.sh"]

# This allows docker to check if the service is running
HEALTHCHECK --start-period=5s --interval=10s --timeout=5s --retries=5 \
  CMD curl -f http://localhost:8080/hdfstream/msgpack -o /dev/null || exit 1
