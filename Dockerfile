# Use the official Tomcat base image
FROM tomcat:9

# Maintainer info (optional)
LABEL maintainer="j.c.helly@durham.ac.uk"

# Remove default webapps
#RUN rm -rf /usr/local/tomcat/webapps/*

# Copy your WAR file into Tomcat’s webapps directory
#COPY myapp.war /usr/local/tomcat/webapps/

# Expose Tomcat’s default port
EXPOSE 8080

# Start Tomcat
CMD ["catalina.sh", "run"]
