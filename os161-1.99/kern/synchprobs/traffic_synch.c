#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

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

static struct lock *cvLock;
static struct cv *originNorth;
static struct cv *originSouth;
static struct cv *originEast;
static struct cv *originWest;



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
  // NS = cv_create("NS");
  // SN = cv_create("SN");
  // EW = cv_create("EW");
  // WE = cv_create("WE");
  // NW = cv_create("NW");
  // WN = cv_create("WN");
  // NE = cv_create("NE");
  // EN = cv_create("EN");
  // SW = cv_create("SW");
  // WS = cv_create("WS");
  // SE = cv_create("SE");
  // ES = cv_create("ES");

  originNorth = cv_create("originNorth");
  originSouth = cv_create("originSouth");
  originEast = cv_create("originEast");
  originWest = cv_create("originWest");
  cvLock = lock_create("cvLock");

  if (originNorth == NULL || originSouth == NULL || originEast == NULL || originWest == NULL || cvLock == NULL){
    panic("could not create some cv or entryLock or exitLock");
  }

  // entryLock = lock_create("entryLock");
  // exitLock = lock_create("exitLock");

  // if (NS == NULL || SN == NULL ||EW == NULL || WE == NULL || NW == NULL || WN == NULL 
  //     || NE == NULL || EN == NULL || SW == NULL || WS == NULL || SE == NULL || 
  //     ES == NULL || exitLock == NULL ||entryLock == NULL){
  //   panic("could not create some cv or entryLock or exitLock");
  // }

  // intersectionSem = sem_create("intersectionSem",1);
  // if (intersectionSem == NULL) {
  //   panic("could not create intersection semaphore");
  // }
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
  //KASSERT(cv != NULL);
  // cv_destroy(NS);
  // cv_destroy(SN);
  // cv_destroy(EW);
  // cv_destroy(WE);
  // cv_destroy(NW);
  // cv_destroy(WN);
  // cv_destroy(NE);
  // cv_destroy(EN);
  // cv_destroy(SW);
  // cv_destroy(WS);
  // cv_destroy(SE);
  // cv_destroy(ES);
  // lock_destroy(entryLock);
  // lock_destroy(exitLock);
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

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  // (void)origin;  /* avoid compiler complaint about unused parameter */
  // (void)destination; /* avoid compiler complaint about unused parameter */
  // KASSERT(intersectionSem != NULL);

  if (origin == north){
    if (destination == south){
      lock_acquire(cvLock);
        while(ew > 0 || we > 0 || ws > 0 || es > 0 || wn > 0 || sw > 0){
          cv_wait(originNorth,cvLock);
        }
        ns++;
      lock_release(cvLock);
    } else if (destination == east){
      lock_acquire(cvLock);
        while(se > 0 || we > 0 || sw > 0 || wn > 0 || ew > 0 || sn > 0 || es > 0){
          cv_wait(originNorth,cvLock);
        }
        ne++;
      lock_release(cvLock);
    } else if (destination == west){
    lock_acquire(cvLock);
      while(sw > 0 || ew > 0){
        cv_wait(originNorth,cvLock);
      }
      nw++;
    lock_release(cvLock);
    }
  } else if (origin == south){
    if (destination == north) {
      lock_acquire(cvLock);
        while(ew > 0 || we > 0 || wn > 0 || en > 0 || ne > 0 || es > 0){
          cv_wait(originSouth,cvLock);
        }
        sn++; 
      lock_release(cvLock);
    } else if (destination == east){
      lock_acquire(cvLock);
        while(ne > 0 || we > 0){
          cv_wait(originSouth,cvLock);
        }
        se++;
      lock_release(cvLock);
    } else if (destination == west){
      lock_acquire(cvLock);
        while(ne > 0 || ew > 0 || nw > 0 || wn > 0 || we > 0 || ns > 0 || es > 0){
          cv_wait(originSouth,cvLock);
        }
        sw++;
      lock_release(cvLock);
    }
  } else if (origin == west){
    if (destination == north){
      lock_acquire(cvLock);
        while(ns > 0 || sn > 0 || es > 0 || en > 0 || sw > 0 || ew > 0 || ne > 0){
          cv_wait(originWest,cvLock);
        }
        wn++;
      lock_release(cvLock);
    } else if (destination == south){
      lock_acquire(cvLock);
        while(ns > 0 || es > 0){
          cv_wait(originWest,cvLock);
        }
        ws++;
      lock_release(cvLock);
    } else if (destination == east){
      lock_acquire(cvLock);
        while(ns > 0 || sn > 0 || ne > 0 || se > 0 || es > 0 || sw > 0){
          cv_wait(originWest,cvLock);
        }
        we++;
      lock_release(cvLock);
    }
  } else if (origin == east){
    if (destination == north){
      lock_acquire(cvLock);
        while(wn > 0 || sn > 0){
          cv_wait(originEast,cvLock);
        }
        en++;
      lock_release(cvLock);    
    } else if (destination == south){
      lock_acquire(cvLock);
        while(wn > 0 || ns > 0 || ws > 0 || sn > 0 || sw > 0 || we > 0 || ne > 0){
          cv_wait(originEast,cvLock);
        }
        es++;
      lock_release(cvLock);
    } else if (destination == west){
      lock_acquire(cvLock);
        while(ns > 0 || sn > 0 || nw > 0 || sw > 0 || ne > 0 || wn > 0){
          cv_wait(originEast,cvLock);
        }
        ew++;
      lock_release(cvLock);
    }
  }

  // FIRST IMPLEMENTATION 
  // if (origin == north && destination == south){
  //   lock_acquire(entryLock);
  //   while(ew > 0 || we > 0 || ws > 0 || es > 0 || wn > 0 || sw > 0){
  //     if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     } else if (ws > 0){
  //       cv_wait(WS,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     }
  //   }
  //   ns++; 
  //   lock_release(entryLock);
  // } else if (origin == south && destination == north){
  //   lock_acquire(entryLock);
  //   while(ew > 0 || we > 0 || wn > 0 || en > 0 || ne > 0 || es > 0){
  //     if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (en > 0){
  //       cv_wait(EN,entryLock);
  //     } else if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     }
  //   }
  //   sn++;
  //   lock_release(entryLock);
  // } else if (origin == east && destination == west){
  //   lock_acquire(entryLock);
  //   while(ns > 0 || sn > 0 || nw > 0 || sw > 0 || ne > 0 || wn > 0){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     } else if (nw > 0){
  //       cv_wait(NW,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     } else if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     }
  //   }
  //   ew++;
  //   lock_release(entryLock);
  // } else if (origin == west && destination == east){
  //   lock_acquire(entryLock);
  //   while(ns > 0 || sn > 0 || ne > 0 || se > 0 || es > 0 || sw > 0){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     } else if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (se > 0){
  //       cv_wait(SE,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     }
  //   }
  //   we++;
  //   lock_release(entryLock);
  // } else if (origin == north && destination == west){
  //   lock_acquire(entryLock);
  //   while(sw > 0 || ew > 0){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     }
  //   }
  //   nw++;
  //   lock_release(entryLock);
  // } else if (origin == west && destination == north){
  //   lock_acquire(entryLock);
  //   while(ns > 0 || sn > 0 || es > 0 || en > 0 || sw > 0 || ew > 0 || ne > 0){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     } else if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (en > 0){
  //       cv_wait(EN,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     } else if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     }
  //   }
  //   wn++;
  //   lock_release(entryLock);
  // } else if (origin == north && destination == east){
  //   lock_acquire(entryLock);
  //   while(se > 0 || we > 0 || sw > 0 || wn > 0 || ew > 0 || sn > 0 || es > 0){
  //     if (se > 0){
  //       cv_wait(SE,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     } else if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     }
  //   }
  //   ne++;
  //   lock_release(entryLock);
  // } else if (origin == east && destination == north){
  //   lock_acquire(entryLock);
  //   while(wn > 0 || sn > 0){
  //     if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     }
  //   }
  //   en++;
  //   lock_release(entryLock);
  // } else if (origin == south && destination == west){
  //   lock_acquire(entryLock);
  //   while(ne > 0 || ew > 0 || nw > 0 || wn > 0 || we > 0 || ns > 0 || es > 0){
  //     if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (nw > 0){
  //       cv_wait(NW,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     } else if (ew > 0){
  //       cv_wait(EW,entryLock);
  //     }
  //   }
  //   sw++;
  //   lock_release(entryLock);
  // } else if (origin == west && destination == south){
  //   lock_acquire(entryLock);
  //   while(ns > 0 || es > 0 ){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (es > 0){
  //       cv_wait(ES,entryLock);
  //     }
  //   }
  //   ws++;
  //   lock_release(entryLock);
  // } else if (origin == south && destination == east){
  //   lock_acquire(entryLock);
  //   while(ne > 0 || we > 0){
  //     if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     }
  //   }
  //   se++;
  //   lock_release(entryLock);
  // } else if (origin == east && destination == south){
  //   lock_acquire(entryLock);
  //   while(wn > 0 || ns > 0 || ws > 0 || sn > 0 || sw > 0 || we > 0 || ne > 0){
  //     if (ns > 0){
  //       cv_wait(NS,entryLock);
  //     } else if (sn > 0){
  //       cv_wait(SN,entryLock);
  //     } else if (we > 0){
  //       cv_wait(WE,entryLock);
  //     } else if (wn > 0){
  //       cv_wait(WN,entryLock);
  //     } else if (ws > 0){
  //       cv_wait(WS,entryLock);
  //     } else if (sw > 0){
  //       cv_wait(SW,entryLock);
  //     } else if (ne > 0){
  //       cv_wait(NE,entryLock);
  //     }
  //   }
  //   es++;
  //   lock_release(entryLock);
  // }
  
  //P(intersectionSem);
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
  // KASSERT(intersectionSem != NULL);
  // V(intersectionSem);

    if (origin == north){
      lock_acquire(cvLock);
        if (destination == south){
            ns--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originSouth,cvLock);
        } else if (destination == east){
            ne--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originSouth,cvLock);
        } else if (destination == west){
            nw--;
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originSouth,cvLock);
        }
        cv_broadcast(originWest,cvLock);
        cv_broadcast(originEast,cvLock);
        cv_broadcast(originSouth,cvLock);
      lock_release(cvLock);
    } else if (origin == south){
      lock_acquire(cvLock);
        if (destination == north) {
            sn--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originNorth,cvLock);
        } else if (destination == east){
            se--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originNorth,cvLock);
        } else if (destination == west){
            sw--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originNorth,cvLock);
        }
        cv_broadcast(originNorth,cvLock);
        cv_broadcast(originEast,cvLock);
        cv_broadcast(originWest,cvLock);
      lock_release(cvLock);
    } else if (origin == west){
      lock_acquire(cvLock);
        if (destination == north){
            wn--;
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originNorth,cvLock);
            // cv_broadcast(originSouth,cvLock);
        } else if (destination == south){
            ws--;
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originNorth,cvLock);
        } else if (destination == east){
            we--;
            // cv_broadcast(originEast,cvLock);
            // cv_broadcast(originNorth,cvLock);
            // cv_broadcast(originSouth,cvLock);
        }
            cv_broadcast(originEast,cvLock);
            cv_broadcast(originNorth,cvLock);
            cv_broadcast(originSouth,cvLock);
      lock_release(cvLock);
    } else if (origin == east){
      lock_acquire(cvLock);
        if (destination == north){
            en--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originSouth,cvLock);
        } else if (destination == south){
            es--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originNorth,cvLock);
            // cv_broadcast(originSouth,cvLock);
        } else if (destination == west){
            ew--;
            // cv_broadcast(originWest,cvLock);
            // cv_broadcast(originNorth,cvLock);
            // cv_broadcast(originSouth,cvLock);
        }
        cv_broadcast(originSouth,cvLock);
        cv_broadcast(originWest,cvLock);
        cv_broadcast(originNorth,cvLock);
      lock_release(cvLock);
    }
}

  // FIRST IMPLEMENTATION
  // if (origin == north && destination == south){
  //   lock_acquire(exitLock);
  //   ns--;
  //   if(ns == 0){
  //     cv_broadcast(NS,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == south && destination == north){
  //   lock_acquire(exitLock);
  //   sn--;
  //   if(sn == 0){
  //     cv_broadcast(SN,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == east && destination == west){
  //   lock_acquire(exitLock);
  //   ew--;
  //   if(ew == 0){
  //     cv_broadcast(EW,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == west && destination == east){
  //   lock_acquire(exitLock);
  //   we--;
  //   if(we == 0){
  //     cv_broadcast(WE,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == north && destination == west){
  //   lock_acquire(exitLock);
  //   nw--;
  //   if(nw == 0){
  //     cv_broadcast(NW,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == west && destination == north){
  //   lock_acquire(exitLock);
  //   wn--;
  //   if(wn == 0){
  //     cv_broadcast(WN,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == north && destination == east){
  //   lock_acquire(exitLock);
  //   ne--;
  //   if(ne == 0){
  //     cv_broadcast(NE,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == east && destination == north){
  //   lock_acquire(exitLock);
  //   en--;
  //   if(en == 0){
  //     cv_broadcast(EN,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == south && destination == west){
  //   lock_acquire(exitLock);
  //   sw--;
  //   if(sw == 0){
  //     cv_broadcast(SW,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == west && destination == south){
  //   lock_acquire(exitLock);
  //   ws--;
  //   if(ws == 0){
  //     cv_broadcast(WS,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == south && destination == east){
  //   lock_acquire(exitLock);
  //   se--;
  //   if(se == 0){
  //     cv_broadcast(SE,exitLock);
  //   }
  //   lock_release(exitLock);
  // } else if (origin == east && destination == south){
  //   lock_acquire(exitLock);
  //   es--;
  //   if(es == 0){
  //     cv_broadcast(ES,exitLock);
  //   }
  //   lock_release(exitLock);
  // }
