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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <limits.h>
#include "opt-A3.h" 

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground.
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

/*
 * Wrap rma_stealmem in a spinlock.
 */
static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;

//static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static int *coremap = NULL;
static paddr_t firstAddress;
static int coremap_length = 0;

#define MIN(x, y) (((x) < (y)) ? (x) : (y))


int 
cmd_showcoremap(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	for(int i = 0;i < coremap_length; i ++){
		kprintf("Coremap at %d : %d\n",i,coremap[i]);
	}
	return 0;
}

int cmd_showpagetable(int nargs, char **args){
	(void)nargs;
	(void)args;
	struct addrspace *as;
	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	for(unsigned int i = 0; i < as->as_npages1; i++){
		kprintf("Page Num : %d , Physical address : %p",i,(void *)as->as_pbase1[i]);
	}
	return 0;
}

void
vm_bootstrap(void)
{
	/* Do nothing. */

	#if OPT_A3
	paddr_t lowAddress, highAddress;
	ram_getsize(&lowAddress,&highAddress);

	// Pages are in Virtual Memory and Frames are in Physical memory.
	int numPages = (highAddress - lowAddress)/PAGE_SIZE;
	int numFrames = numPages;
	coremap_length = numPages;
	// How do i know how many frames are used for coremap?
	// Need to see what's the memory used by the coremap?

	int CoremapSize = (sizeof(int) * numFrames);
	CoremapSize = ROUNDUP(CoremapSize,PAGE_SIZE);
	int numPagesCoremap =  CoremapSize / PAGE_SIZE;
	
	firstAddress = lowAddress;

    // We can just write to the memory as we own it as we are the kernel.
	coremap = (int*) PADDR_TO_KVADDR(lowAddress);
	// We are managing the memory of the CoreMap in the coremap, not suggested by lesley but FTW
	// You set the first page is used by the coremap itself 
	coremap[0] = numPagesCoremap;
	// You then set the rest of the pages to -1
	for(int i = 1; i < numPagesCoremap-1; i++){
		coremap[i] = -1;
	}
	// The rest of the pages are all zero, we havent used anything.
	for (int i = numPagesCoremap; i < numPages; i++){
		coremap[i] = 0;
	}
	#endif
}

static
paddr_t
getppages(unsigned long npages)
{

	// Instead of doing this we need to actually get ppages.
	// ram_stealmem was only for the vm_bootstrap
	#if OPT_A3
	if (coremap == NULL){

		paddr_t addr;
		addr = ram_stealmem(npages);
		return addr;

	} else {
	
		paddr_t paddr;
		bool foundSpace = false;
		for(int i = 0; i < coremap_length;i++){
			if(coremap[i] != 0){
				continue;
			} else {
				unsigned long tempCount = 0;
				for(unsigned long j = i; j < i + npages; j++){
					if(coremap[j] == 0){
						tempCount += 1;
					}
				}
				if(tempCount == npages){
					foundSpace = true;
					coremap[i] = npages;
					unsigned long minLen = MIN(npages,(unsigned long)coremap_length);
					for(unsigned long k = i+1; k < i + minLen; k++){
						coremap[k] = -1;
					}
					paddr = firstAddress + (PAGE_SIZE * i);
					break;
				}
			}
		}

		if(foundSpace){
			return paddr;
		} else{
			return 0;
		}

	}
	
	#endif
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	spinlock_acquire(&coremap_lock);
	pa = getppages(npages);
	spinlock_release(&coremap_lock);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */
	spinlock_acquire(&coremap_lock);
	vaddr_t paddr = addr - MIPS_KSEG0;
	int frameNumber = (paddr - firstAddress)/PAGE_SIZE;
	int numPages = coremap[frameNumber];
	for(int i = frameNumber; i < frameNumber+numPages; i++){
		coremap[i] = 0;
	}
	spinlock_release(&coremap_lock);
	(void)addr;
}

