#ifndef SHOP
#define SHOP
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
using namespace std;

#define kDefaultNumChairs 0

class Shop {
public:
    Shop(int num_barbers, int num_chairs) : num_barbers_((num_barbers > 0) ? num_barbers : 1),
     max_waiting_cust_(num_chairs), num_in_service_(0), cust_drops_(0)
   { 
      init(); 
   };
   Shop() : num_barbers_(1), max_waiting_cust_(kDefaultNumChairs), num_in_service_(0), 
     cust_drops_(0)
   { 
      init(); 
   };
   ~Shop();

   bool visitShop(int id, int &barber_id);   // return true only when a customer got a service
   void leaveShop(int id, int barber_id);
   void helloCustomer(int);
   void byeCustomer(int);
   int get_cust_drops() const;


private:
    const int max_waiting_cust_;              // the max number of threads that can wait
    int num_barbers_;
    int *customer_in_chair_;
    int num_in_service_;         
    bool *money_paid_;
    queue<int> waiting_chairs_;  // includes the ids of all waiting threads
    queue<int> sleeping_barbers_; // includes the ids of all sleeping barbers
    int cust_drops_;

    // Mutexes and condition variables to coordinate threads
    // mutex_ is used in conjuction with all conditional variables
    pthread_mutex_t mutex_;
    pthread_cond_t  cond_customers_waiting_;
    pthread_cond_t  *cond_customer_served_;
    pthread_cond_t  *cond_barber_sleeping_;
    void init();
    string int2string(int i);
    void print(bool who, int person, string message);
};

#endif