/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <copyinout.h>
#include "opt-A2.h"

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** argv)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	//(void)argv;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
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
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	int index = 0;
	int arg_length;

	// We iterate through the char** on the kernel here. 
	while(argv[index] != NULL){
			char *argument;
			// Adding 1 to the length for for \0
			arg_length = strlen(argv[index]) + 1; 
			int original_length = arg_length;
			//Checking if the length of this string is divisible by 4 or not otherwise we make 
			if (arg_length % 4 != 0) {
				arg_length += (4 - (arg_length % 4));
			}
			//arg_length = ROUNDUP(arg_length, 4);
			//DEBUG(DB_SYSCALL,"Old Arg_length = %d, New Arg_length : %d",original_length,arg_length);

			argument = kmalloc(sizeof(arg_length));
			argument = kstrdup(argv[index]);
			int i = 0;
			while (i < arg_length){
				if (i < original_length){
					argument[i] = argv[index][i];
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
				kfree(argument);
				return result;
			}

			kfree(argument);

			argv[index] = (char *)stackptr;
			index+= 1;	
		}

			if (argv[index] == NULL) {
					stackptr -= 4 * sizeof(char);
			}

		int counter = index-1; // As we don't want NULL
		while (counter >= 0){
			stackptr = stackptr - sizeof(char*);
			result = copyout((const void *)(argv+counter), (userptr_t) stackptr, (sizeof(char *)));
			if (result) {
				return result;
			}
			counter -= 1;
		}


	/* Warp to user mode. */
	enter_new_process(index /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

