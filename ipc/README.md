# Introduction
IPC - inter process communication is computer technology that provide connection between two or more process. The POSIX Linux standard provide us 3 main IPC: _shared memory_, _pipes_, and _messages_. You can read about linux syscall and realisation of this functionality in [man](https://man7.org/linux/man-pages/man2/ipc.2.html).  
## About test
My goal in this task to compare this IPCs in term of performance. So I just wrote simple program that share through IPC a file with size of 100 MB, and after that I checked MD5 hash.
## Results
Note: all results was checked on a virtual machine, so it can be a huge mistake in real performance. It would great if someone will check this code on real linux.

| IPC name      | Average time (s) | 
|---------------|------------------|
| Pipe          | 0.5              | 
| Shared memory | 4.1              | 
| Messages      | 8.2              | 

It seems that messages the slowest IPC, but this is can be inccorect argument, because I used semaphore for message synchronization. According to my computer science teacher it is unnecessary to use semaphore for messages.

