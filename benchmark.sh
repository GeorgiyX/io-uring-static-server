#!/bin/bash

NGNIX_COMMAND=nginx
NGINX_CONFIG_1_CPU=$(realpath data/nginx-1-cpu.conf)
NGINX_CONFIG_8_CPU=$(realpath data/nginx-8-cpu.conf)

IO_URING_COMMAND=./build/io-uring-static-server
IO_URING_CONFIG_1_CPU=data/io-uring-server-1-cpu.conf
IO_URING_CONFIG_8_CPU=data/io-uring-server-8-cpu.conf

BENCHMARK_DIR=benchmark
THREAD_COUNT=8
WRK_COMMAND=/opt/bin/wrk
URL=http://localhost:80/httptest/splash.css

mkdir -p $BENCHMARK_DIR > /dev/null

function benchmark() {
      local COMMAND="$1"
      local THREADS="$2"
      local CONNECTIONS="$3"
      local FILE_PATH="$BENCHMARK_DIR/$4"

      echo -e "test $COMMAND at $THREADS threads and $CONNECTIONS connections"
      touch $FILE_PATH
      $COMMAND & sleep 2 && $WRK_COMMAND -c$CONNECTIONS -t$THREADS -d10s $URL >> $FILE_PATH
      sudo fuser -k 80/tcp > /dev/null
      echo -e "\n" >> $FILE_PATH
}

function make_benchmark() {
  local COMMAND="$1"
  local FILE="$2"
  CONNECTIONS=(100 1000 10000)
  for CONNECTION in ${CONNECTIONS[*]}; do
    benchmark "$COMMAND" $THREAD_COUNT $CONNECTION $FILE
  done
}

make_benchmark "sudo $IO_URING_COMMAND $IO_URING_CONFIG_1_CPU" "io-uring-static-server-1-cpu.txt"
make_benchmark "sudo $IO_URING_COMMAND $IO_URING_CONFIG_8_CPU" "io-uring-static-server-8-cpu.txt"
make_benchmark "sudo $NGNIX_COMMAND -c $NGINX_CONFIG_1_CPU" "ngnix-1-cpu.txt"
make_benchmark "sudo $NGNIX_COMMAND -c $NGINX_CONFIG_8_CPU" "ngnix-8-cpu.txt"
