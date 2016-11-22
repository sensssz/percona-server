#ifndef MY_TRACE_TOOL_H
#define MY_TRACE_TOOL_H

#include <fstream>
#include <list>
#include <vector>
#include <deque>
#include <pthread.h>
#include <time.h>
#include <unordered_map>
#include <cstdlib>
#include <string>

#define TRX_TYPES 6

/** This macro is used for tracing the running time of
    a function call which appears inside an if statement*/
#define TRACE_S_E(function_call, index) (TRACE_START()|(function_call)|TRACE_END(index))

#define var_mutex_enter() pthread_mutex_lock(&TraceTool::var_mutex)
#define var_mutex_exit() pthread_mutex_unlock(&TraceTool::var_mutex)
#define estimate_mutex_enter() pthread_mutex_lock(&TraceTool::last_second_mutex); pthread_rwlock_rdlock(&TraceTool::data_lock);
#define estimate_mutex_exit() pthread_mutex_unlock(&TraceTool::last_second_mutex); pthread_rwlock_unlock(&TraceTool::data_lock)

typedef unsigned long int ulint;
typedef unsigned int uint;

using std::ofstream;
using std::list;
using std::vector;
using std::endl;
using std::string;
using std::deque;

/** The global transaction id counter */
extern ulint transaction_id;

/********************************************************************//**
To break down the variance of a function, we need to trace the running
time of a function and the functions it calls. */

/********************************************************************//**
This function marks the start of a function call */
void TRACE_FUNCTION_START();

/********************************************************************//**
This function marks the end of a function call */
void TRACE_FUNCTION_END();

/********************************************************************//**
This function marks the start of a child function call. */
bool TRACE_START();

/********************************************************************//**
This function marks the end of a child function call. */
bool TRACE_END(
  int index);   /*!< Index of the child function call. */

/********************************************************************//**
Transaction types in TPCC workload. */
enum transaction_type
{
    NEW_ORDER, PAYMENT, ORDER_STATUS, DELIVERY, STOCK_LEVEL, NONE
};
typedef enum transaction_type transaction_type;

class TraceTool
{
private:
    static TraceTool *instance;             /*!< Instance for the Singleton pattern. */
    static pthread_mutex_t instance_mutex;  /*!< Mutex for protecting instance. */
    
    static timespec global_last_query;      /*!< Time when MySQL receives the most recent query. */
    static pthread_mutex_t last_query_mutex;/*!< Mutex for protecting global_last_query */
    
    static __thread timespec function_start;/*!< Time for the start of a function call. */
    static __thread timespec function_end;  /*!< Time for the end of a function call. */
    static __thread timespec call_start;    /*!< Time for the start of a child function call. */
    static __thread timespec call_end;      /*!< Time for the end of a child function call. */
    static __thread bool new_transaction;   /*!< True if we need to start a new transaction. */
    static __thread timespec trans_start;   /*!< Start time of the current transaction. */
    
    ofstream log_file;                      /*!< An log file for outputing debug messages. */
    
    vector<vector<long> > function_times;  /*!< Stores the running time of the child functions
                                                 and also transaction latency (the last one). */
    vector<ulint> transaction_start_times;  /*!< Stores the start time of transactions. */
    vector<transaction_type> transaction_types;/*!< Stores the transaction types of transactions. */
    
    TraceTool();
    TraceTool(TraceTool const&){};
public:
    static pthread_rwlock_t data_lock;      /*!< A read-write lock for protecting function_times. */
    static __thread ulint current_transaction_id;   /*!< Each thread can execute only one transaction at
                                                         a time. This is the ID of the current transactions. */
    
    static __thread int path_count;         /*!< Number of node in the function call path. Used for
                                                 tracing running time of functions. */
    
    static __thread bool is_commit;         /*!< True if the current transactions commits. */
    static __thread bool query_is_commit;   /*!< True if the current query is a commit query (Note that
                                                 these two doesn't have to be true at the same time). */
    static __thread bool commit_successful; /*!< True if the current transaction successfully commits. */
    static __thread transaction_type type;  /*!< Type of the current transaction. */
    static long num_trans[TRX_TYPES];       /*!< Number of successfully submitted transactions. */
    static double mean_latency[TRX_TYPES];  /*!< Mean of total wait time of successfully committed
                                             transactions*/
    static pthread_mutex_t var_mutex;
    
    vector<long> time_so_far;
    vector<long> trx_ids;
    
    ulint num_waits;
    ulint total_locks;
    
    /********************************************************************//**
    The Singleton pattern. Used for getting the instance of this class. */
    static TraceTool *get_instance();
    
    /********************************************************************//**
    Check if we should trace the running time of function calls. */
    static bool should_monitor();
    
    /********************************************************************//**
    Calcualte time interval in nanoseconds. */
    static long difftime(timespec start, timespec end);
    
    /********************************************************************//**
    Periododically checks if any query comes in in the last 5 second.
    If no then dump all logs to disk. */
    static void *check_write_log(void *);
    
    /********************************************************************//**
    Get the current time in nanosecond. */
    static timespec get_time();
    /********************************************************************//**
    Get the current tiem in microsecond. */
    static ulint now_micro();
    
    /********************************************************************//**
    Retruns the log file for outputing debug information. */
    ofstream &get_log()
    {
        return log_file;
    }

    /********************************************************************//**
    Sumbits the total wait time of a transaction. */
    void update_ctv(long latency);
    
    /********************************************************************//**
    Start a new query. This may also start a new transaction. */
    void start_new_query();
    /********************************************************************//**
    End a new query. This may also end the current transaction. */
    void end_query();
    /********************************************************************//**
    End the current transaction. */
    void end_transaction();
    
    /********************************************************************//**
    Analysis the current query to find out the transaction type. */
    void set_query(const char *query);
    /********************************************************************//**
    Dump data about function running time and latency to log file. */
    void write_latency(string dir);
    /********************************************************************//**
    Write necessary data to log files. */
    void write_log();
    
    /********************************************************************//**
    Record running time of a function. */
    void add_record(int function_index, long duration);
};

#endif
