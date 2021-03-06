/* $Id$ */
/** @file
 * IPRT - User & Kernel Memory, Ring-0 Driver, Linux.
 */

/*
 * Copyright (C) 2009 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "the-linux-kernel.h"
#include "internal/iprt.h"

#include <iprt/mem.h>
#include <iprt/err.h>


RTR0DECL(int) RTR0MemUserCopyFrom(void *pvDst, RTR3PTR R3PtrSrc, size_t cb)
{
    if (RT_LIKELY(copy_from_user(pvDst, (void *)R3PtrSrc, cb) == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}
RT_EXPORT_SYMBOL(RTR0MemUserCopyFrom);


RTR0DECL(int) RTR0MemUserCopyTo(RTR3PTR R3PtrDst, void const *pvSrc, size_t cb)
{
    if (RT_LIKELY(copy_to_user((void *)R3PtrDst, pvSrc, cb) == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}
RT_EXPORT_SYMBOL(RTR0MemUserCopyTo);


RTR0DECL(bool) RTR0MemUserIsValidAddr(RTR3PTR R3Ptr)
{
    return access_ok(VERIFY_READ, (void *)R3Ptr, 1);
}
RT_EXPORT_SYMBOL(RTR0MemUserIsValidAddr);


RTR0DECL(bool) RTR0MemKernelIsValidAddr(void *pv)
{
    /* Couldn't find a straight forward way of doing this... */
#ifdef RT_ARCH_X86
# ifdef CONFIG_X86_HIGH_ENTRY
    return true; /* ?? */
# else
    return (uintptr_t)pv >= PAGE_OFFSET;
# endif

#elif defined(RT_ARCH_AMD64)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
    /* Linux 2.6.0 ... 2.6.10 set PAGE_OFFSET to 0x0000010000000000.
     * Linux 2.6.11 sets PAGE_OFFSET to 0xffff810000000000.
     * Linux 2.6.33 sets PAGE_OFFSET to 0xffff880000000000. */
    return (uintptr_t)pv >= PAGE_OFFSET;
# else
    /* Not correct (KERNEL_TEXT_START=0xffffffff80000000),
     * VMALLOC_START (0xffffff0000000000) would be better */
    return (uintptr_t)pv >= KERNEL_TEXT_START;
# endif

#else
# error "PORT ME"
    return !access_ok(VERIFY_READ, pv, 1);
#endif
}
RT_EXPORT_SYMBOL(RTR0MemKernelIsValidAddr);


RTR0DECL(bool) RTR0MemAreKrnlAndUsrDifferent(void)
{
#if defined(RT_ARCH_X86) && defined(CONFIG_X86_HIGH_ENTRY) /* ?? */
    return false;
#else
    return true;
#endif
}
RT_EXPORT_SYMBOL(RTR0MemAreKrnlAndUsrDifferent);


/**
 * Treats both source and destination as unsafe buffers.
 */
static int rtR0MemKernelCopyLnxWorker(void *pvDst, void const *pvSrc, size_t cb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 55)
/* _ASM_EXTABLE was introduced in 2.6.25 from what I can tell. Using #ifndef
   here since it has to be a macro and you never know what someone might have
   backported to an earlier kernel release. */
# ifndef _ASM_EXTABLE
#  if ARCH_BITS == 32
#   define _ASM_EXTABLE(a_Instr, a_Resume) \
    ".section __ex_table,\"a\"\n" \
    ".balign 4\n" \
    ".long   " #a_Instr "\n" \
    ".long   " #a_Resume "\n" \
    ".previous\n"
#  else
#   define _ASM_EXTABLE(a_Instr, a_Resume) \
    ".section __ex_table,\"a\"\n" \
    ".balign 8\n" \
    ".quad   " #a_Instr "\n" \
    ".quad   " #a_Resume "\n" \
    ".previous\n"
#  endif
# endif /* !_ASM_EXTABLE */
    int rc;
    if (!cb)
        return VINF_SUCCESS;

    __asm__ __volatile__ ("cld\n"
                          "1:\n\t"
                          "rep; movsb\n"
                          "2:\n\t"
                          ".section .fixup,\"ax\"\n"
                          "3:\n\t"
                          "movl %4, %0\n"
                          ".previous\n"
                          _ASM_EXTABLE(1b, 3b)
                          : "=r" (rc),
                            "=D" (pvDst),
                            "=S" (pvSrc),
                            "=c" (cb)
                          : "i" (VERR_ACCESS_DENIED),
                            "0" (VINF_SUCCESS),
                            "1" (pvDst),
                            "2" (pvSrc),
                            "3" (cb)
                          : "memory");
    return rc;
#else
    return VERR_NOT_SUPPORTED;
#endif
}


RTR0DECL(int) RTR0MemKernelCopyFrom(void *pvDst, void const *pvSrc, size_t cb)
{
    return rtR0MemKernelCopyLnxWorker(pvDst, pvSrc, cb);
}
RT_EXPORT_SYMBOL(RTR0MemKernelCopyFrom);


RTR0DECL(int) RTR0MemKernelCopyTo(void *pvDst, void const *pvSrc, size_t cb)
{
    return rtR0MemKernelCopyLnxWorker(pvDst, pvSrc, cb);
}
RT_EXPORT_SYMBOL(RTR0MemKernelCopyTo);

