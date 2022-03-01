#include "Shop.h"

Shop::~Shop() {
    delete[] cond_barber_sleeping_; // delete all cond_barber
    delete[] money_paid_;
    delete[] customer_in_chair_;
}
void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);
   pthread_cond_init(&cond_customers_waiting_, NULL); 
   money_paid_ = new bool[num_barbers_];
   in_service_ = new bool[num_barbers_];
   customer_in_chair_ = new int[num_barbers_];
   cond_barber_sleeping_ = new pthread_cond_t[num_barbers_]; // dynamic allocate size 
   cond_customer_served_ = new pthread_cond_t[num_barbers_]; // dynamic allocate size 
   for (int i = 0; i < num_barbers_; i++) {
     pthread_cond_init(&cond_barber_sleeping_[i], NULL);
     pthread_cond_init(&cond_customer_served_[i], NULL);
     money_paid_[i] = false;
     in_service_[i] = false;
     customer_in_chair_[i] = 0;
   }
}

string Shop::int2string(int i) 
{
   stringstream out;
   out << i;
   return out.str( );
}

void Shop::print(bool who, int person, string message) // when who == true print customer's info, else barber's info
{
   cout << ((who) ? "customer[" : "barber  [" ) << person << "]: " << message << endl;
}

int Shop::get_cust_drops() const
{
    return cust_drops_;
}

void Shop::printQueue(queue<int> q) {
    cout << "queue = ";
    while (!q.empty()) {
        cout << q.front() << " ";
        q.pop();
    }
    cout << endl;
}

//-----------------------------------------------------------------------------
// operations of customers
//-----------------------------------------------------------------------------

bool Shop::visitShop(int id, int &barber_id) 
{
   pthread_mutex_lock(&mutex_);
   
   // If all chairs are full then leave shop
   if (num_in_service_ == num_barbers_ && waiting_chairs_.size() == max_waiting_cust_) 
   {
      print(1, id, "leaves the shop because of no available waiting chairs.");
      ++cust_drops_;
      pthread_mutex_unlock(&mutex_);
      return false;
   }
   
   // If someone is being served or transitioning waiting to service chair
   // then take a chair and wait for service
   if (sleeping_barbers_.empty() || !waiting_chairs_.empty() || num_in_service_ == num_barbers_) 
   {
      waiting_chairs_.push(id);
    //   print(1, id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));
      cout << "customer[" << id << "] takes a waiting chair [" << waiting_chairs_.size() 
        << "]. # waiting seats available = " << max_waiting_cust_ - waiting_chairs_.size() << endl;
      while (num_in_service_ == num_barbers_)
        pthread_cond_wait(&cond_customers_waiting_, &mutex_);
      waiting_chairs_.pop();
   }

   if (sleeping_barbers_.empty())
     pthread_cond_wait(&cond_customers_waiting_, &mutex_);
   ++num_in_service_;  
//    printQueue(sleeping_barbers_);
   barber_id = sleeping_barbers_.front();
   
//    print(1, id, "moves to the service chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));
   cout << "customer[" << id << "] moves to barber[" << barber_id << "]'s service chair " << endl;
   customer_in_chair_[barber_id] = id;
   in_service_[barber_id] = true;
   
   // wake up 1st avaliable barber just in case if he is sleeping
//    cout << "customer[" << id << "] wakes up barber[" << barber_id << "]" << endl;
   sleeping_barbers_.pop();
   pthread_cond_signal(&cond_barber_sleeping_[barber_id]);
   
   pthread_mutex_unlock(&mutex_); 
   return true;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock( &mutex_ );

   // Wait for service to be completed
//    print(1, id, "wait for the hair-cut to be done");
   cout << "customer[" << id << "] wait for barber[" << barber_id << "] done the hair-cut" << endl;
   while (in_service_[barber_id] == true)
   {
      pthread_cond_wait(&cond_customer_served_[barber_id], &mutex_);
   }
   // Pay the barber and signal barber appropriately
   money_paid_[barber_id] = true;
   cout << "customer[" << id << "] paid. " << endl;
   pthread_cond_signal(&cond_barber_sleeping_[barber_id]);
//    print(1, id, "says good-bye to the barber" );
   cout << "customer[" << id << "] says good-bye to the barber[" << barber_id << "]" << endl;
   pthread_mutex_unlock(&mutex_);
}

//-----------------------------------------------------------------------------
// operations of barbers
//-----------------------------------------------------------------------------

void Shop::helloCustomer(int id) 
{
   pthread_mutex_lock(&mutex_);

   sleeping_barbers_.push(id);
   // If no customers than barber can sleep
   if (waiting_chairs_.empty() && customer_in_chair_[id] == 0 ) 
   {
      print(0, id, "sleeps because of no customers.");
    //   sleeping_barbers_.push(id);
    //   printQueue(sleeping_barbers_);
      pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
    //   sleeping_barbers_.pop();
   }

   if (customer_in_chair_[id] == 0)               // check if the customer, sit down.
   {
       pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
   }
   print(0, id, "starts a hair-cut service for " + int2string( customer_in_chair_[id] ) );
   pthread_mutex_unlock( &mutex_ );
}

void Shop::byeCustomer(int id) 
{
  pthread_mutex_lock(&mutex_);
  // Hair Cut-Service is done so signal customer and wait for payment
  in_service_[id] = false;
  print(0, id, "says he's done with a hair-cut service for " + int2string(customer_in_chair_[id]));
  money_paid_[id] = false;
  pthread_cond_signal(&cond_customer_served_[id]);
  while (money_paid_ == false)
  {
      pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
  }
  --num_in_service_;
  //Signal to customer to get next one
  customer_in_chair_[id] = 0;
  print(0, id, "calls in another customer");
  pthread_cond_signal( &cond_customers_waiting_ );

  pthread_mutex_unlock( &mutex_ );  // unlock
}
