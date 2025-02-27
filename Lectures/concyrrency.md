Intro to Concurrency

Look at threads.c for example code 

Concurrent is when there are multiple tasks that are able to progress at the same time
Parrerell process is programs running independently possibly at the same time
Concurrency is similar to parrerel but they need to combine at some point


Threads are lightweiht processes - subdivision of a process
- Every process has at least one thread - main thread 
Process - lots of information: 
 - open files
 - code segment 
 - data 
 - heap
 - stack 
 - register context
 - open communication channel
 - page table
 - ...
Thread - exectuion context 
 - registers
 - stack
 - everything else is implicitly shared within the process



Address Space
 Stack

 ... 

 ... 

 Heap x
 Data 
 Code