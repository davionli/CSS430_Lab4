#ifndef SHOP_ORG_H_
#define SHOP_ORG_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
using namespace std;

#define kDefaultNumChairs 3

class Shop {
public:
    Shop(int num_chairs) : max_waiting_cust_((num_chairs > 0 ) ? num_chairs : kDefaultNumChairs), customer_in_chair_(0),
      in_service_(false), money_paid_(false), cust_drops_(0)
   { 
      init(); 
   };
   Shop() : max_waiting_cust_(kDefaultNumChairs), customer_in_chair_(0), in_service_(false),
      money_paid_(false), cust_drops_(0)
   { 
      init(); 
   };

   bool visitShop(int id);   // return true only when a customer got a service
   void leaveShop(int id);
   void helloCustomer();
   void byeCustomer();
   int get_cust_drops() const;


private:
    const int max_waiting_cust_;              // the max number of threads that can wait
    int customer_in_chair_;
    bool in_service_;            
    bool money_paid_;
    queue<int> waiting_chairs_;  // includes the ids of all waiting threads
    int cust_drops_;

    const int barber = 0; // the id of the barber thread
  
    void init();
    string int2string(int i);
    void print(int person, string message);
};

#endif