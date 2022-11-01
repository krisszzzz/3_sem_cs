# Introduction
IPC - inter process communication is computer technology that provides a connection between two or more processes. The POSIX Linux standard provides us 3 main IPC: _shared memory_, _pipes_, and _messages_. You can read about Linux syscall and realisation of this functionality in [man](https://man7.org/linux/man-pages/man2/ipc.2.html).  
## About test
My goal of this task to compare this IPCs in term of performance. So I just wrote a simple program that shares through IPC a file with a size of 100 MB, and after that I checked MD5 hash.
## Results
Note: all results were checked on a virtual machine, so it can be a huge mistake in real performance. It would great if someone will check this code on real Linux.
| IPC name      | Average time (s) | 
|---------------|------------------|
| Pipe          | 0.5              | 
| Shared memory | 4.1              | 
| Messages      | 8.2              | 

It seems that the messages the slowest IPC, but this is can be incorrect argument, because I used a semaphore for message synchronization. According to my computer science teacher it is unnecessary to use semaphore for messages.

