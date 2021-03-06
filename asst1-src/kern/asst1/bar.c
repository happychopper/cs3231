#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* Declare any globals you need here (e.g. locks, etc...) */

// the siple queue for order
struct barorder *order_queue[NCUSTOMERS];
int order_hi, order_lo,drink_hi, drink_lo;

// lock for controlling changing queue
struct lock *order_lock;
// semaphore for consumer/ producer model of order list
struct semaphore *full_sem;
struct semaphore *empty_sem;

// lock array for custumer to wait for bartender to mix the drink
struct semaphore *drink_sem[NCUSTOMERS];

// lock for bartender to require bottle
struct lock *bottle_lock[NBOTTLES];




/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */

void order_drink(struct barorder *order)
{
	// kprintf("Customer start ordering ..\n");

    // wait if the order list is already full
    P(full_sem);

    // CRITICAL REGION:: changing and reading order list
    lock_acquire(order_lock);

    // push this order to the queue
    order_queue[order_hi] =order;

    // assign this drink to correspond bartender
    // by dequeue the drenk_sem
    order->wait_drink = drink_sem[drink_lo];
    // add up the drink lo
    drink_lo = (drink_lo + 1) % NCUSTOMERS; 
    
    // increament the queue pointer
    order_hi  = (order_hi + 1 )% NCUSTOMERS;

    // CRITICAL REGION END::
    lock_release(order_lock);

    // wake up any sleeping bartender to do this drink
    V(empty_sem);
    
    // waiting that bartender to finish the drink
    P(order->wait_drink);
}



/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */

struct barorder *take_order(void)
{
    struct barorder *ret = NULL;


    // Sleep until customer order a drink 
    P(empty_sem);

    // CRITICAL REGION:: Changing and reading order list
    lock_acquire(order_lock);

    // dequeue the order queue
    ret = order_queue[order_lo];
    order_lo = (order_lo +1) % NCUSTOMERS;

    // CRITICAL REGION END::
    lock_release(order_lock);
    // Some bartender can take order
    V(full_sem);


    return ret;
}


/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */

void fill_order(struct barorder *order)
{

    /* add any sync primitives you need to ensure mutual exclusion
        holds as described */
    
    // sort the request order that meet the require of convension 
    // of takeing out bottle by same order (ascending order)
    sort_requested_bottles(order);

    // take the bottle form the carbinet
    take_bottles(order);

    /* the call to mix must remain */
    mix(order);

    // return the bottle to the carbinet
    return_bottles(order);

}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)
{

    // CRITICAL REGION:: Changing the counter of drink queue
    lock_acquire(order_lock);

    // push drink queue by return the sem to drink_sem
    drink_sem[drink_hi] = order->wait_drink;
    
    // add up the counter of the drink queue
    drink_hi = (drink_hi + 1) % NCUSTOMERS; 

    // wake up customer to enjoy the drink 
    // and this semaphore might be used by another customer
    V(order->wait_drink);

    // CRITICAL REGION END::
    lock_release(order_lock);


    
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */

void bar_open(void)
{
    // counter for looping
    int i;
    // tmp value to prevent memleak
    char *tmp_str ;

    // initial the order queue by reset order_hi and order_lo
    order_hi = order_lo = 0;
    // initial the drink sem queue by reset the drink_hi and drink_lo
    // Note: the inital queue is full of semaphore.
    drink_lo = drink_hi = 0;

    // continue: create order lock for order list
    order_lock = lock_create("order_lock");
    // continue: create semaphore for consumer/ producer
    full_sem  = sem_create("order_full",NCUSTOMERS);
    empty_sem = sem_create("order_empty",0);

    // initial the drink lock array to wait the mix is done
    for(i =0; i< NCUSTOMERS; i ++){
        tmp_str = get_name("drink_sem",i);
        drink_sem[i] = sem_create(tmp_str,0);
        kfree(tmp_str);
    }

    // initial the bottle lock array
    for(i=0; i < NBOTTLES; i++){
        tmp_str = get_name("bottle_lock",i);
        bottle_lock[i] = lock_create(tmp_str);
        kfree(tmp_str);
    }

}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{
    // counter for the loop
    int i = 0;
    
    // destory the semaphore and lock use by order control
    lock_destroy(order_lock);
    sem_destroy(full_sem);
    sem_destroy(empty_sem);


    // destory the drink lock array to wait the mix is done
    for(i =0; i< NCUSTOMERS; i ++){
        sem_destroy(drink_sem[i]);
    }

    // destory the bottle lock array
    for(i=0; i < NBOTTLES; i++){
        lock_destroy(bottle_lock[i]);
    }

}

/*
 * **********************************************************************
 * HELPER FUNCTION AREA
 * **********************************************************************
 */

void sort_requested_bottles(struct barorder *order){
	// simple bubble sort to meet the requirement of convenstion
	// which is require the bottle by accending order
	int swap =1;

	// set up the temp value to record bottle id
	unsigned int tmp_bot;

	while(swap){
		// reset the flag value
		swap = 0;
		for (int i =0 ; i < DRINK_COMPLEXITY -1; i++){
			if (order->requested_bottles[i] > 
				order->requested_bottles[i+1]){
				// swap the bottle require order
				tmp_bot 						  =
					order->requested_bottles[i];
				order->requested_bottles[i] =
					order->requested_bottles[i+1];
				order->requested_bottles[i+1] =
					tmp_bot;
                swap = 1;
			}
		}
	}
}


void take_bottles(struct barorder * this_order){
    // counter for the loop
    int i,j;
    // reset the index of bottle_lock
    j = 0;
    for(i=0; i < DRINK_COMPLEXITY; i ++){
        // clean up the bottle hold by previous order
        this_order ->hold_bottle[i] = NULL;


        /**
         * request bottle is not 0
         * then requeset the drink which is not require before 
         */
        if(this_order->requested_bottles[i]
            && ( i== 0 || 
            this_order->requested_bottles[i]!= this_order->requested_bottles[i-1] 
            )
        ){
            // only the not duplicate lock can be acquire
            this_order->hold_bottle[j] 
                = bottle_lock[this_order->requested_bottles[i]-1]; 
            // take this lock from carbinet
            lock_acquire(this_order->hold_bottle[j]);

            // add up the counter for hold_bottle
            j++;

        }
    }

}
void return_bottles(struct barorder * this_order){
    // counter for the loop
    int i;
    for(i=DRINK_COMPLEXITY -1; i >= 0; i --){
        // release the bottle by reverse order
        if(this_order->hold_bottle[i]!= NULL){
            // release used bottle
            lock_release(this_order->hold_bottle[i]);
        }
        
    }

}

//function for generating names for semaphores and locks
char *get_name(const char *main_name, int count) {
	int str_len = 0;
	while (main_name[str_len] != '\0') {
		str_len++;
	}

	// malloc the return char's space
	char *ret = kmalloc(str_len * 4 + 12);
	for (int i = 0; i< str_len; i++) {
		// strcpy
		ret[i] = main_name[i];
	}
	// get the count to the name
	ret[str_len] = '0' + count / 10;
	ret[str_len + 1] = '0' + count % 10;
	// get the null at the end
	ret[str_len + 2] = '\0';
	return ret;
}