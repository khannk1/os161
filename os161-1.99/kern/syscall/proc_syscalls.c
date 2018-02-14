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

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
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
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}


#if OPT_A2
int sys_fork(struct trapframe *tf, pid_t *retval){
  DEBUG(DB_SYSCALL, "Called Sys_fork\n");
  KASSERT(tf != NULL);
  KASSERT(retval != NULL);

  struct trapframe* child = kmalloc(sizeof(*tf));
  if (child == NULL){
    tf->tf_v0 = ENOMEM;
    tf->tf_a3 = -1;
    return (-1);
  }

  *child = *tf;

  struct proc *childProcess = proc_create_runprogram("childProcess");
  if (childProcess == NULL){
    kfree(child);
    tf->tf_v0 = ENOMEM;
    tf->tf_a3 = -1;
    return (-1);
  }

  int ret;
  ret = as_copy(curproc->p_addrspace,&(childProcess->p_addrspace));
  if (ret != 0){
    kfree(child);
    proc_destroy(childProcess);
    tf->tf_v0 = ENOMEM;
    tf->tf_a3 = -1;
    return (-1);
  }

  // lock_acquire(childProcess->process_lock);
  childProcess->parent_address = curproc;
  //DEBUG(DB_SYSCALL, "Just set the child's parent address to %p\n",curproc);
  //lock_release(childProcess->process_lock);

  DEBUG(DB_SYSCALL, "Just created a child with pid %d\n",childProcess->p_pid);

  unsigned *index;
  lock_acquire(curproc->process_lock);
  array_add(curproc->childrenArray, childProcess, index);
  lock_release(curproc->process_lock);
 
  //DEBUG(DB_SYSCALL, "Child ppid = %lu \n", (unsigned long) childProcess->p_pid);
  unsigned long data = 0;
  ret = thread_fork("childThread",childProcess,enter_forked_process,child,data);
  if (ret != 0){
    kfree(child);
    proc_destroy(childProcess);
    tf->tf_v0 = ENOTSUP;
    tf->tf_a3 = -1;
    return (-1);
  }

  *retval = childProcess->p_pid;
  return(0);
}

#endif
