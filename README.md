## Что это 
HTTP сервер для раздачи статики. Написан на C++ с использованием API `io_uring`.

## Особенности работы

1. ring-per-thread, (`cpu_limit` опция в конфиге, либо на усмотрение сервера).
1. worker-pool разделяется между всеми кольцами (`share_ring_backend` опция в конфиге).
2. Буферы под чтение и запись аллоцируются в начале работы и переиспользуются в течении работы сервера. 
2. Буферы могут быть пошарены между яром и серверов, в таком случае не происходит копирование между яром и user space (`register_buffers` опция в конфиге).
3. Возможность запуска в режиме `SQPOLL` . I/O в таком режиме осуществляется без системных вызовов (`enable_sq_poll` опция в конфиге).
3. Использование DMA при отправке файлов.
## Сборка и запуск

Клонируем репозиторий и подмодули:

```bash
git clone --recurse-submodules https://github.com/GeorgiyX/io-uring-static-server.git
```


```bash

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND                                                                                  
  11998 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.09 io-uring-static                                                                          
  11999 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:03.84 iou-sqp-11998                                                                            
  12000 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12001 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12002 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12019 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 iou-wrk-11999                                                                            
  12020 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 iou-wrk-11999                                                                            
  12021 georgiy   20   0  237240   1596   1420 S   0,0   0,0   0:00.00 iou-wrk-11999
```

```bash
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND                                                                                  
  12635 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.10 io-uring-static                                                                          
  12636 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:01.79 iou-sqp-12635                                                                            
  12637 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.50 iou-sqp-12635                                                                            
  12638 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12639 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.50 iou-sqp-12635                                                                            
  12640 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12641 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.51 iou-sqp-12635                                                                            
  12642 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 io-uring-static                                                                          
  12650 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 iou-wrk-12641                                                                            
  12651 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 iou-wrk-12639                                                                            
  12652 georgiy   20   0  237240   1648   1472 S   0,0   0,0   0:00.00 iou-wrk-12637
```

