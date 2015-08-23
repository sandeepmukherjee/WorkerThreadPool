/**
 @mainpage

  The WorkerThreadPool (WTP) is a library that allows implementors to define Work Items that get run concurrently by a fixed set of threads.

  Current status: Version 1.0 (tag v1.0)

  Callers add WTP::WorkItem objects to queues which are maintained internally by the WTP. When a worker thread is free, it looks in its list of queues, selects one, removes the next WorkItem from the queue, and invokes WorkItem's run() method. Every WorkerThreadPool comes with exactly one "concurrent" queue. WorkItem objects added to this queue may execute concurrently. Callers can create additional queues, but all additional queues are "sequential", meaning the next WorkItem does not get executed till the current one has completed.

  WorkItems can create SubItems, which execute concurrently. The parent WorkItem waits till all SubItems are complete.

  To use the WTP, implementors need to:
 - Instantiate a WTP::WorkerThreadPool object.
 - [optional] Instantiate a WTP::Freelist and populate with pre-allocated
   WTP::WorkItem objects.
 - Start all threads using the WTP::WorkerThreadPool::startThreads() function.
 - [optional] Add one or more sequential queues.
 - Instantiate a WorkItem and add to a queue. Repeat as needed.
 - [optional] Spawn SubItems from within WorkItems in a sequential queue.
 - When done, call waitEmpty().
 - Call shutDown() to shut down the threads.
 - Deallocate WorkerThreadPool object.
 - If using freelists, deallocate them.

  The code is extensively documented in Doxygen format, and all efforts are made to keep the comments coherent and in sync with the code. Inaccurate or missing documentation should be reported as bugs. Run "make doc" to generate Doxygen docs.

  This library is well-suited for applications that can divide the total work into smaller, well-defined independent chunks. It may not be appropriate for applications that require a lot of thread interaction or deal with a lot of shared data. In particular, it is illegal for WorkItems to reference other WorkItems in running state.

  The subdirectory "tests" contain test programs that get compiled by default. The "applications" subdirectory has some sample apps. These do not build by default and may not compile on all platforms. To build them, cd to the directory and type "make".

  Please see LICENSE.txt for licensing information. 

*/
