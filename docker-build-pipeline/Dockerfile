# Use the official maven/Java 8 image to create a build artifact.
# https://hub.docker.com/_/maven
FROM maven:3.8.6-openjdk-11-slim AS build

# Set the working directory in the image to /app
WORKDIR /app

# Copy the pom.xml file to our app directory
COPY pom.xml .

# Build all dependencies for offline use
RUN mvn dependency:go-offline -B

# Copy the rest of the project
COPY src src

# Package the application
RUN mvn package -DskipTests

# Use OpenJDK for base image.
# It's important to use OpenJDK 8u191 or above that has container support enabled.
# https://hub.docker.com/r/adoptopenjdk/openjdk8
# https://docs.docker.com/develop/develop-images/multistage-build/#use-multi-stage-builds
FROM openjdk:11

# Copy the jar to the production image from the builder stage.
COPY --from=build /app/target/*.war /app.war

# Exposing port 8080
EXPOSE 8080

# Run the application
ENTRYPOINT ["java","-jar","/app.war"]
