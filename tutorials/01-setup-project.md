# 01 – Setup Project

As a first step in my challenge to build a Message-Oriented Middleware using C, I will set up my development environment. C is a language that usually depends on the current filesystem. We depend on installed libraries, versions of the kernel, and tools. Compiling a program is usually hard because we need a set of dependencies and their respective versions.

To address this challenge, we will use [Automake](https://www.gnu.org/savannah-checkouts/gnu/automake/manual/1.10/html_node/index.html) to create the Makefile for this project. Automake is a GNU tool and can be installed on any Linux distribution. If you use Windows, you can install it via [MinGW](https://www.mingw-w64.org/). With Autotools, we can define a config file and describe how our program is structured, and all Makefiles are automatically generated.

Building an environment in Linux can be tricky because to test it we need a "clean Linux" installation. This "clean Linux" can be reproduced by using Docker. So, if you want to test this build, execute the lines below.

```bash
docker build . -t cmom                       ## Create the Docker image
docker run --rm -it cmom                     ## Execute the system
docker run --rm -it --entrypoint bash cmom   ## Access the build environment.
```

## Setup environment

Now, we need to list all dependencies and install them. For this project we will use `autoconf`, `automake`, and `libtool`, which will enable Autotools. `make` is also an Autotools dependency, but if you want to build your Makefiles manually, you can install it. For simple projects it's a good idea to manually write Makefiles. Maintaining a Makefile is not hard and you can mix languages and compilers, which can be a good solution for complex projects.

Run this step, or adapt it to your Linux distribution.

```bash
sudo apt update
sudo apt install autoconf automake libtool make gcc
```

## Setup Automake project

To set up Automake, we need to configure `configure.ac` and all `Makefile.am` files. The first one will inform Automake what your project is, which dependencies it should have, and which flags to use to build it.

```
AC_INIT([cmom], [1.0], [victor.perticarrari@gmail.com])
AC_CONFIG_SRCDIR([src/cmom.c])
AC_CONFIG_HEADERS([config.h])

# Use auxiliary files in a 'build-aux' directory (optional but tidy)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Check for a C compiler
AC_PROG_CC

# Checks for header files (standard C headers)
AC_HEADER_STDC

# Checks for library functions (e.g., malloc, strdup)
AC_CHECK_FUNCS([strdup])

# Generate Makefiles
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
```

The second one will show which files to compile, what the deployable artifact should be, and which flags we should use. For example, the file below says we should build `cmom` by compiling `cmom.c` using the `include` folder with the include flag `-I`.

```
bin_PROGRAMS = cmom
cmom_SOURCES = cmom.c
cmom_CFLAGS = -I$(top_srcdir)/include
```

## Building and running

Now that Automake is configured, let's build it. It's simple and all projects follow the same pattern. You have to create the configure script by running `autoreconf --install` and then execute `./configure`. This is only needed once. After that you can change whatever you want and the command `make` will regenerate everything.

So execute `make` and voilà! You have your program built as `./src/cmom`. It only prints "Hello World!". It's fine! That's all we need for now.