#include "Scheduler.h"
#include <algorithm>

Scheduler::Scheduler(int threads):tpool(threads),stop(false) { 
}

Scheduler::~Scheduler() {

}

void Scheduler::add_task(Task&& task) {
  // Let's acquire the lock on threadpool's task queue
  std::unique_lock<std::mutex> lock(tpool.queue_mutex);  
  to_be_scheduled.push_back(std::move(task));
}

void Scheduler::run_scheduler() {
  if (running) {
    std::cout<<"Scheduler already running"<<std::endl;
    return;
  }
  running = true;
  stop = false;
  // Start scheduler thread loop to schedule tasks
  auto t = std::thread([this](){
    while(1) {
      {
      std::unique_lock<std::mutex> lk(this->stop_mutex);
      if (this->stop) { 
        running = false;
        std::cout<<"Time to Bail. Stopping scheduler"<<std::endl;
        return;
      }
      }
      //Let's acquire the lock on threadpool's task queue
      std::unique_lock<std::mutex> lock(tpool.queue_mutex);

      // Get task with nearest execution
      auto it = std::min_element(to_be_scheduled.begin(), to_be_scheduled.end());

      // Wait till it's time to execute the task
      if (condition.wait_until(lock, it->start_time) == std::cv_status::timeout) {
        // Fill tpool task queue.
        tpool.tasks.push_back(it->task);
        // Repeat
        it->start_time += it->repeat_after;
      }
      // Notify a thread
      tpool.condition.notify_one();
    }
  });
  t.detach();
}

void Scheduler::stop_scheduler() {
  {
  std::unique_lock<std::mutex> lock(tpool.queue_mutex);  
  stop = true;
  }
}

void Scheduler::clear_task_queue() {
  std::unique_lock<std::mutex> lock(tpool.queue_mutex);
  to_be_scheduled.clear();
}

void Scheduler::unschedule(int task_id) {
  std::unique_lock<std::mutex> lock(tpool.queue_mutex);
  for (auto it = to_be_scheduled.begin(); it != to_be_scheduled.end(); ++it) {
    if (it->task_id == task_id) {
      to_be_scheduled.erase(it);
      break;
    }
  }
}














