FROM ubuntu:20.04

# Apt
RUN apt -y update
RUN apt install -y build-essential cmake wget git
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

# Build CDS library
WORKDIR /tmp
RUN git clone https://github.com/Microsoft/vcpkg.git
WORKDIR /tmp/vcpkg
RUN ./bootstrap-vcpkg.sh
RUN ./vcpkg integrate install
RUN vcpkg install libcds
RUN cp -r installed/x64-linux/include/cds /usr/include
RUN cp buildtrees/libcds/x64-linux-rel/bin/* /usr/lib

# Build server
WORKDIR /app
COPY . .
WORKDIR /app/build
RUN cmake .. && make -j`nproc`


WORKDIR /app
CMD ./build/io-uring-static-server

EXPOSE 80
VOLUME ["/var/www/html"]