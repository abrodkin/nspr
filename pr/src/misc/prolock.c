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
**  prolock.c -- NSPR Ordered Lock
** 
**  Implement the API defined in prolock.h
** 
*/
#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#include "prolock.h"
#include "prlog.h"
#include "prerror.h"

PR_IMPLEMENT(PROrderedLock *) 
    PR_CreateOrderedLock( 
        PRInt32 order,
        const char *name
)
{
#ifdef XP_MAC
#pragma unused( order, name )
#endif
    PR_ASSERT(!"Not implemented"); /* Not implemented yet */
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
} /*  end PR_CreateOrderedLock() */


PR_IMPLEMENT(void) 
    PR_DestroyOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); /* Not implemented yet */
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} /*  end PR_DestroyOrderedLock() */


PR_IMPLEMENT(void) 
    PR_LockOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); /* Not implemented yet */
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} /*  end PR_LockOrderedLock() */


PR_IMPLEMENT(PRStatus) 
    PR_UnlockOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); /* Not implemented yet */
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
} /*  end PR_UnlockOrderedLock() */

#else /* ! defined(FORCE_NSPR_ORDERED_LOCK) */
/*
** NSPR Ordered Lock is not defined when !DEBUG and !FORCE_NSPR_ORDERED_LOCK
**  
*/

/* Some compilers don't like an empty compilation unit. */
static int dummy = 0;
#endif /* defined(FORCE_NSPR_ORDERED_LOCK */
