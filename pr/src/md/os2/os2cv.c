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

/*
 *  os2cv.c -- OS/2 Machine-Dependent Code for Condition Variables
 *
 *  We implement our own condition variable wait queue.  Each thread
 *  has a semaphore object (thread->md.blocked_sema) to block on while
 *  waiting on a condition variable.
 *
 *  We use a deferred condition notify algorithm.  When PR_NotifyCondVar
 *  or PR_NotifyAllCondVar is called, the condition notifies are simply
 *  recorded in the _MDLock structure.  We defer the condition notifies
 *  until right after we unlock the lock.  This way the awakened threads
 *  have a better chance to reaquire the lock.
 */
 
#include "primpl.h"

/*
 * AddThreadToCVWaitQueueInternal --
 *
 * Add the thread to the end of the condition variable's wait queue.
 * The CV's lock must be locked when this function is called.
 */

static void
AddThreadToCVWaitQueueInternal(PRThread *thred, struct _MDCVar *cv)
{
    PR_ASSERT((cv->waitTail != NULL && cv->waitHead != NULL)
            || (cv->waitTail == NULL && cv->waitHead == NULL));
    cv->nwait += 1;
    thred->md.inCVWaitQueue = PR_TRUE;
    thred->md.next = NULL;
    thred->md.prev = cv->waitTail;
    if (cv->waitHead == NULL) {
        cv->waitHead = thred;
    } else {
        cv->waitTail->md.next = thred;
    }
    cv->waitTail = thred;
}

/*
 * md_UnlockAndPostNotifies --
 *
 * Unlock the lock, and then do the deferred condition notifies.
 * If waitThred and waitCV are not NULL, waitThred is added to
 * the wait queue of waitCV before the lock is unlocked.
 *
 * This function is called by _PR_MD_WAIT_CV and _PR_MD_UNLOCK,
 * the two places where a lock is unlocked.
 */
static void
md_UnlockAndPostNotifies(
    _MDLock *lock,
    PRThread *waitThred,
    _MDCVar *waitCV)
{
    PRIntn index;
    _MDNotified post;
    _MDNotified *notified, *prev = NULL;

    /*
     * Time to actually notify any conditions that were affected
     * while the lock was held.  Get a copy of the list that's in
     * the lock structure and then zero the original.  If it's
     * linked to other such structures, we own that storage.
     */
    post = lock->notified;  /* a safe copy; we own the lock */

#if defined(DEBUG)
    memset(&lock->notified, 0, sizeof(_MDNotified));  /* reset */
#else
    lock->notified.length = 0;  /* these are really sufficient */
    lock->notified.link = NULL;
#endif

    /* 
     * Figure out how many threads we need to wake up.
     */
    notified = &post;  /* this is where we start */
    do {
        for (index = 0; index < notified->length; ++index) {
            _MDCVar *cv = notified->cv[index].cv;
            PRThread *thred;
            int i;
            
            /* Fast special case: no waiting threads */
            if (cv->waitHead == NULL) {
                notified->cv[index].notifyHead = NULL;
                continue;
            }

            /* General case */
            if (-1 == notified->cv[index].times) {
                /* broadcast */
                thred = cv->waitHead;
                while (thred != NULL) {
                    thred->md.inCVWaitQueue = PR_FALSE;
                    thred = thred->md.next;
                }
                notified->cv[index].notifyHead = cv->waitHead;
                cv->waitHead = cv->waitTail = NULL;
                cv->nwait = 0;
            } else {
                thred = cv->waitHead;
                i = notified->cv[index].times;
                while (thred != NULL && i > 0) {
                    thred->md.inCVWaitQueue = PR_FALSE;
                    thred = thred->md.next;
                    i--;
                }
                notified->cv[index].notifyHead = cv->waitHead;
                cv->waitHead = thred;
                if (cv->waitHead == NULL) {
                    cv->waitTail = NULL;
                } else {
                    if (cv->waitHead->md.prev != NULL) {
                        cv->waitHead->md.prev->md.next = NULL;
                        cv->waitHead->md.prev = NULL;
                    }
                }
                cv->nwait -= notified->cv[index].times - i;
            }
        }
        notified = notified->link;
    } while (NULL != notified);

    if (waitThred) {
        AddThreadToCVWaitQueueInternal(waitThred, waitCV);
    }

    /* Release the lock before notifying */
        DosReleaseMutexSem(lock->mutex);

    notified = &post;  /* this is where we start */
    do {
        for (index = 0; index < notified->length; ++index) {
            PRThread *thred;
            PRThread *next;

            thred = notified->cv[index].notifyHead;
            while (thred != NULL) {
                BOOL rv;

                next = thred->md.next;
                thred->md.prev = thred->md.next = NULL;
                rv = DosPostEventSem(thred->md.blocked_sema);
                PR_ASSERT(rv == NO_ERROR);
                thred = next;
            }
        }
        prev = notified;
        notified = notified->link;
        if (&post != prev) PR_DELETE(prev);
    } while (NULL != notified);
}

/*
 * Notifies just get posted to the protecting mutex.  The
 * actual notification is done when the lock is released so that
 * MP systems don't contend for a lock that they can't have.
 */
