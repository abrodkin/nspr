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
** RCCondition - C++ wrapper around NSPR's PRCondVar
*/

#include "rccv.h"

#include <prlog.h>
#include <prerror.h>
#include <prcvar.h>

RCCondition::RCCondition(class RCLock *lock): RCBase()
{
    cv = PR_NewCondVar(lock->lock);
    PR_ASSERT(NULL != cv);
    timeout = PR_INTERVAL_NO_TIMEOUT;
}  /* RCCondition::RCCondition */

RCCondition::~RCCondition()
{
    if (NULL != cv) PR_DestroyCondVar(cv);
}  /* RCCondition::~RCCondition */

PRStatus RCCondition::Wait()
{
    PRStatus rv;
    PR_ASSERT(NULL != cv);
    if (NULL == cv)
    {
        SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        rv = PR_FAILURE;
    }
    else
        rv = PR_WaitCondVar(cv, timeout.interval);
    return rv;
}  /* RCCondition::Wait */

PRStatus RCCondition::Notify()
{
    return PR_NotifyCondVar(cv);
}  /* RCCondition::Notify */

PRStatus RCCondition::Broadcast()
{
    return PR_NotifyAllCondVar(cv);
}  /* RCCondition::Broadcast */

PRStatus RCCondition::SetTimeout(const RCInterval& tmo)
{
    if (NULL == cv)
    {
        SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }
    timeout = tmo;
    return PR_SUCCESS;
}  /* RCCondition::SetTimeout */

RCInterval RCCondition::GetTimeout() const { return timeout; }

/* rccv.cpp */
