# 01 - Setup Project


This project will be developed using only C. We will use autotools to allow do not depend on the current file system and, also, every step is recorded in a Dockerfile to ensure its reproductibilie.

To execute it as docker, you can use the following commands bellow. The first will build the image, the second will execute the system and the third one will allow you to interact with the build environment.

```bash
docker build . -t cmom
docker run --rm -it cmom
docker run --rm -it --entrypoint bash cmom
```

Now that you now how to execute, let me explain what we are running. 

We start with a clean Ubuntu, so the first steps installing every dependency we need to develop using C and Autotools. We can do it by using the command bellow.

```bash
sudo apt update
sudo apt install autoconf automake libtool make gcc
```

Autotools automatically generates the makefiles based on some files. 