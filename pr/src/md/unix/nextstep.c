/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "primpl.h"

#import <mach/mach.h>
#import <mach/mach_error.h>
#import <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <syscall.h>



/*	These functions are hidden in NEXTSTEP, but beacuse they have syscall()
**	entries I can wrap these into their corresponding missing function.
*/
caddr_t
mmap(caddr_t addr, size_t len, int prot, int flags,
          int fildes, off_t off)
{
	return (caddr_t) syscall (SYS_mmap, addr, len, prot, flags, fildes, off);
}

int
munmap(caddr_t addr, size_t len)
{
	return syscall (SYS_munmap, addr, len);
}

int
mprotect(caddr_t addr, size_t len, int prot)
{
	return syscall (SYS_mprotect, addr, len, prot);
}


/* If found the brk() symbol in the sahred libraries but no syscall() entry ...
** I don't know whether it will work ...
int brk(void *endds)
{
	return syscall ();
}
*/

void *sbrk(int incr)
{
	return (void *) syscall (SYS_sbrk, incr);
}

/*	These are my mach based versions, untested and probably bad ...
*/
caddr_t my_mmap(caddr_t addr, size_t len, int prot, int flags,
          int fildes, off_t off)
{
	kern_return_t ret_val;
	
	/*	First map ...
	*/
	ret_val = map_fd ( fildes, 					/* fd				*/
	                  (vm_offset_t) off,		/* offset			*/
					  (vm_offset_t*)&addr,		/* address			*/
					  TRUE, 					/* find_space		*/
					  (vm_size_t) len);			/* size				*/

	if (ret_val != KERN_SUCCESS) {
    	mach_error("Error calling map_fd() in mmap", ret_val );
		return (caddr_t)0;
	}
	
	/*	... then protect (this is probably bad)
	*/
	ret_val = vm_protect( task_self(),			/* target_task 		*/
						 (vm_address_t)addr,	/* address			*/
						 (vm_size_t) len,		/* size 			*/
						 FALSE,					/* set_maximum		*/
						 (vm_prot_t) prot);		/* new_protection	*/
	if (ret_val != KERN_SUCCESS) {
		mach_error("vm_protect in mmap()", ret_val );
		return (caddr_t)0;
	}
	
	return addr;
}

int my_munmap(caddr_t addr, size_t len)
{
	kern_return_t ret_val;

	ret_val = vm_deallocate(task_self(),
	 						(vm_address_t) addr,
							(vm_size_t) len);

	if (ret_val != KERN_SUCCESS) {
		mach_error("vm_deallocate in munmap()", ret_val);
		return -1;
	}
	
	return 0;
}

int my_mprotect(caddr_t addr, size_t len, int prot)
{
	vm_prot_t mach_prot;
	kern_return_t ret_val;
	
	switch (prot) {
		case PROT_READ:		mach_prot = VM_PROT_READ;		break;
		case PROT_WRITE:	mach_prot = VM_PROT_WRITE;		break;
		case PROT_EXEC:		mach_prot = VM_PROT_EXECUTE;	break;
		case PROT_NONE:		mach_prot = VM_PROT_NONE;		break;
	}
	
	ret_val = vm_protect(task_self(),			/* target_task 		*/
						 (vm_address_t)addr,	/* address			*/
						 (vm_size_t) len,		/* size 			*/
						 FALSE,					/* set_maximum		*/
						 (vm_prot_t) prot);		/* new_protection	*/

	if (ret_val != KERN_SUCCESS) {
		mach_error("vm_protect in mprotect()", ret_val);
		return -1;
	}
	
	return 0;
}

char *strdup(const char *s1)
{
	int len = strlen (s1);
	char *copy = (char*) malloc (len+1);
	
	if (copy == (char*)0)
		return (char*)0;

	strcpy (copy, s1);

	return copy;
}

/* Stub rld functions
*/
extern NSObjectFileImageReturnCode NSCreateObjectFileImageFromFile(
    const char *pathName,
    NSObjectFileImage *objectFileImage)
{
	return NSObjectFileImageFailure;
}

extern void * NSAddressOfSymbol(
    NSSymbol symbol)
{
	return NULL;
}

extern NSModule NSLinkModule(
    NSObjectFileImage objectFileImage, 
    const char *moduleName, /* can be NULL */
    enum bool bindNow)
{
	return NULL;
}

extern NSSymbol NSLookupAndBindSymbol(
    const char *symbolName)
{
	return NULL;
}

extern enum bool NSUnLinkModule(
    NSModule module, 
    enum bool keepMemoryMapped)
{
	return 0;
}



void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#ifndef _PR_PTHREADS
    if (isCurrent) {
	(void) sigsetjmp(CONTEXT(t), 1);
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}

#ifndef _PR_PTHREADS

void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}

/* These functions should not be called for NEXTSTEP */
void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for NEXTSTEP.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for NEXTSTEP.");
	return PR_FAILURE;
}

#endif