static void md_PostNotifyToCvar(_MDCVar *cvar, _MDLock *lock,
        PRBool broadcast)
{
    PRIntn index = 0;
    _MDNotified *notified = &lock->notified;

    while (1) {
        for (index = 0; index < notified->length; ++index) {
            if (notified->cv[index].cv == cvar) {
                if (broadcast) {
                    notified->cv[index].times = -1;
                } else if (-1 != notified->cv[index].times) {
                    notified->cv[index].times += 1;
                }
                return;
            }
        }
        /* if not full, enter new CV in this array */
        if (notified->length < _MD_CV_NOTIFIED_LENGTH) break;

        /* if there's no link, create an empty array and link it */
        if (NULL == notified->link) {
            notified->link = PR_NEWZAP(_MDNotified);
        }

        notified = notified->link;
    }

    /* A brand new entry in the array */
    notified->cv[index].times = (broadcast) ? -1 : 1;
    notified->cv[index].cv = cvar;
    notified->length += 1;
}

/*
 * _PR_MD_NEW_CV() -- Creating new condition variable
 * ... Solaris uses cond_init() in similar function.
 *
 * returns: -1 on failure
 *          0 when it succeeds.
 *
 */
PRInt32
_PR_MD_NEW_CV(_MDCVar *cv)
{
    cv->magic = _MD_MAGIC_CV;
    /*
     * The waitHead, waitTail, and nwait fields are zeroed
     * when the PRCondVar structure is created.
     */
    return 0;
} 

void _PR_MD_FREE_CV(_MDCVar *cv)
{
    cv->magic = (PRUint32)-1;
    return;
}

/*
 *  _PR_MD_WAIT_CV() -- Wait on condition variable
 */
void
_PR_MD_WAIT_CV(_MDCVar *cv, _MDLock *lock, PRIntervalTime timeout )
{
    PRThread *thred = _PR_MD_CURRENT_THREAD();
    ULONG rv, count;
    ULONG msecs = (timeout == PR_INTERVAL_NO_TIMEOUT) ?
            SEM_INDEFINITE_WAIT : PR_IntervalToMilliseconds(timeout);

    /*
     * If we have pending notifies, post them now.
     */
    if (0 != lock->notified.length) {
        md_UnlockAndPostNotifies(lock, thred, cv);
    } else {
        AddThreadToCVWaitQueueInternal(thred, cv);
        DosReleaseMutexSem(lock->mutex); 
    }

    /* Wait for notification or timeout; don't really care which */
    rv = DosWaitEventSem(thred->md.blocked_sema, msecs);
    if (rv == NO_ERROR) {
        DosResetEventSem(thred->md.blocked_sema, &count);
    }

    DosRequestMutexSem((lock->mutex), SEM_INDEFINITE_WAIT);

    PR_ASSERT(rv == NO_ERROR || rv == ERROR_TIMEOUT);

    if(rv == ERROR_TIMEOUT)
    {
       if (thred->md.inCVWaitQueue) {
           PR_ASSERT((cv->waitTail != NULL && cv->waitHead != NULL)
                   || (cv->waitTail == NULL && cv->waitHead == NULL));
           cv->nwait -= 1;
           thred->md.inCVWaitQueue = PR_FALSE;
           if (cv->waitHead == thred) {
               cv->waitHead = thred->md.next;
               if (cv->waitHead == NULL) {
                   cv->waitTail = NULL;
               } else {
                   cv->waitHead->md.prev = NULL;
               }
           } else {
               PR_ASSERT(thred->md.prev != NULL);
               thred->md.prev->md.next = thred->md.next;
               if (thred->md.next != NULL) {
                   thred->md.next->md.prev = thred->md.prev;
               } else {
                   PR_ASSERT(cv->waitTail == thred);
                   cv->waitTail = thred->md.prev;
               }
           }
           thred->md.next = thred->md.prev = NULL;
       } else {
           /*
            * This thread must have been notified, but the
            * SemRelease call happens after SemRequest
            * times out.  Wait on the semaphore again to make it
            * non-signaled.  We assume this wait won't take long.
            */
           rv = DosWaitEventSem(thred->md.blocked_sema, SEM_INDEFINITE_WAIT);
           if (rv == NO_ERROR) {
               DosResetEventSem(thred->md.blocked_sema, &count);
           }
           PR_ASSERT(rv == NO_ERROR);
       }
    }
    PR_ASSERT(thred->md.inCVWaitQueue == PR_FALSE);
    return;
} /* --- end _PR_MD_WAIT_CV() --- */

void
_PR_MD_NOTIFY_CV(_MDCVar *cv, _MDLock *lock)
{
    md_PostNotifyToCvar(cv, lock, PR_FALSE);
    return;
}

PRStatus
_PR_MD_NEW_LOCK(_MDLock *lock)
{
    DosCreateMutexSem(0, &(lock->mutex), 0, 0);
    (lock)->notified.length=0;
    (lock)->notified.link=NULL;
    return PR_SUCCESS;
}

void
_PR_MD_FREE_LOCK(_MDLock *lock)
{
    DosCloseMutexSem(lock->mutex);
}

void _PR_MD_LOCK(_MDLock *lock)
{
    DosRequestMutexSem(lock->mutex, SEM_INDEFINITE_WAIT);
}

PRIntn
_PR_MD_TEST_AND_LOCK(_MDLock *lock)
{
    DosRequestMutexSem(lock->mutex, SEM_INDEFINITE_WAIT);
    return 0;
}

void
_PR_MD_NOTIFYALL_CV(_MDCVar *cv, _MDLock *lock)
{
    md_PostNotifyToCvar(cv, lock, PR_TRUE);
    return;
}

void
_PR_MD_UNLOCK(_MDLock *lock)
{
    if (0 != lock->notified.length) {
        md_UnlockAndPostNotifies(lock, NULL, NULL);
    } else {
        DosReleaseMutexSem(lock->mutex);
    }
    return;
}
