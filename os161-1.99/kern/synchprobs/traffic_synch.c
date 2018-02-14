#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <queue.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
//static struct semaphore *intersectionSem;
// static struct lock *entryLock;
// static struct lock *exitLock;
// static struct cv *NS;
// static struct cv *SN;
// static struct cv *EW;
// static struct cv *WE;
// static struct cv *NW;
// static struct cv *WN;
// static struct cv *NE;
// static struct cv *EN;
// static struct cv *SW;
// static struct cv *WS;
// static struct cv *SE;
// static struct cv *ES;
volatile int ns = 0;
volatile int sn = 0;
volatile int ew = 0;
volatile int we = 0;
volatile int nw = 0;
volatile int wn = 0;
volatile int ne = 0;
volatile int en = 0;
volatile int sw = 0;
volatile int ws = 0;
volatile int se = 0;
volatile int es = 0;

volatile int originNorthCounter = 0;
volatile int originSouthCounter = 0;
volatile int originEastCounter = 0;
volatile int originWestCounter = 0;

volatile int carsAllowed = 0;
static int carsInsideIntersection = 0;

static struct lock *cvLock;
static struct cv *originNorth;
static struct cv *originSouth;
static struct cv *originEast;
static struct cv *originWest;

static struct queue *CarwaitingQueue;
static bool isitQueued[4] = {false};


struct OriginDirection {
  Direction origin;
};



/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  originNorth = cv_create("originNorth");
  originSouth = cv_create("originSouth");
  originEast = cv_create("originEast");
  originWest = cv_create("originWest");
  cvLock = lock_create("cvLock");
  CarwaitingQueue = q_create(4);

  if (originNorth == NULL || originSouth == NULL || originEast == NULL || originWest == NULL || cvLock == NULL){
    panic("could not create some cv or entryLock or exitLock");
  }

  if (CarwaitingQueue == NULL) {
    panic("couldn't create the queue");
  }
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(q_empty(CarwaitingQueue));
  KASSERT(CarwaitingQueue != NULL);
  q_destroy(CarwaitingQueue);
  cv_destroy(originNorth);
  cv_destroy(originSouth);
  cv_destroy(originEast);
  cv_destroy(originWest);
  lock_destroy(cvLock);

}

/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

static void addToQueue(Direction origin) {

  if (!isitQueued[origin]){

    struct OriginDirection *item = kmalloc(sizeof(*item));

    item->origin = origin;

    q_addtail(CarwaitingQueue, item);

    isitQueued[origin] = true;
  }

}

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  // (void)origin;  /* avoid compiler complaint about unused parameter */
  // (void)destination; /* avoid compiler complaint about unused parameter */
  // KASSERT(intersectionSem != NULL);

