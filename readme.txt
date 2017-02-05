Platform used: ubuntu 12.04 lts
Packages used: sqlite3
             Version: 3.7.9-2ubuntu1.1
             Depends: libc6 (>= 2.14), libreadline6 (>= 6.0), libsqlite3-0 (= 3.7.9-2ubuntu1.1)

To run(inside the dir files are located):
   1--> g++ -c Scheduler.cc scheduler_launcher.cc ThreadPool.cc Task.cc Results.cc  -std=c++11 -pthread -lsqlite3
   2--> g++ -o scheduler Scheduler.o scheduler_launcher.o ThreadPool.o Task.o Results.o  -std=c++11 -pthread -lsqlite3
   3--> ./scheduler

Classes:
   1-->ThreadPool : Implements a pool of threads available to Scheduler. Has a task queue which threads lookup for tasks
   2-->Scheduler : Maintains a Threadpool. Allows adding tasks for execution. Main scheduler runs in a thread separate
                   from the launcher.
   3-->Task : An encapsulation for the callable object, its start time and its frequency. Scheduler allows these to be scheduled.
   4-->Results : Class implementing DB interface and in-memory buffers for results of Tasks.

Main Program: scheduler_launcher.cc
            |
            |--> Launches a scheduler. Provides scheduler with two tasks i.e ping to 8.8.8.8 and reading virtual-memory usage. Also, schedules
                 two tasks to write results of aforementioned tasks into DB. Tests scheduler functions in various ways.
