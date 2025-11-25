# Use the official Tomcat base image
FROM tomcat:9

# Maintainer info (optional)
LABEL maintainer="j.c.helly@durham.ac.uk"

# Build msgpack-c
RUN git clone https://github.com/msgpack/msgpack-c.git /tmp/msgpack-c \
    && cd /tmp/msgpack-c \
    && git checkout c-6.1.0 \
    && cmake . -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/msgpack-c

# Remove default webapps
RUN rm -rf /usr/local/tomcat/webapps/*

# Copy your WAR file into Tomcat’s webapps directory
#COPY myapp.war /usr/local/tomcat/webapps/

# Expose Tomcat’s default port
EXPOSE 8080

# Start Tomcat
CMD ["catalina.sh", "run"]
