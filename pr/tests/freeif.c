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
 * Copyright (C) 2000 Netscape Communications Corporation.  All
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
 * A test to see if the macros PR_DELETE and PR_FREEIF are
 * properly defined.  (See Bugzilla bug #39110.)
 */

#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>

static void Noop(void) { }

static void Fail(void)
{
    printf("FAIL\n");
    exit(1);
}

int main()
{
    int foo = 1;
    char *ptr = NULL;

    /* this fails to compile with the old definition of PR_DELETE */
    if (foo)
        PR_DELETE(ptr);
    else
        Noop();

    /* this nests incorrectly with the old definition of PR_FREEIF */
    if (foo)
        PR_FREEIF(ptr);
    else
        Fail();

    printf("PASS\n");
    return 0;
}
