/** $Id$ */
/** @file
 *
 * VBox HDD container test utility, async I/O memory backend
 */

/*
 * Copyright (C) 2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOGGROUP LOGGROUP_DEFAULT /** @todo: Log group */
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/thread.h>
#include <iprt/circbuf.h>
#include <iprt/semaphore.h>

#include "VDMemDisk.h"
#include "VDIoBackendMem.h"

#define VDMEMIOBACKEND_REQS 1024

/**
 * Memory I/O request.
 */
typedef struct VDIOBACKENDREQ
{
    /** I/O request direction. */
    VDIOTXDIR       enmTxDir;
    /** Memory disk handle. */
    PVDMEMDISK      pMemDisk;
    /** Start offset. */
    uint64_t        off;
    /** Size of the transfer. */
    size_t          cbTransfer;
    /** Number of segments in the array. */
    unsigned        cSegs;
    /** Completion handler to call. */
    PFNVDIOCOMPLETE pfnComplete;
    /** Opaque user data. */
    void           *pvUser;
    /** Segment array - variable in size */
    RTSGSEG         aSegs[1];
} VDIOBACKENDREQ, *PVDIOBACKENDREQ;

typedef PVDIOBACKENDREQ *PPVDIOBACKENDREQ;

/**
 * I/O memory backend
 */
typedef struct VDIOBACKENDMEM
{
    /** Thread handle for the backend. */
    RTTHREAD    hThreadIo;
    /** Circular buffer used for submitting requests. */
    PRTCIRCBUF  pRequestRing;
    /** Size of the buffer in request items. */
    unsigned    cReqsRing;
    /** Event semaphore the thread waits on for more work. */
    RTSEMEVENT  EventSem;
    /** Flag whether the server should be still running. */
    volatile bool fRunning;
    /** Number of requests waiting in the request buffer. */
    volatile uint32_t cReqsWaiting;
} VDIOBACKENDMEM;

static int vdIoBackendMemThread(RTTHREAD hThread, void *pvUser);

/**
 * Pokes the I/O thread that something interesting happened.
 *
 * @returns IPRT status code.
 *
 * @param pIoBackend    The backend to poke.
 */
static int vdIoBackendMemThreadPoke(PVDIOBACKENDMEM pIoBackend)
{
    return RTSemEventSignal(pIoBackend->EventSem);
}

