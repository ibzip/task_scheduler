#include "ThreadPool.h"
#include "Task.h"
#include <iostream>
class Scheduler {
  private:
    ThreadPool tpool;
    std::vector<Task> to_be_scheduled;
    std::condition_variable condition;
    std::mutex stop_mutex;
    bool stop;
    bool running;
  public:
    Scheduler(int threads);
    ~Scheduler();
    void add_task(Task&& task);
    void run_scheduler();
    void stop_scheduler();
    void clear_task_queue();
    void unschedule(int task_id);
};
