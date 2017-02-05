#include "Task.h"

Task::Task(std::function<void()>&& tsk, steady_clock::time_point start, steady_clock::duration rep_after, int tid) 
     :task(std::move(tsk)), start_time(start), repeat_after(rep_after), task_id(tid) {
}

bool Task::operator<(const Task& other) const {
  return start_time < other.start_time;
}