void 
free_upages(paddr_t addr)
{
	/* nothing - leak the memory. */
	spinlock_acquire(&coremap_lock);
	int frameNumber = (addr - firstAddress)/PAGE_SIZE;
	int numPages = coremap[frameNumber];
	for(int i = frameNumber; i < frameNumber+numPages; i++){
		coremap[i] = 0;
	}
	spinlock_release(&coremap_lock);
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	int isText = 0;

	//kprintf("faultaddress before AND: %p\n",(void *)faultaddress);
	faultaddress &= PAGE_FRAME;
	//kprintf("faultaddress AFTER AND : %p\n",(void *)faultaddress);

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		//kill_curthread(EX_MOD);
		//panic("dumbvm: got VM_FAULT_READONLY\n");
		return EINVAL;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	//KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	//KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	//KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		//paddr = (faultaddress - vbase1) + as->as_pbase1;
		paddr_t newaddr = faultaddress - vbase1;
		//kprintf("faultaddress after subtracting : %p\n",(void *)newaddr);
		newaddr &= PAGE_FRAME;
		//kprintf("faultaddress after anding: %p\n",(void *)newaddr);
		int pagenum = newaddr/PAGE_SIZE;
		paddr = as->as_pbase1[pagenum];
		//kprintf("Pagenum : %d\n",pagenum);
		//kprintf("Physical address : %p\n",(void *)paddr);
		//kprintf("Physical address & PAGE_FRAME: %p\n", (void *)(paddr & PAGE_FRAME));
		isText = 1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		//paddr = (faultaddress - vbase2) + as->as_pbase2;
		paddr_t newaddr = faultaddress - vbase2;
		//kprintf("faultaddress after subtracting : %p\n",(void *)newaddr);
		newaddr &= PAGE_FRAME;
		//kprintf("faultaddress after anding: %p\n",(void *)newaddr);
		int pagenum = newaddr/PAGE_SIZE;
		paddr = as->as_pbase2[pagenum];
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		//paddr = (faultaddress - stackbase) + as->as_stackpbase;
		paddr_t newaddr = faultaddress - stackbase;
		//kprintf("faultaddress after subtracting : %p\n",(void *)newaddr);
		newaddr &= PAGE_FRAME;
		//kprintf("faultaddress after anding: %p\n",(void *)newaddr);
		int pagenum = newaddr/PAGE_SIZE;
		paddr = as->as_stackpbase[pagenum];
	}
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	// kprintf("Physical address before kassert: %p\n",(void *)paddr);
	// kprintf("Physical address & PAGE_FRAME before kassert: %p\n", (void *)(paddr & PAGE_FRAME));
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */

	//elo &= ~TLBLO_DIRTY;
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		if(isText == 1 && as->loaded == 1){
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
			elo &= ~TLBLO_DIRTY;
		} else {
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		}
		isText = 0;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	#if OPT_A3
	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	if (tlb_probe(ehi,elo) < 0){
		tlb_random(ehi,elo);
	}
	splx(spl);
	#endif
	// kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	// return EFAULT;
	return 0;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = NULL;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = NULL;
	as->as_npages2 = 0;
	as->as_stackpbase = kmalloc(sizeof(paddr_t) * DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return NULL;
	}
	for (unsigned int i = 0; i < DUMBVM_STACKPAGES; i++){
		as->as_stackpbase[i] = 0;
	}
	as->loaded = 0;

	return as;
}

