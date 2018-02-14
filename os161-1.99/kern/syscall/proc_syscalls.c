#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
// Added by me
#include <mips/trapframe.h>
#include <synch.h>
#include "opt-A2.h"

void sys__exit(int exitcode) {
    
    struct addrspace *as;
    struct proc *p = curproc;
    KASSERT(curproc->p_addrspace != NULL);
    as_deactivate();
    as = curproc_setas(NULL);
    as_destroy(as);
    proc_remthread(curthread);

    lock_acquire(p->process_lock);
    int childlen = array_num(p->childrenArray);
    if (childlen >= 1){
      for(unsigned int i = 0; i < array_num(p->childrenArray); i++){
          struct proc *child_p = (struct proc*)array_get(p->childrenArray,i);
          lock_acquire(child_p->process_lock);
          if (child_p->processStatus == 0){  
              lock_release(child_p->process_lock);
              proc_destroy_part2(child_p);
          } else if (child_p->processStatus == 1){
              child_p->parent_address = NULL;
              lock_release(child_p->process_lock);
          }
      }
    }

  //DEBUG(DB_SYSCALL,"Removing %d children\n", childlen);
  int i = 0;
  while(i < childlen){
    array_remove(p->childrenArray,0);
    i += 1;
  }
 // int childlen2 = array_num(p->childrenArray);
  //DEBUG(DB_SYSCALL,"New length of children %d \n", childlen2);
   
  if (p->parent_address == NULL){

    /* for now, just include this to keep the compiler from complaining about
       an unused variable */
    //DEBUG(DB_SYSCALL,"Parent is NULL lol!\n");

    //DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);     
    /* if this is the last user process in the system, proc_destroy()
       will wake up the kernel menu thread */
    lock_release(p->process_lock);
    proc_destroy(p);
    //DEBUG(DB_SYSCALL,"destroying proc done! WE'RE ALLL DONNEEEEE\n");
    thread_exit();
    /* thread_exit() does not return, so we should never get here */
    panic("return from thread_exit in sys_exit\n");

  } else {
    //DEBUG(DB_SYSCALL,"Parent is alive bro! and my parent's pid is %d\n", curproc->parent_address->p_pid);
    // Your parent is alive please become a zombie
    //DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

    p->p_exitcode = _MKWAIT_EXIT(exitcode);
    p->processStatus = 0;

    //DEBUG(DB_SYSCALL,"Waking up my parent cause im done. Peace out!\n");
    cv_signal(p->p_cv, p->process_lock);
    /* if this is the last user process in the system, proc_destroy()
       will wake up the kernel menu thread */
    proc_destroy_part1(p);
    //DEBUG(DB_SYSCALL,"proc_destroy_part1 finished!\n");
    lock_release(p->process_lock);
    //DEBUG(DB_SYSCALL,"lock released!\n");
    thread_exit();
    /* thread_exit() does not return, so we should never get here */
    panic("return from thread_exit in sys_exit\n");

  }
}

/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  #if OPT_A2
    *retval = curproc->p_pid;
  #endif
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
      userptr_t status,
      int options,
      pid_t *retval)
{

  int exitstatus;
  int result = 0;
  #if OPT_A2
  //DEBUG(DB_SYSCALL, "Called sys_waitpid\n");

  KASSERT(retval != NULL);
  bool foundChild = false;
  struct proc *p = curproc;
  struct proc *child_p;

  // Firstly, Only the parent can call WaitPID on its children. 
  // So you check if the given pid is a child of the cur proc?

  if (status == NULL){
    *retval = EFAULT;
    return (-1);
  }

  if (pid == p->p_pid) {
    *retval = ECHILD;
    return (-1);
  }

  if (options != 0) {
    *retval = EINVAL;
    return (-1);
  }
  //DEBUG(DB_SYSCALL, "I'm looking for child with pid %d\n",pid);

  unsigned int child_index = 0;
  for(unsigned int i = 0; i < array_num(p->childrenArray); i+= 1){
      struct proc *child = (struct proc*)array_get(p->childrenArray,i);
      //DEBUG(DB_SYSCALL, "current id : %d\n",child->p_pid);
      if (child->p_pid == pid){
        child_index = i;
        foundChild = true;
        child_p = child;
      }
  }

  if (foundChild == false){
    *retval = ECHILD;
    return (-1);
  } else {

      
    lock_acquire(child_p->process_lock);
      if (child_p->processStatus == 0){
        //DEBUG(DB_SYSCALL,"I found the child I'm waiting for and he's a zombie lol.\n");
        exitstatus = child_p->p_exitcode;
        result = copyout((void *)&exitstatus,status,sizeof(int));

      } else{
          //DEBUG(DB_SYSCALL,"I found the child I'm waiting for and he's working so I'm going to sleep.\n");
            //DEBUG(DB_SYSCALL,"Acquired my child's lock.\n");
            *retval = child_p->p_pid;
            while(child_p->processStatus == 1){
                // put yourself to sleep as you will wait for your child to finish
                cv_wait(child_p->p_cv , child_p->process_lock);
            }
            //DEBUG(DB_SYSCALL,"Okay my child finished now\n");
            exitstatus = child_p->p_exitcode;
            result = copyout((void *)&exitstatus,status,sizeof(int));
            //DEBUG(DB_SYSCALL,"Releasing the lock now\n");   
      }

      array_remove(p->childrenArray,child_index);
      lock_release(child_p->process_lock);
      proc_destroy_part2(child_p);
      
    return(result);
  }
#endif
}



#if OPT_A2
int sys_fork(struct trapframe *tf, pid_t *retval){
  DEBUG(DB_SYSCALL, "Called Sys_fork\n");
  KASSERT(tf != NULL);
  KASSERT(retval != NULL);

  struct proc *childProcess = proc_create_runprogram("childProcess");
  if (childProcess == NULL){
    return (ENOMEM);
  }

  int ret = as_copy(curproc->p_addrspace,&(childProcess->p_addrspace));
  if (ret != 0){
    proc_destroy(childProcess);
    return (ENOMEM);
  }

  struct trapframe* child_tf = kmalloc(sizeof(struct trapframe));
  if (child_tf == NULL){
    proc_destroy(childProcess);
    return (ENOMEM);
  }

  memcpy(child_tf,tf,sizeof(struct trapframe));

  // lock_acquire(childProcess->process_lock);
  childProcess->parent_address = curproc;
  //DEBUG(DB_SYSCALL, "Just set the child's parent address to %p\n",curproc);
  //lock_release(childProcess->process_lock);

  DEBUG(DB_SYSCALL, "Just created a child with pid %d\n",childProcess->p_pid);

  //lock_acquire(globalLock);
  array_add(curproc->childrenArray, childProcess, NULL);
  //lock_release(globalLock);

  //DEBUG(DB_SYSCALL, "Child ppid = %lu \n", (unsigned long) childProcess->p_pid);
  unsigned long data = 0;
  ret = thread_fork("childThread",childProcess,enter_forked_process,child_tf,data);
  if (ret != 0){
    // You've already added the child to your array. 
    // If you couldn't fork, figure out a way to remove him from your array here. 
    kfree(child_tf);
    proc_destroy(childProcess);
    return (ENOTSUP);
  }

  *retval = childProcess->p_pid;
  return(0);
}

#endif
