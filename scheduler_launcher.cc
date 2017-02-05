#include "Scheduler.h"
#include <algorithm>
#include <vector>
#include <mutex>
#include <memory>
#include <unistd.h>
#include <limits>
#include "Results.cc"
#include "sys/sysinfo.h"
#include <string.h>

float pingtask() {
  FILE* fp;
  int status;
  char res[500];
  memset(res, '\0', sizeof(res));
  fp = popen("ping 8.8.8.8 -c1 | grep rtt | awk -F '=' '{print $2}' | awk -F '/' '{print $2}'", "r");

  while (fgets(res, 500, fp) != NULL) {}

  pclose(fp);
  return atof(res);
}

VirtMem memtask(int& status) {

  struct sysinfo memInfo;

  if (sysinfo(&memInfo) == -1) {
    status = -1;
    return VirtMem();
  }
  uint64_t totalVirtualMem = memInfo.totalram;
  // Include swap memory too
  totalVirtualMem += memInfo.totalswap;
  totalVirtualMem *= memInfo.mem_unit;

  uint64_t virtualMemUsed = memInfo.totalram - memInfo.freeram;
  virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
  virtualMemUsed *= memInfo.mem_unit;

  return VirtMem(totalVirtualMem, virtualMemUsed);
  
}

int dbcallback(void *data, int argc, char **argv, char **azColName){
   int i;
   if (data) {
     dbstruct *d = (dbstruct *) data;
     d->min = std::string(argv[2]);
     d->max = std::string(argv[3]);
     d->avg = std::string(argv[4]);
   }
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int main() {

  // Create Scheduler
  Scheduler *sc = new Scheduler(10);

  // Add tasks to the scheduler's queue
  // 1. Ping Task.
  std::string sql = "CREATE TABLE IF NOT EXISTS PING("  \
          "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
          "VAL            REAL     NOT NULL," \
          "MIN              REAL     NOT NULL," \
          "MAX              REAL     NOT NULL," \
          "AVG              REAL     NOT NULL);";

  // Create a pointer to result struct for ping task
  std::shared_ptr<Results<float>> ping_result(new Results<float>("ping.db"));

  //1. Open conn to DB
  //2. Create ping table in DB
  if (ping_result->open_conn() == 0) {
    std::cout<<"Couldn't open connection with DB.Exit"<<std::endl;
    return 0;
  }

  if (ping_result->create_table(sql) == 0) {
    std::cout<<"Couldn't create ping table in DB.Exit"<<std::endl;
    return 0;
  }

  // Add pingtask to scheduler
  sc->add_task(Task(
             [ping_result](){
                       // Execute task and get result
                       float n = pingtask();
                       if (n == 0)
                         // Ping didn't succeed.
                         return;

                       // Store the result in memory
                       {
                         std::unique_lock<std::mutex> lk(ping_result->mu);
                         ping_result->res.push_back(n);
                       }
                   }, steady_clock::now()+seconds(4), seconds(2), 1));

  // 2. Memory Task.
  // Create a pointer to result struct for memory task
  std::unique_ptr<Results<VirtMem>> mem_result(new Results<VirtMem>("memory.db"));
  sql = "CREATE TABLE IF NOT EXISTS MEMORY("  \
        "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
        "VAL            INTEGER     NOT NULL," \
        "MIN            INTEGER     NOT NULL," \
        "MAX            INTEGER     NOT NULL," \
        "AVG            INTEGER     NOT NULL," \
        "TOTAL          INTEGER     NOT NULL);";

  //1. Open conn to DB
  //2. Create ping table in DB
  if (mem_result->open_conn() == 0) {
    std::cout<<"Couldn't open connection with DB.Exit"<<std::endl;
    return 0;
  }

  if (mem_result->create_table(sql) == 0) {
    std::cout<<"Couldn't create memory table in DB.Exit"<<std::endl;
    return 0;
  }

  // Add memorytask to scheduler
  sc->add_task(Task(
             [&mem_result](){
                       // Execute task and get result
                       int st;
                       VirtMem n = memtask(st);
                       if (st == -1)
                         // Task didn't succeed
                         return;
                       // Store the result in memory
                       {
                         std::unique_lock<std::mutex> lk(mem_result->mu);
                         mem_result->res.push_back(n);
                       }
                   }, steady_clock::now()+seconds(4), seconds(2), 2));

  // Launch two more tasks which will read ping and memory results
  // from memory buffers and write them to the respective tables in DB.

  // 3. Ping database writer.
  sc->add_task(Task(
             [ping_result](){
                       std::vector<float> data;
                       {
                         // Copy from results
                         std::unique_lock<std::mutex> lk(ping_result->mu);
                         if (ping_result->res.empty())
                           return;
                         data = std::vector<float>(ping_result->res.begin(), ping_result->res.end());
                         ping_result->res.clear();
                       }

                       // Store the result in DB
                       {
                         std::unique_lock<std::mutex> lk(ping_result->db_lock);
                         // Move contents of data to Database
                         std::string sql = "SELECT * FROM PING where ID=(SELECT MAX(ID) FROM PING)";
                         dbstruct *db = new dbstruct();
                         ping_result->execute_sql(dbcallback, sql, (void *)db);
                         float min = atof(db->min.c_str());
                         float max = atof(db->max.c_str());
                         auto avg = atof(db->avg.c_str());
                         for (auto it=data.begin(); it != data.end(); ++it) {
                           float val;
                           val = *it;
                           min = min == 0? *it:std::min(val,min);
                           max = max == 0? *it:std::max(val, max);
                           avg = avg == 0? *it:((val + avg)/2);

                           // write the record into DB via insert
                           sql = "INSERT INTO ping (VAL, MIN, MAX, AVG) "  \
                                 "VALUES (" + std::to_string(val) + ", " + std::to_string(min) +
                                 ", " + std::to_string(max) + ", " + std::to_string(avg) + " );";
                           ping_result->execute_sql(dbcallback, sql, NULL);
                         }
                       }
                   }, steady_clock::now()+seconds(4), seconds(8), 3));

  // 4. memory database writer.
  sc->add_task(Task(
             [&mem_result](){
                       std::vector<VirtMem> data;
                       {  
                         // Copy from results
                         std::unique_lock<std::mutex> lk(mem_result->mu);
                         if (mem_result->res.empty())
                           return;
                         data = std::vector<VirtMem>(mem_result->res.begin(), mem_result->res.end());
                         mem_result->res.clear();
                       }

                       // Store the result in DB
                       {
                         std::unique_lock<std::mutex> lk(mem_result->db_lock);
                         // Move contents of data to Database
                         std::string sql = "SELECT * FROM MEMORY where ID=(SELECT MAX(ID) FROM MEMORY)";
                         dbstruct *db = new dbstruct();
                         mem_result->execute_sql(dbcallback, sql, (void *)db);
                         uint64_t min = strtoull(db->min.c_str(), NULL, 0);
                         uint64_t max = strtoull(db->max.c_str(), NULL, 0);
                         uint64_t avg = strtoull(db->avg.c_str(), NULL, 0);
                         for (auto it=data.begin(); it != data.end(); ++it) {
                           VirtMem val;
                           val = *it;
                           min = min == 0? val.curr:std::min(val.curr, min);
                           max = max == 0? val.curr:std::max(val.curr, max);
                           avg = avg == 0? val.curr:((val.curr + avg)/2);

                           // write the record into DB via insert
                           sql = "INSERT INTO MEMORY (VAL, MIN, MAX, AVG, TOTAL) "  \
                                 "VALUES (" + std::to_string(val.curr) + ", " + std::to_string(min) +
                                 ", " + std::to_string(max) + ", " + std::to_string(avg) + ", " +
                                 std::to_string(val.total) + " );";
                           mem_result->execute_sql(dbcallback, sql, NULL);
                         }
                       }
                       mem_result->doshit();

                   }, steady_clock::now()+seconds(4), seconds(8), 4));

  // Alright, let's run the scheduler.
  sc->run_scheduler();
  
  // Add a task while scheduler is running
  sc->add_task(Task([](){std::cout<<"another task"<<std::endl;}, steady_clock::now()+seconds(2), seconds(5), 5));
  sc->run_scheduler(); // This shows an error since scheduler is running already.

  std::cout<<"Main thread sleeping for 30 seconds"<<std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(30000));
  std::cout<<"Waking from sleep and stopping scheduler"<<std::endl;
  sc->stop_scheduler();
  std::cout<<"Scheduler stopped. will start after 3 seconds"<<std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  sc->run_scheduler();
  std::cout<<"Unscheduling task no 5"<<std::endl;
  // Unschedule task with id=5
  sc->unschedule(5);
  std::this_thread::sleep_for(std::chrono::milliseconds(50000));
  sc->stop_scheduler();
  std::cout<<"Sample run of scheduler done. Now main program will stop scheduler," \
             "print DB contents, close DB connections and EXIT."<<std::endl;

  sql = "SELECT * FROM MEMORY";
  mem_result->execute_sql(dbcallback, sql, NULL);
  std::cout<<"<><><><><><><><><><<><><><><><><><><><><><><><><><><><>"<<std::endl<<std::endl;
  sql = "SELECT * FROM PING";
  ping_result->execute_sql(dbcallback, sql, NULL);
  ping_result->close_conn();
  mem_result->close_conn();
  return 0;
}
