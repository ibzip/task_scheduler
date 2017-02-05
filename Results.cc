#include <vector>
#include <mutex>
#include <sqlite3.h>
#include <iostream>

template <typename T>
class Results {
  public:
  std::vector<T> res;
  std::mutex mu;
  std::mutex db_lock;
  std::string db_name;
  sqlite3 *db;
  Results() {
  }
  Results<T>(std::string db):db_name(db) {
  }

  int open_conn() {
    int  rc;
    /* Open database */
    rc = sqlite3_open(db_name.c_str(), &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
    }else{
      fprintf(stdout, "Opened database successfully\n");
    }
  }

  int create_table(std::string table_create_stmt) {

    const char *sql;
    sql = table_create_stmt.c_str();
    int rc;
    char *zErrMsg = 0;
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return 0;
    }else{
      fprintf(stdout, "Table created successfully\n");
    }
  }


  int execute_sql(int (*f)(void *, int, char**, char**), std::string sql, void * something) {
    char * zErrMsg;
    const char* data = "Callback function called";
    int  rc = sqlite3_exec(db, sql.c_str(), f, something, &zErrMsg);
    if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
  }
  void doshit() {
    std::cout<<"doing shit"<<std::endl;
    while(1) {}
  }


  void close_conn() {
    sqlite3_close(db);
  }
};

struct VirtMem {
  uint64_t total;
  uint64_t curr;
  VirtMem(){}
  VirtMem(uint64_t t, uint64_t cu) {
    total = t;
    curr = cu;
  }
};

struct dbstruct {
  std::string min;
  std::string max;
  std::string avg;
};


