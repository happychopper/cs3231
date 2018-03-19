#ifndef BAR_H
#define BAR_H
#include <synch.h>

#include "barglass.h"

/*
 * You are free to add anything you think you require to this file,
 * with the exceptions noted below.
 */

/* struct barorder is the main type referred to in the code. It must
   be preserved as noted for our later testing to work */

struct barorder {
        unsigned int requested_bottles[DRINK_COMPLEXITY]; /* Do not change */
        int go_home_flag;                                 /* Do not change */
        struct glass glass;                               /* Do not change */

        /* This struct can be extended with your own entries below here */ 
		int serving_no; //use together with waiting_bartender() function
};


char *get_name(const char *main_name, int count);
int mixable(struct barorder *order);
void sort_requested_bottles(struct barorder *order);
void take_bottles(struct barorder *order);
void return_bottles(struct barorder *order);

#endif