int VDIoBackendMemCreate(PPVDIOBACKENDMEM ppIoBackend)
{
    int rc = VINF_SUCCESS;
    PVDIOBACKENDMEM pIoBackend = NULL;

    pIoBackend = (PVDIOBACKENDMEM)RTMemAllocZ(sizeof(VDIOBACKENDMEM));
    if (pIoBackend)
    {
        rc = RTCircBufCreate(&pIoBackend->pRequestRing, VDMEMIOBACKEND_REQS * sizeof(PVDIOBACKENDREQ));
        if (RT_SUCCESS(rc))
        {
            pIoBackend->cReqsRing = VDMEMIOBACKEND_REQS * sizeof(VDIOBACKENDREQ);
            pIoBackend->fRunning  = true;

            rc = RTSemEventCreate(&pIoBackend->EventSem);
            if (RT_SUCCESS(rc))
            {
                rc = RTThreadCreate(&pIoBackend->hThreadIo, vdIoBackendMemThread, pIoBackend, 0, RTTHREADTYPE_IO,
                                    RTTHREADFLAGS_WAITABLE, "MemIo");
                if (RT_SUCCESS(rc))
                {
                    *ppIoBackend = pIoBackend;

                    LogFlowFunc(("returns success\n"));
                    return VINF_SUCCESS;
                }
                RTSemEventDestroy(pIoBackend->EventSem);
            }

            RTCircBufDestroy(pIoBackend->pRequestRing);
        }

        RTMemFree(pIoBackend);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

int VDIoBackendMemDestroy(PVDIOBACKENDMEM pIoBackend)
{
    ASMAtomicXchgBool(&pIoBackend->fRunning, false);
    vdIoBackendMemThreadPoke(pIoBackend);

    RTThreadWait(pIoBackend->hThreadIo, RT_INDEFINITE_WAIT, NULL);
    RTSemEventDestroy(pIoBackend->EventSem);
    RTCircBufDestroy(pIoBackend->pRequestRing);
    RTMemFree(pIoBackend);

    return VINF_SUCCESS;
}

int VDIoBackendMemTransfer(PVDIOBACKENDMEM pIoBackend, PVDMEMDISK pMemDisk,
                           VDIOTXDIR enmTxDir, uint64_t off, size_t cbTransfer, PCRTSGSEG paSegs,
                           unsigned cSegs, PFNVDIOCOMPLETE pfnComplete, void *pvUser)
{
    PVDIOBACKENDREQ pReq = NULL;
    PPVDIOBACKENDREQ ppReq = NULL;
    size_t cbData;

    LogFlowFunc(("Queuing request\n"));

    pReq = (PVDIOBACKENDREQ)RTMemAlloc(RT_OFFSETOF(VDIOBACKENDREQ, aSegs[cSegs]));
    if (!pReq)
        return VERR_NO_MEMORY;

    RTCircBufAcquireWriteBlock(pIoBackend->pRequestRing, sizeof(PVDIOBACKENDREQ), (void **)&ppReq, &cbData);
    if (!pReq)
    {
        RTMemFree(pReq);
        return VERR_NO_MEMORY;
    }

    Assert(cbData == sizeof(PVDIOBACKENDREQ));
    pReq->enmTxDir    = enmTxDir;
    pReq->cbTransfer  = cbTransfer;
    pReq->off         = off;
    pReq->pMemDisk    = pMemDisk;
    pReq->cSegs       = cSegs;
    pReq->pfnComplete = pfnComplete;
    pReq->pvUser      = pvUser;
    for (unsigned i = 0; i < cSegs; i++)
    {
        pReq->aSegs[i].pvSeg = paSegs[i].pvSeg;
        pReq->aSegs[i].cbSeg = paSegs[i].cbSeg;
    }

    *ppReq = pReq;
    RTCircBufReleaseWriteBlock(pIoBackend->pRequestRing, sizeof(PVDIOBACKENDREQ));
    uint32_t cReqsWaiting = ASMAtomicIncU32(&pIoBackend->cReqsWaiting);
    if (cReqsWaiting == 1)
        vdIoBackendMemThreadPoke(pIoBackend);

    return VINF_SUCCESS;
}

/**
 * I/O thread for the memory backend.
 *
 * @returns IPRT status code.
 *
 * @param hThread    The thread handle.
 * @param pvUser     Opaque user data.
 */
static int vdIoBackendMemThread(RTTHREAD hThread, void *pvUser)
{
    PVDIOBACKENDMEM pIoBackend = (PVDIOBACKENDMEM)pvUser;

    while (pIoBackend->fRunning)
    {
        int rc = RTSemEventWait(pIoBackend->EventSem, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(rc) || !pIoBackend->fRunning)
            break;

        PVDIOBACKENDREQ pReq;
        PPVDIOBACKENDREQ ppReq;
        size_t cbData;
        uint32_t cReqsWaiting = ASMAtomicXchgU32(&pIoBackend->cReqsWaiting, 0);

        while (cReqsWaiting)
        {
            int rcReq = VINF_SUCCESS;

            /* Do we have another request? */
            RTCircBufAcquireReadBlock(pIoBackend->pRequestRing, sizeof(PVDIOBACKENDREQ), (void **)&ppReq, &cbData);
            Assert(!ppReq || cbData == sizeof(PVDIOBACKENDREQ));
            RTCircBufReleaseReadBlock(pIoBackend->pRequestRing, cbData);

            pReq = *ppReq;
            cReqsWaiting--;

            LogFlowFunc(("Processing request\n"));
            switch (pReq->enmTxDir)
            {
                case VDIOTXDIR_READ:
                {
                    RTSGBUF SgBuf;
                    RTSgBufInit(&SgBuf, pReq->aSegs, pReq->cSegs);
                    rcReq = VDMemDiskRead(pReq->pMemDisk, pReq->off, pReq->cbTransfer, &SgBuf);
                    break;
                }
                case VDIOTXDIR_WRITE:
                {
                    RTSGBUF SgBuf;
                    RTSgBufInit(&SgBuf, pReq->aSegs, pReq->cSegs);
                    rcReq = VDMemDiskWrite(pReq->pMemDisk, pReq->off, pReq->cbTransfer, &SgBuf);
                    break;
                }
                case VDIOTXDIR_FLUSH:
                    break;
                default:
                    AssertMsgFailed(("Invalid TX direction!\n"));
            }

            /* Notify completion. */
            pReq->pfnComplete(pReq->pvUser, rcReq);
            RTMemFree(pReq);
        }
    }

    return VINF_SUCCESS;
}