lock_acquire(cvLock);

  if(!q_empty(CarwaitingQueue)) {
    if (!isitQueued[origin]) {
      addToQueue(origin);
    }
    if (origin == north){
      cv_wait(originNorth,cvLock);
    } else if (origin == south){
      cv_wait(originSouth,cvLock);
    } else if (origin == east){
      cv_wait(originEast,cvLock);
    } else {
      cv_wait(originWest,cvLock);
    }
    
  }

  if (origin == north){
    if (destination == south){
        while(ew > 0 || we > 0 || ws > 0 || es > 0 || wn > 0 || sw > 0){
          addToQueue(origin);
          cv_wait(originNorth,cvLock);
        }
        ns++;
    } else if (destination == east){
        while(se > 0 || we > 0 || sw > 0 || wn > 0 || ew > 0 || sn > 0 || es > 0){
          addToQueue(origin);
          cv_wait(originNorth,cvLock);
        }
        ne++;
    } else if (destination == west){
      while(sw > 0 || ew > 0){
        addToQueue(origin);
        cv_wait(originNorth,cvLock);
      }
      nw++;
    }
  } else if (origin == south){
    if (destination == north) {
        while(ew > 0 || we > 0 || wn > 0 || en > 0 || ne > 0 || es > 0){
          addToQueue(origin);
          cv_wait(originSouth,cvLock);
        }
        sn++; 
    } else if (destination == east){
        while(ne > 0 || we > 0){
          addToQueue(origin);
          cv_wait(originSouth,cvLock);
        }
        se++;
    } else if (destination == west){
        while(ne > 0 || ew > 0 || nw > 0 || wn > 0 || we > 0 || ns > 0 || es > 0){
          addToQueue(origin);
          cv_wait(originSouth,cvLock);
        }
        sw++;
    }
  } else if (origin == west){
    if (destination == north){
        while(ns > 0 || sn > 0 || es > 0 || en > 0 || sw > 0 || ew > 0 || ne > 0){
          addToQueue(origin);
          cv_wait(originWest,cvLock);
        }
        wn++;
    } else if (destination == south){
        while(ns > 0 || es > 0){
          addToQueue(origin);
          cv_wait(originWest,cvLock);
        }
        ws++;

    } else if (destination == east){
        while(ns > 0 || sn > 0 || ne > 0 || se > 0 || es > 0 || sw > 0){
          addToQueue(origin);
          cv_wait(originWest,cvLock);
        }
        we++;
    }
  } else if (origin == east){
    if (destination == north){
        while(wn > 0 || sn > 0){
          addToQueue(origin);
          cv_wait(originEast,cvLock);
        }
        en++;
    } else if (destination == south){
        while(wn > 0 || ns > 0 || ws > 0 || sn > 0 || sw > 0 || we > 0 || ne > 0){
          addToQueue(origin);
          cv_wait(originEast,cvLock);
        }
        es++;
    } else if (destination == west){
        while(ns > 0 || sn > 0 || nw > 0 || sw > 0 || ne > 0 || wn > 0){
          addToQueue(origin);
          cv_wait(originEast,cvLock);
        }
        ew++;
    }
  }
  carsInsideIntersection++;
  lock_release(cvLock);
}

/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  // (void)origin;  /* avoid compiler complaint about unused parameter */
  // (void)destination; /* avoid compiler complaint about unused parameter */
  lock_acquire(cvLock);
  carsInsideIntersection--;

  if (origin == north){
      
    if (destination == south){
        ns--;
    } else if (destination == east){
        ne--;
    } else if (destination == west){
        nw--;
    }
        // cv_broadcast(originWest,cvLock);
        // cv_broadcast(originEast,cvLock);
        // cv_broadcast(originSouth,cvLock);

    } else if (origin == south){
        if (destination == north) {
            sn--;
        } else if (destination == east){
            se--;
        } else if (destination == west){
            sw--;
        }

        // cv_broadcast(originNorth,cvLock);
        // cv_broadcast(originEast,cvLock);
        // cv_broadcast(originWest,cvLock);
        
    } else if (origin == west){
        if (destination == north){
            wn--;
        } else if (destination == south){
            ws--;
        } else if (destination == east){
            we--;
        }

        // cv_broadcast(originEast,cvLock);
        // cv_broadcast(originSouth,cvLock);
        // cv_broadcast(originNorth,cvLock);
    } else if (origin == east){
        if (destination == north){
            en--;
        } else if (destination == south){
            es--;
        } else if (destination == west){
            ew--;
        }
      }

  if (!q_empty(CarwaitingQueue) && carsInsideIntersection == 0) {
    // Get the first car waiting 
    struct OriginDirection *head = q_peek(CarwaitingQueue);
    // Get the origin of this car
    Direction OriginWaitingCar = head->origin;
    // now remove it from the queue
    q_remhead(CarwaitingQueue);


    // cv_broadcast(originWest,cvLock);
        // cv_broadcast(originNorth,cvLock);
        // cv_broadcast(originSouth,cvLock);
    if (OriginWaitingCar == north) {
      isitQueued[OriginWaitingCar]=false;
      cv_broadcast(originNorth, cvLock);
    }
    else if (OriginWaitingCar == south) {
      isitQueued[OriginWaitingCar]=false;
      cv_broadcast(originSouth, cvLock);
    }
    else if (OriginWaitingCar == east) {
      isitQueued[OriginWaitingCar]=false;
      cv_broadcast(originEast, cvLock);
    }
    else {
      isitQueued[OriginWaitingCar]=false;
      cv_broadcast(originWest, cvLock);
    }
}

lock_release(cvLock);

}

