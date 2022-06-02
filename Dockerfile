FROM ubuntu:20.04

# Apt
RUN apt -y update
RUN apt install -y build-essential cmake wget
RUN echo "tzdata "debconf/frontend" select 8" | debconf-set-selections && \
    apt install -y tzdata && apt install -y libboost-all-dev

# Build liburing
WORKDIR /tmp
RUN wget https://github.com/axboe/liburing/archive/refs/tags/liburing-2.1.tar.gz
RUN tar -xvzf liburing-liburing-2.1.tar.gz
WORKDIR /tmp/liburing-liburing-2.1
RUN ./configure
RUN make -j`nproc`
RUN install

# Build server
WORKDIR /app
COPY . .
WORKDIR /app/build
RUN cmake .. && make -j`nproc`


WORKDIR /app
CMD ./build/io-uring-static-server

EXPOSE 80
VOLUME ["/var/www/html"]