FROM ubuntu:22.04

RUN mkdir /app
WORKDIR /app
ADD . /app
## Install dependencies
RUN apt update
RUN apt install -y autoconf automake libtool make gcc check liblog4c-dev

RUN autoreconf --install
RUN ./configure
RUN make

ENTRYPOINT ["/app/src/cmom"]
