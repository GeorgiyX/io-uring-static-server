FROM ubuntu:20.04

# SetUp
RUN apt -y update
RUN apt install -y build-essential
RUN echo "tzdata "debconf/frontend" select 8" | debconf-set-selections && \
    apt install -y tzdata && apt install -y libboost-all-dev
RUN apt install -y cmake

# Build
WORKDIR /app
COPY . .
WORKDIR /app/build
RUN cmake .. && make -j`nproc`


WORKDIR /app
CMD ./build/io-uring-static-server

EXPOSE 80
VOLUME ["/var/www/html"]