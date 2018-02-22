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
#include <vfs.h>
#include <kern/fcntl.h>
#include <limits.h>
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
	//int childlen2 = array_num(p->childrenArray);
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
	//DEBUG(DB_SYSCALL, "Called Sys_fork\n");
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

	//DEBUG(DB_SYSCALL, "Just created a child with pid %d\n",childProcess->p_pid);

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


int sys_execv(const char *progname,char **argv){
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	lock_acquire(execvLock);
	// Can the argv argument be a null pointer?
	if(progname == NULL || argv == NULL){
		return EFAULT;
	}
	// Create space for the program name on the kernel stack
	char *progname_kernel = (char*) kmalloc(sizeof(char)* PATH_MAX);
	size_t size;

	// Copy the progname from the userspace to the kernel stack
	result = copyinstr((const_userptr_t) progname, progname_kernel, PATH_MAX, &size);
	if (result) {
		kfree(progname_kernel);
		return EFAULT;
	}

	DEBUG(DB_SYSCALL,"Size of progname_kernel = %d\n",size);

	// Similarly, Create space for the argument list on the kernel stack.
	char **argv_kernel = (char**) kmalloc(sizeof(char **));

	// Similarly, Copy the pointer to char * ( list of strings ) from the userspace to kernel stack
	result = copyin((const_userptr_t) argv, argv_kernel, sizeof(char **));
	if (result) {
			kfree(progname_kernel);
			kfree(argv_kernel);
			return EFAULT;
	}


	char** arguments = argv;
	int argc = 0;
	int i = 0;

	// We're looping through the arguments on the userspace 
	// and we're creating pointers to the data in the kernel_stack and copying the corresponding data over.
	while(arguments[i] != NULL){
		argv_kernel[i] = (char*) kmalloc(sizeof(char) * PATH_MAX);
		result = copyinstr((const_userptr_t) argv[i], argv_kernel[i], PATH_MAX, &size);
		if (result) {
			kfree(argv_kernel);
			kfree(progname_kernel);
			return EFAULT;
		}
		i += 1;
		argc += 1;
	}
	argv_kernel[i] = NULL;
	DEBUG(DB_SYSCALL,"Number of args = %d\n",argc);
	/* Open the file. */

	// PROGRAM WORKS TILL HERE AS THE NUMBER OF ARGS ARE PRINTED CORRECTLY
	char *fname_temp;
	fname_temp = kstrdup(progname);
	result = vfs_open((char*)progname, O_RDONLY, 0, &v);
	if (result) {
		kfree(argv_kernel);
		kfree(fname_temp);
		kfree(progname_kernel);
		return result;
	}
	kfree(fname_temp);

	/* This check was for runprogram not needed for us. */
	// KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		kfree(argv_kernel);
		kfree(progname_kernel);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		kfree(argv_kernel);
		kfree(progname_kernel);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		kfree(argv_kernel);
		kfree(progname_kernel);
		return result;
	}

	// Now, we have the arguments on the kernel stack of the process 
	// and we need to copy arguments to the UserStack that we just defined. 
	int index = 0;
	int arg_length;

	// We iterate through the char** on the kernel here. 
	while(argv_kernel[index] != NULL){
			char *argument;
			// Adding 1 to the length for for \0
			arg_length = strlen(argv_kernel[index]) + 1; 
			int original_length = arg_length;
			//Checking if the length of this string is divisible by 4 or not otherwise we make 
			if (arg_length % 4 != 0) {
				arg_length += (4 - (arg_length % 4));
			}
			//arg_length = ROUNDUP(arg_length, 4);
			DEBUG(DB_SYSCALL,"Old Arg_length = %d, New Arg_length : %d",original_length,arg_length);


			argument = kmalloc(sizeof(arg_length));
			argument = kstrdup(argv_kernel[index]);

			int i = 0;
			while (i < arg_length){
				if (i < original_length){
					argument[i] = argv_kernel[index][i];
				} else{
					argument[i] = '\0';
				}
				i += 1;
			}

			// DEBUG(DB_SYSCALL,"VALUE OF ARGUMENT IN KERNEL AFTER PADDING : ");
			//Subtracting from the Stack pointer and then copying the item argument on the stack
			stackptr -= arg_length;

			result = copyout((const void *) argument, (userptr_t)stackptr,(size_t) arg_length);
			if (result) {
				kfree(argv_kernel);
				kfree(progname_kernel);
				kfree(argument);
				return result;
			}

			kfree(argument);
			argv_kernel[index] = (char *)stackptr;
			index+= 1;	
	}

		if (argv_kernel[index] == NULL ) {
				stackptr -= 4 * sizeof(char);
		}

		int counter = index-1; // As we don't want NULL
		while (counter >= 0){
			stackptr = stackptr - sizeof(char*);
			result = copyout((const void *) (argv_kernel+counter), (userptr_t) stackptr, (sizeof(char *)));
			counter -= 1;
			if (result) {
				kfree(argv_kernel);
				kfree(progname_kernel);
				return result;
			}
		}

		kfree(argv_kernel);
		kfree(progname_kernel);
		

		lock_release(execvLock);
	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t) stackptr,
				stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;

}

#endif