void
as_destroy(struct addrspace *as)
{
	for(unsigned int i = 0; i < as->as_npages1; i++){
		free_upages(as->as_pbase1[i]);
	}
	for(unsigned int i = 0; i < as->as_npages2; i++){
		free_upages(as->as_pbase2[i]);
	}
	for(unsigned int i = 0; i < DUMBVM_STACKPAGES; i++){
		free_upages(as->as_stackpbase[i]);
	}
	free_upages((paddr_t)as->as_pbase1);
	free_upages((paddr_t)as->as_pbase2);
	free_upages((paddr_t)as->as_stackpbase);
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = curproc_getas();
#ifdef UW
        /* Kernel threads don't have an address spaces to activate */
#endif
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		//kprintf("Initializing the page table for TEXT SEGMENT with %d pages\n",as->as_npages1);
		as->as_pbase1 = kmalloc(sizeof(paddr_t) * npages);
		if (as->as_pbase1 == 0) {
			return ENOMEM;
		}
		for (unsigned int i = 0; i < npages; i++){
			as->as_pbase1[i] = 0;
		}
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		//kprintf("Initializing the page table for CODE with %d pages\n",as->as_npages2);
		as->as_pbase2 = kmalloc(sizeof(paddr_t) * as->as_npages2);
		if (as->as_pbase2 == 0) {
			return ENOMEM;
		}
		for (unsigned int i = 0; i < as->as_npages2; i++){
			as->as_pbase2[i] = 0;
		}
		return 0;
	}



	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{
	//KASSERT(as->as_pbase1 == 0);
	//KASSERT(as->as_pbase2 == 0);
	//KASSERT(as->as_stackpbase == 0);

	//kprintf("Getting frames for pagetable, Num pages %d\n",as->as_npages1);
	for(unsigned int i = 0; i < as->as_npages1; i++){
		paddr_t pageaddr = getppages(1);
		if(pageaddr == 0){
			return ENOMEM;
		}
		//kprintf("Physical address for pagetable: %p\n",(void *)pageaddr);
		as->as_pbase1[i] = pageaddr;
		as_zero_region(pageaddr, 1);
	}

	//kprintf("Getting frames for pagetable, Num pages %d\n",as->as_npages2);
	for(unsigned int i = 0; i < as->as_npages2; i++){
		paddr_t pageaddr = getppages(1);
		if(pageaddr == 0){
			return ENOMEM;
		}
		//kprintf("Physical address for pagetable %d : %p\n",i,(void *)pageaddr);
		as->as_pbase2[i] = pageaddr;
		as_zero_region(pageaddr, 1);
	}

	//as->as_pbase2 = getppages(as->as_npages2);

	for(unsigned int i = 0; i < DUMBVM_STACKPAGES; i++){
		paddr_t pageaddr = getppages(1);
		if(pageaddr == 0){
			return ENOMEM;
		}
		//kprintf("Physical address for pagetable %d : %p\n",i,(void *)pageaddr);
		as->as_stackpbase[i] = pageaddr;
		as_zero_region(pageaddr, 1);
	}

	//as->as_stackpbase = getppages(DUMBVM_STACKPAGES);	
	//as_zero_region(as->as_pbase1, as->as_npages1);
	//as_zero_region(as->as_pbase2, as->as_npages2);
	//as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);


	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

// uw-testbin/widefork
// sys161 kernel "p uw-testbin/hogparty;p uw-testbin/hogparty;p uw-testbin/hogparty;p uw-testbin/hogparty;p uw-testbin/hogparty;p uw-testbin/hogparty;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;p uw-testbin/widefork;q"
// sys161 kernel "p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p testbin/matmult;p uw-testbin/vm-data1;p uw-testbin/vm-data1;p uw-testbin/vm-data1; p testbin/matmult;"
// int
// as_define_stack_modified(struct addrspace *as, vaddr_t *stackptr,char** args)
// {
// 	KASSERT(as->as_stackpbase != 0);
// 	*stackptr = USERSTACK;

// 	char** argv_kernel = args;
// 	int index = 0;
// 	int result;
// 	int arg_length;
// 	// We iterate through the char** on the kernel here. 
// 	while(argv_kernel[index] != NULL){
// 			char *argument = NULL;
// 			// Adding 1 to the length for for \0
// 			arg_length = strlen(argv_kernel[index]) + 1; 
// 			size_t original_length = arg_length;
// 			//Checking if the length of this string is divisible by 4 or not otherwise we make 
// 			arg_length = ROUNDUP(arg_length, 8);
// 			DEBUG(DB_SYSCALL,"Old Arg_length = %d, New Arg_length : %d",original_length,arg_length);

// 			argument = kmalloc(sizeof(arg_length));
// 			argument = kstrdup(argv_kernel[index]);

// 			// DEBUG(DB_SYSCALL,"VALUE OF ARGUMENT IN KERNEL AFTER PADDING : ");
// 			//Subtracting from the Stack pointer and then copying the item argument on the stack

// 			// IMPORTANT : you need to subtract the stack ptr first and then you need to copyout as
// 			// Copyout will start and then grow upwards.
// 			stackptr -= arg_length;

// 			result = copyoutstr((const void *) argument, (userptr_t)stackptr,(size_t) arg_length, &original_length);
// 			if (result) {
// 				kfree(argument);
// 				return result;
// 			}
// 			kfree(argument);
// 			argv_kernel[index] = (char *)stackptr;
// 			index+= 1;	
// 	}

// 		if (argv_kernel[index] == NULL) {
// 				stackptr -= 4 * sizeof(char);
// 		}

// 		int counter = index-1; // As we don't want NULL
// 		while (counter >= 0){
// 			// Now we need to copy the pointers we created previously to the user stack.
// 			// So we subtract the stack ptr. 
// 			stackptr = stackptr - sizeof(char*);
// 			// Now we copyout this pointer to the Userstack
// 			result = copyout((const void *) (argv_kernel+counter), (userptr_t) stackptr, (sizeof(char *)));
// 			counter -= 1;
// 			if (result) {
// 				return result;
// 			}
// 		}
// 	return 0;
// }

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;


	// FOR THE TEXT SECTION
	new->as_pbase1 = kmalloc(sizeof(paddr_t)*new->as_npages1);
	if (new->as_pbase1 == 0) {
		return ENOMEM;
	}
	for (unsigned int i = 0; i < new->as_npages1; i++){
		new->as_pbase1[i] = 0;
	}

	new->as_pbase2 = kmalloc(sizeof(paddr_t) * new->as_npages2);
	if (new->as_pbase2 == 0) {
		return ENOMEM;
	}
	for (unsigned int i = 0; i < new->as_npages2; i++){
		new->as_pbase2[i] = 0;
	}

	new->as_stackpbase = kmalloc(sizeof(paddr_t) * DUMBVM_STACKPAGES);
	if (new->as_stackpbase == 0) {
		return ENOMEM;
	}
	for (unsigned int i = 0; i < DUMBVM_STACKPAGES; i++){
		new->as_stackpbase[i] = 0;
	}


	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	for(unsigned int i = 0; i < new->as_npages1; i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_pbase1[i]),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1[i]),PAGE_SIZE);
	}
	
	for(unsigned int i = 0; i < new->as_npages2; i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_pbase2[i]),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2[i]),PAGE_SIZE);
	}

	for(unsigned int i = 0; i < DUMBVM_STACKPAGES; i++){
		memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase[i]),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase[i]),PAGE_SIZE);
	}

	*ret = new;
	return 0;
}
