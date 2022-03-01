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
    money_paid_ = new bool[num_barbers_];   // check if barber gets paid after each haircut
    customer_in_chair_ = new int[num_barbers_];   // store customer id for barber
    cond_barber_sleeping_ = new pthread_cond_t[num_barbers_]; // dynamic allocate size 
    cond_customer_served_ = new pthread_cond_t[num_barbers_]; // dynamic allocate size 
    for (int i = 0; i < num_barbers_; i++) {
        pthread_cond_init(&cond_barber_sleeping_[i], NULL);
        pthread_cond_init(&cond_customer_served_[i], NULL);
        money_paid_[i] = false;
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


//-----------------------------------------------------------------------------
// operations of customers
//-----------------------------------------------------------------------------

bool Shop::visitShop(int id, int &barber_id) {
    pthread_mutex_lock(&mutex_);
    // when all barbers are working and there is no waiting chairs, customer leaves 
    if (num_in_service_ == num_barbers_ && waiting_chairs_.size() == max_waiting_cust_) {
        cout << "customer[" << id << "]: leaves the shop because of no available waiting chairs." << endl;
        ++cust_drops_;
        pthread_mutex_unlock(&mutex_);
        return false;
    }
    // if no barbers avaliable now
    if (sleeping_barbers_.empty()) {
        // if someone is waiting and exist empty chair
        if (!waiting_chairs_.empty() || waiting_chairs_.size() < max_waiting_cust_) {
            waiting_chairs_.push(id);   // add to the waiting queue
            cout << "customer[" << id << "]: takes a waiting chair [" << waiting_chairs_.size() 
                << "]. # waiting seats available = " << max_waiting_cust_ - waiting_chairs_.size() << endl;
            while (sleeping_barbers_.empty())
                pthread_cond_wait(&cond_customers_waiting_, &mutex_);  // wait to be called
            waiting_chairs_.pop();  // leave the queue
        }
        else {  // in case of someone sneaks in and no space for waiting
            cout << "customer[" << id << "]: leaves the shop because of no available waiting chairs." << endl;
            ++cust_drops_;
            pthread_mutex_unlock(&mutex_);
            return false;
        }
    }
    // customer is assigned to the first avaliable sleeping barber
    barber_id = sleeping_barbers_.front();  // assigned to barber
    sleeping_barbers_.pop();
    ++num_in_service_;  
    // customer sits in the service seat and shout out.
    customer_in_chair_[barber_id] = id;
    cout << "customer[" << id << "]: moves to barber[" << barber_id 
        << "]'s service chair  # waiting seats available = " << max_waiting_cust_ - waiting_chairs_.size() << endl;
    // customer wakes up barber
    pthread_cond_signal(&cond_barber_sleeping_[barber_id]);
    pthread_mutex_unlock(&mutex_); 
    return true; 
}

void Shop::leaveShop(int id, int barber_id) {
    pthread_mutex_lock( &mutex_ );
    // Wait for service to be completed
    cout << "customer[" << id << "]: wait for barber[" << barber_id << "] done the hair-cut" << endl;
    if (customer_in_chair_[barber_id] == id)
        pthread_cond_wait(&cond_customer_served_[barber_id], &mutex_);
    // Pay the barber and signal barber appropriately
    money_paid_[barber_id] = true;  // customer pay then leave
    cout << "customer[" << id << "]: says good-bye to the barber[" << barber_id << "]" << endl;
    pthread_cond_signal(&cond_barber_sleeping_[barber_id]); // signal barber I paid
    pthread_mutex_unlock(&mutex_);
}

//-----------------------------------------------------------------------------
// operations of barbers
//-----------------------------------------------------------------------------

void Shop::helloCustomer(int id) {
    pthread_mutex_lock(&mutex_);
    // barber push to the sleeping queue(ready queue)
    sleeping_barbers_.push(id);
    // Signal customer again in case 
    // pthread_cond_signal( &cond_customers_waiting_ );
    // If no customers than barber can sleep
    if (waiting_chairs_.empty() && customer_in_chair_[id] == 0 ) {
        print(0, id, "sleeps because of no customers.");
        pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
    }
    // check if the customer, sit down.
    if (customer_in_chair_[id] == 0) 
        pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
    // in_service_[id] = true;
    print(0, id, "starts a hair-cut service for customer[" + int2string( customer_in_chair_[id] ) + "]" );
    pthread_mutex_unlock( &mutex_ );
}

void Shop::byeCustomer(int id) 
{
    pthread_mutex_lock(&mutex_);
    // Hair Cut-Service is done so signal customer and wait for payment
    print(0, id, "says he's done with a hair-cut service for customer[" + int2string(customer_in_chair_[id]) + "]");
    money_paid_[id] = false;
    // signal customer cut finished, and ask a payment
    pthread_cond_signal(&cond_customer_served_[id]);
    if (!money_paid_[id]) 
        pthread_cond_wait(&cond_barber_sleeping_[id], &mutex_);
    --num_in_service_;
    //Signal to customer to get next one
    customer_in_chair_[id] = 0;
    print(0, id, "calls in another customer");
    pthread_cond_signal( &cond_customers_waiting_ );
    pthread_mutex_unlock( &mutex_ );  // unlock
}