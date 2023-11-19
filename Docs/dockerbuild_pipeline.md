**LIMITATIONS**

With this configuration you need either a custom jenkins image with docker preinstalled or a jenkins container with docker plugins installed which will install docker as a tool.

Also as it is essestially using docker inside docker we need to expose the docker socket to the container. At the time of execution it will use the outside (node) docker env to build the image and push it to the target registory and internal registory.

`Dockerfile for custom jenkins image`
```docker
FROM jenkins/jenkins
USER root
RUN apt-get update && apt-get install -y lsb-release
RUN apt-get update && apt-get install -y ca-certificates curl gnupg
RUN install -m 0755 -d /etc/apt/keyrings
RUN curl -fsSL https://download.docker.com/linux/debian/gpg |  gpg --dearmor -o /etc/apt/keyrings/docker.gpg
RUN chmod a+r /etc/apt/keyrings/docker.gpg
RUN echo \
  "deb [arch="$(dpkg --print-architecture)" signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/debian \
  "$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
   tee /etc/apt/sources.list.d/docker.list > /dev/null
RUN apt-get update
RUN apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

`Docker command to pass the socket to jenkins`
```
docker run -p 8080:8080 -p 50000:50000 -v /var/run/docker.sock:/var/run/docker.sock --restart=on-failure -d jenkins-docker
```


------------------
This script assumes that you have a Dockerfile in your project root directory that is set up to create a Docker image with your Spring Boot application.

This script will clean the workspace, pull the code from GitHub, compile the code using Maven, package the code into a ".war" file, build a Docker image, push the Docker image to Docker Hub, and finally create a Docker container from the image.
------------------

This Dockerfile starts with a Maven and Java 11 base image, sets the working directory to `/app`, copies the `pom.xml` file to the app directory, downloads all dependencies, copies the rest of the project, packages the application into a .war file, starts a new stage with an OpenJDK 11 base image, copies the .war file from the build stage to the production image, and finally runs the application.

This Dockerfile does the following:
1. It starts with a Maven and Java 11 base image and sets the working directory to `/app`.
2. It then copies the `pom.xml` file to the app directory and downloads all dependencies.
3. It copies the rest of the project and packages the application, skipping the tests.
4. It then starts a new stage with an Adopt OpenJDK base image, copies the war file from the build stage to the production image, and finally runs the application.

Please replace the `pom.xml` and `src` with your actual `pom.xml` and source code directory. Also, replace `*.war` with your actual war file name.
--------------------------------

The Dockerfile should be placed in the root directory of your GitHub repository. The Jenkins pipeline script I provided earlier will automatically use this Dockerfile when it executes the `docker build` command.

Here's how it works:
1. The `git 'https://github.com/your-github-repo'` command in the 'Checkout' stage pulls your code (along with the Dockerfile) from your GitHub repository.
2. The `sh 'mvn package'` command in the 'Package' stage compiles your code and packages it into a .war file as specified in your `pom.xml`.
3. The `sh 'docker build -t your-dockerhub-username/your-docker-image-name .'` command in the 'Build Docker image' stage builds a Docker image using the Dockerfile in your project root directory. It names the image using your Docker Hub username and the image name you provide.
4. The `sh 'docker push your-dockerhub-username/your-docker-image-name'` command in the 'Push Docker image' stage pushes the Docker image to your Docker Hub account.
5. The `sh 'docker run -d --name my-container your-dockerhub-username/your-docker-image-name'` command in the 'Create Docker container' stage creates a new Docker container from the image.

Please replace `your-github-repo`, `your-dockerhub-username`, `your-dockerhub-password`, and `your-docker-image-name` with your actual GitHub repository URL, Docker Hub username, Docker Hub password, and Docker image name respectively.

Please note that this is a basic example and might need to be adjusted based on your specific use case and environment. Always make sure to secure your credentials and consider using Jenkins credentials binding for sensitive data like your Docker Hub password. Also, please ensure that your Jenkins instance has the necessary permissions to access your GitHub repository and Docker Hub.
-------------------------

This YAML file (Dock-Multipod-deploymeny.yaml) creates two services named `jenkins-docker` and `jenkins-blueocean` using the `docker:dind` and `myjenkins-blueocean:2.426.1-1` images respectively. It also sets the `--storage-driver overlay2` command for `jenkins-docker`, runs the container in privileged mode, and connects it to the `jenkins` network. The `DOCKER_TLS_CERTDIR` environment variable is set to `/certs` for `jenkins-docker`, and two volumes `jenkins-docker-certs` and `jenkins-data` are mounted at `/certs/client` and `/var/jenkins_home` respectively inside the container. The container's port `2376` is published to the host's port `2376`. The `jenkins` network is assumed to be a network, and the volumes `jenkins-docker-certs` and `jenkins-data` are created if they do not exist.

For `jenkins-blueocean`, it is connected to the `jenkins` network and the `DOCKER_HOST`, `DOCKER_CERT_PATH`, and `DOCKER_TLS_VERIFY` environment variables are set. The container's ports `8080` and `50000` are published to the host's ports `8080` and `50000` respectively. The `jenkins-data` volume is mounted at `/var/jenkins_home` and the `jenkins-docker-certs` volume is mounted at `/certs/client` in read-only mode inside the container. The container is configured to restart on failure.