/*-
 * Copyright (c) 2016-2017 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#if !__has_feature(capabilities)
#error "This code requires a CHERI-aware compiler"
#endif

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <machine/cpuregs.h>
#include <machine/sysarch.h>

#include <cheri/cheri.h>
#include <cheri/cheric.h>
#include <cheri/libcheri_enter.h>
#include <cheri/libcheri_errno.h>
#include <cheri/libcheri_fd.h>
#include <cheri/libcheri_sandbox.h>

#include <cheribsdtest-helper.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "cheribsdtest.h"

/*
 * Tests that validate capability-flow policies configured by the kernel and
 * libcheri:
 *
 * 1. That local capabilities cannot be passed to, or returned from, CCall/
 * CReturn.
 *
 * 2. That, within a sandbox, local capabilities can only be stored in the
 * stack, not BSS / globals.
 */

/*
 * Test that if we pass in a global capability, the sandbox can successfully
 * store it to sandbox bss.
 */
CHERIBSDTEST(test_sandbox_store_global_capability_in_bss,
    "Try to store global capability to sandbox bss",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	v = invoke_store_capability_in_bss(carg);
	if (v != 0)
		cheribsdtest_failure_errx("unexpected return value (%ld)", v);
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, the sandbox cannot store it
 * to sandbox bss.  Rely on the signal handler to unwind the stack.
 */
CHERIBSDTEST(test_sandbox_store_local_capability_in_bss_catch,
    "Try to store local capability to sandbox bss; caught",
    .ct_flags = CT_FLAG_SIGNAL | CT_FLAG_SI_CODE | CT_FLAG_SI_TRAPNO |
	CT_FLAG_SIGNAL_UNWIND | CT_FLAG_SANDBOX,
    .ct_signum = SIGPROT,
    .ct_si_code = PROT_CHERI_STORELOCAL,
    .ct_si_trapno = TRAPNO_CHERI },
{
	void * __capability carg;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	v = invoke_store_local_capability_in_bss(carg);
	if (v != -1)
		cheribsdtest_failure_errx("unexpected return value (%ld)", v);
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, and the sandbox recasts it to
 * be a local capability, it cannot store it to sandbox bss.  Disable the
 * signal handler.
 */
CHERIBSDTEST(test_sandbox_store_local_capability_in_bss_nocatch,
    "Try to store local capability to sandbox bss; uncaught",
    .ct_flags = CT_FLAG_SIGEXIT | CT_FLAG_SANDBOX,
    .ct_signum = SIGPROT },
{
	void * __capability carg;
	register_t v;

	signal_handler_clear(SIGPROT);
	carg = (__cheri_tocap void * __capability)&v;
	v = invoke_store_local_capability_in_bss(carg);
	if (v != -1)
		cheribsdtest_failure_errx("unexpected return value (%ld)", v);
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, the sandbox can successfully
 * store it to a sandbox's stack.
 */
CHERIBSDTEST(test_sandbox_store_global_capability_in_stack,
    "Try to store global capability to sandbox stack",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	v = invoke_store_capability_in_stack(carg);
	if (v != 0)
		cheribsdtest_failure_errx("unexpected return value (%ld)", v);
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, and the sandbox recasts it to
 * be a local capability, it can success store it to a sandbox's stack.
 */
CHERIBSDTEST(test_sandbox_store_local_capability_in_stack,
    "Try to store local capability to sandbox stack",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	v = invoke_store_local_capability_in_stack(carg);
	if (v != 0)
		cheribsdtest_failure_errx("unexpected return value (%ld)", v);
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, and the sandbox returns it,
 * that we get the same capability back.
 */
CHERIBSDTEST(test_sandbox_return_global_capability,
    "Try to return global capability from sandbox",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	void * __capability cret = NULL;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	cret = invoke_return_capability(carg);
	if (cret != carg)
		cheribsdtest_failure_errx("unexpected capability on return");
	cheribsdtest_success();
}

/*
 * Test that if we pass in a global capability, and the sandbox recasts it to
 * be a local capability to return it, that it is not returned.
 */
CHERIBSDTEST(test_sandbox_return_local_capability,
    "Try to return a local capability from a sandbox",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	void * __capability cret = NULL;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	libcheri_errno = 0;
	cret = invoke_return_local_capability(carg);
	if (cret == carg)
		cheribsdtest_failure_errx("local capability returned");
	if (libcheri_errno != LIBCHERI_ERRNO_RETURN_LOCAL_RETVAL)
		cheribsdtest_failure_errx(
		  "returned local capability with unexpected libcheri_errno %d",
		    libcheri_errno);
	cheribsdtest_success();
}

/*
 * Test that if we pass a local capability to a sandbox, CCall rejects the
 * attempt.
 */
CHERIBSDTEST(test_sandbox_pass_local_capability_arg,
    "Try to pass a local capability to a sandbox",
    .ct_flags = CT_FLAG_SANDBOX)
{
	void * __capability carg;
	register_t v;

	carg = (__cheri_tocap void * __capability)&v;
	carg = cheri_local(carg);
	libcheri_errno = 0;
	v = invoke_store_capability_in_stack(carg);
	if (libcheri_errno != LIBCHERI_ERRNO_INVOKE_LOCAL_ARG)
		cheribsdtest_failure_errx(
		    "passed local capability with unexpected libcheri_errno %d",
		    libcheri_errno);
	cheribsdtest_success();
}
