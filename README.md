## What is it
HTTP server for serving static files. Written in C++ using the `io_uring` API.

## Features

1. ring-per-thread, (`cpu_limit` option in the config, or at the option of server).
2. io_uring workerpool is shared between all rings (`share_ring_backend` option in the config).
3. Buffers for reading and writing are allocated at the beginning of work and reused during server operation. 
4. Buffers can be shared between kernel and servers, in this case there is no copying between kernel and user space (`register_buffers` option in the config, *it is not working correctly now, leave `0`*).
5. The ability to run in the `SQPOLL` mode. I/O in this mode is performed without system calls (`enable_sq_poll` option in the config).
6. Using DMA when sending files.
## Build and run

Cloning the repository and submodules:

```bash
git clone --recurse-submodules https://github.com/GeorgiyX/io-uring-static-server.git
```

Buid docker image:

```bash
docker build -t io-uring-static .
```

Run server:

```bash
docker run -p 80:80 -v <config_dir>:/etc/httpd.conf:ro -v <static_dir>:/var/www/html:ro --name io-uring-static -t io-uring-static
```

* `<config_dir>` - directory with configuration files
* `<static_dir>` - directory with static files

Run tests:

```bash
http-test-suite/httptest.py
```



## Performance

Server performance was compared with nginx (epoll) using the wrk tool. Below are the benchmark results (RPS) for both servers, with different CPU configurations. To get metrics, you can use script `benchmark.sh`.  The load is represented as a `GET` request for a file (98.6 KB). Load goes for 100 seconds in 8 threads for 100, 1000, 10000 connections. Tests were performed on AMD Ryzen 7 5800HS, 8 cores, 16 GB RAM, kernel 5.13.0.

**1 CPU test:**

* io-uring-static-server: [config](https://github.com/GeorgiyX/io-uring-static-server/blob/master/configs/io-uring-server-1-cpu.conf), [result](https://github.com/GeorgiyX/io-uring-static-server/blob/master/benchmarks/io-uring-static-server-1-cpu.txt).
* nginx: [config](https://github.com/GeorgiyX/io-uring-static-server/blob/master/configs/nginx-1-cpu.conf), [result](https://github.com/GeorgiyX/io-uring-static-server/blob/master/benchmarks/ngnix-1-cpu.txt).

| connections | io-uring-static-server | nginx | difference |
| ----------- | ---------------------- | ----- | ---------- |
| 100         | 33947                  | 8306  | x4.09      |
| 1K          | 33302                  | 8462  | x3.9       |
| 10K         | 33313                  | 8454  | x3.94      |

**8 CPU test:**

* io-uring-static-server: [config](https://github.com/GeorgiyX/io-uring-static-server/blob/master/configs/io-uring-server-8-cpu.conf), [result](https://github.com/GeorgiyX/io-uring-static-server/blob/master/benchmarks/io-uring-static-server-8-cpu.txt).
* nginx: [config](https://github.com/GeorgiyX/io-uring-static-server/blob/master/configs/nginx-8-cpu.conf), [result](https://github.com/GeorgiyX/io-uring-static-server/blob/master/benchmarks/ngnix-8-cpu.txt).

| connections | io-uring-static-server | nginx | difference |
| ----------- | ---------------------- | ----- | ---------- |
| 100         | 79438                  | 65904 | x1.2       |
| 1K          | 74167                  | 65492 | x1.13      |
| 10K         | 72126                  | 64977 | x1.11      |
