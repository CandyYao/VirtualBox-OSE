/* $Id$ */
/** @file
 * IEM - Instruction Decoding and Emulation.
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


/**
 * Common worker for instructions like ADD, AND, OR, ++ with a byte
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_r8, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R; /* CMP,TEST */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,  pu8Dst,           0);
        IEM_MC_ARG(uint8_t,    u8Src,            1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_MEM_MAP(pu8Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_GREG_U8(u8Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, fAccess);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_rv, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_NO_LOCK_PREFIX();

        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R /* CMP,TEST */;
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *, pu16Dst,          0);
                IEM_MC_ARG(uint16_t,   u16Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U16(u16Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *, pu32Dst,          0);
                IEM_MC_ARG(uint32_t,   u32Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U32(u32Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *, pu64Dst,          0);
                IEM_MC_ARG(uint64_t,   u64Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U64(u64Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for byte instructions like ADD, AND, OR, ++ with a register as
 * the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_r8_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_GREG_U8(pu8Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         */
        IEM_MC_BEGIN(3, 1);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);
        IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_MEM_U8(u8Src, pIemCpu->iEffSeg, GCPtrEffDst);
        IEM_MC_REF_GREG_U8(pu8Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with a
 * register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rv_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U16(pu16Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U32(pu32Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U64(pu64Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're accessing memory.
         */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U16(u16Src, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U16(pu16Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U32(u32Src, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U32(pu32Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U64(u64Src, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U64(pu64Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on AL with
 * a byte immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_AL_Ib, PCIEMOPBINSIZES, pImpl)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
    IEM_MC_ARG_CONST(uint8_t,   u8Src,/*=*/ u8Imm,  1);
    IEM_MC_ARG(uint32_t *,      pEFlags,            2);

    IEM_MC_REF_GREG_U8(pu8Dst, X86_GREG_xAX);
    IEM_MC_REF_EFLAGS(pEFlags);
    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on
 * AX/EAX/RAX with a word/dword immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rAX_Iz, PCIEMOPBINSIZES, pImpl)
{
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
            IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U16(pu16Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
            IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U32(pu32Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
            IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U64(pu64Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcodes 0xf1, 0xd6. */
FNIEMOP_DEF(iemOp_Invalid)
{
    IEMOP_MNEMONIC("Invalid");
    return IEMOP_RAISE_INVALID_OPCODE();
}



/** @name ..... opcodes.
 *
 * @{
 */

/** @}  */


/** @name Two byte opcodes (first byte 0x0f).
 *
 * @{
 */

/** Opcode 0x0f 0x00 /0. */
FNIEMOP_DEF_1(iemOp_Grp6_sldt, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00 /1. */
FNIEMOP_DEF_1(iemOp_Grp6_str, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00 /2. */
FNIEMOP_DEF_1(iemOp_Grp6_lldt, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00 /3. */
FNIEMOP_DEF_1(iemOp_Grp6_ltr, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00 /4. */
FNIEMOP_DEF_1(iemOp_Grp6_verr, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00 /5. */
FNIEMOP_DEF_1(iemOp_Grp6_verw, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x00. */
FNIEMOP_DEF(iemOp_Grp6)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: return FNIEMOP_CALL_1(iemOp_Grp6_sldt, bRm);
        case 1: return FNIEMOP_CALL_1(iemOp_Grp6_str,  bRm);
        case 2: return FNIEMOP_CALL_1(iemOp_Grp6_lldt, bRm);
        case 3: return FNIEMOP_CALL_1(iemOp_Grp6_ltr,  bRm);
        case 4: return FNIEMOP_CALL_1(iemOp_Grp6_verr, bRm);
        case 5: return FNIEMOP_CALL_1(iemOp_Grp6_verw, bRm);
        case 6: return IEMOP_RAISE_INVALID_OPCODE();
        case 7: return IEMOP_RAISE_INVALID_OPCODE();
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

}


/** Opcode 0x0f 0x01 /0. */
FNIEMOP_DEF_1(iemOp_Grp7_sgdt, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /0. */
FNIEMOP_DEF(iemOp_Grp7_vmcall)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /0. */
FNIEMOP_DEF(iemOp_Grp7_vmlaunch)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /0. */
FNIEMOP_DEF(iemOp_Grp7_vmresume)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /0. */
FNIEMOP_DEF(iemOp_Grp7_vmxoff)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /1. */
FNIEMOP_DEF_1(iemOp_Grp7_sidt, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /1. */
FNIEMOP_DEF(iemOp_Grp7_monitor)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /1. */
FNIEMOP_DEF(iemOp_Grp7_mwait)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /2. */
FNIEMOP_DEF_1(iemOp_Grp7_lgdt, uint8_t, bRm)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    IEMMODE enmEffOpSize = pIemCpu->enmCpuMode == IEMMODE_64BIT
                         ? IEMMODE_64BIT
                         : pIemCpu->enmEffOpSize;
    IEM_MC_BEGIN(3, 1);
    IEM_MC_ARG_CONST(uint8_t,   iEffSeg, /*=*/pIemCpu->iEffSeg,     0);
    IEM_MC_ARG(RTGCPTR,         GCPtrEffSrc,                        1);
    IEM_MC_ARG_CONST(IEMMODE,   enmEffOpSizeArg,/*=*/enmEffOpSize,  2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
    IEM_MC_CALL_CIMPL_3(iemCImpl_lgdt, iEffSeg, GCPtrEffSrc, enmEffOpSizeArg);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x01 /2. */
FNIEMOP_DEF(iemOp_Grp7_xgetbv)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /2. */
FNIEMOP_DEF(iemOp_Grp7_xsetbv)
{
    AssertFailed();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Opcode 0x0f 0x01 /3. */
FNIEMOP_DEF_1(iemOp_Grp7_lidt, uint8_t, bRm)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    IEMMODE enmEffOpSize = pIemCpu->enmCpuMode == IEMMODE_64BIT
                         ? IEMMODE_64BIT
                         : pIemCpu->enmEffOpSize;
    IEM_MC_BEGIN(3, 1);
    IEM_MC_ARG_CONST(uint8_t,   iEffSeg, /*=*/pIemCpu->iEffSeg,     0);
    IEM_MC_ARG(RTGCPTR,         GCPtrEffSrc,                        1);
    IEM_MC_ARG_CONST(IEMMODE,   enmEffOpSizeArg,/*=*/enmEffOpSize,  2);
    IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
    IEM_MC_CALL_CIMPL_3(iemCImpl_lidt, iEffSeg, GCPtrEffSrc, enmEffOpSizeArg);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x01 /4. */
FNIEMOP_DEF_1(iemOp_Grp7_smsw, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /6. */
FNIEMOP_DEF_1(iemOp_Grp7_lmsw, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /7. */
FNIEMOP_DEF_1(iemOp_Grp7_invlpg, uint8_t, bRm)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /7. */
FNIEMOP_DEF(iemOp_Grp7_swapgs)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01 /7. */
FNIEMOP_DEF(iemOp_Grp7_rdtscp)
{
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0x0f 0x01. */
FNIEMOP_DEF(iemOp_Grp7)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0:
            if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
                return FNIEMOP_CALL_1(iemOp_Grp7_sgdt, bRm);
            switch (bRm & X86_MODRM_RM_MASK)
            {
                case 1: return FNIEMOP_CALL(iemOp_Grp7_vmcall);
                case 2: return FNIEMOP_CALL(iemOp_Grp7_vmlaunch);
                case 3: return FNIEMOP_CALL(iemOp_Grp7_vmresume);
                case 4: return FNIEMOP_CALL(iemOp_Grp7_vmxoff);
            }
            return IEMOP_RAISE_INVALID_OPCODE();

        case 1:
            if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
                return FNIEMOP_CALL_1(iemOp_Grp7_sidt, bRm);
            switch (bRm & X86_MODRM_RM_MASK)
            {
                case 0: return FNIEMOP_CALL(iemOp_Grp7_monitor);
                case 1: return FNIEMOP_CALL(iemOp_Grp7_mwait);
            }
            return IEMOP_RAISE_INVALID_OPCODE();

        case 2:
            if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
                return FNIEMOP_CALL_1(iemOp_Grp7_lgdt, bRm);
            switch (bRm & X86_MODRM_RM_MASK)
            {
                case 0: return FNIEMOP_CALL(iemOp_Grp7_xgetbv);
                case 1: return FNIEMOP_CALL(iemOp_Grp7_xsetbv);
            }
            return IEMOP_RAISE_INVALID_OPCODE();

        case 3:
            if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
                return FNIEMOP_CALL_1(iemOp_Grp7_lidt, bRm);
            return IEMOP_RAISE_INVALID_OPCODE();

        case 4:
            return FNIEMOP_CALL_1(iemOp_Grp7_smsw, bRm);

        case 5:
            return IEMOP_RAISE_INVALID_OPCODE();

        case 6:
            return FNIEMOP_CALL_1(iemOp_Grp7_lmsw, bRm);

        case 7:
            if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
                return FNIEMOP_CALL_1(iemOp_Grp7_invlpg, bRm);
            switch (bRm & X86_MODRM_RM_MASK)
            {
                case 0: return FNIEMOP_CALL(iemOp_Grp7_swapgs);
                case 1: return FNIEMOP_CALL(iemOp_Grp7_rdtscp);
            }
            return IEMOP_RAISE_INVALID_OPCODE();

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0x0f 0x02. */
FNIEMOP_STUB(iemOp_lar_Gv_Ew);
/** Opcode 0x0f 0x03. */
FNIEMOP_STUB(iemOp_lsl_Gv_Ew);
/** Opcode 0x0f 0x04. */
FNIEMOP_STUB(iemOp_syscall);
/** Opcode 0x0f 0x05. */
FNIEMOP_STUB(iemOp_clts);
/** Opcode 0x0f 0x06. */
FNIEMOP_STUB(iemOp_sysret);
/** Opcode 0x0f 0x08. */
FNIEMOP_STUB(iemOp_invd);
/** Opcode 0x0f 0x09. */
FNIEMOP_STUB(iemOp_wbinvd);
/** Opcode 0x0f 0x0b. */
FNIEMOP_STUB(iemOp_ud2);
/** Opcode 0x0f 0x0d. */
FNIEMOP_STUB(iemOp_nop_Ev_prefetch);
/** Opcode 0x0f 0x0e. */
FNIEMOP_STUB(iemOp_femms);
/** Opcode 0x0f 0x0f. */
FNIEMOP_STUB(iemOp_3Dnow);
/** Opcode 0x0f 0x10. */
FNIEMOP_STUB(iemOp_movups_Vps_Wps__movupd_Vpd_Wpd__movss_Vss_Wss__movsd_Vsd_Wsd);
/** Opcode 0x0f 0x11. */
FNIEMOP_STUB(iemOp_movups_Wps_Vps__movupd_Wpd_Vpd__movss_Wss_Vss__movsd_Vsd_Wsd);
/** Opcode 0x0f 0x12. */
FNIEMOP_STUB(iemOp_movlps_Vq_Mq__movhlps_Vq_Uq__movlpd_Vq_Mq__movsldup_Vq_Wq__movddup_Vq_Wq);
/** Opcode 0x0f 0x13. */
FNIEMOP_STUB(iemOp_movlps_Mq_Vq__movlpd_Mq_Vq);
/** Opcode 0x0f 0x14. */
FNIEMOP_STUB(iemOp_unpckhlps_Vps_Wq__unpcklpd_Vpd_Wq);
/** Opcode 0x0f 0x15. */
FNIEMOP_STUB(iemOp_unpckhps_Vps_Wq__unpckhpd_Vpd_Wq);
/** Opcode 0x0f 0x16. */
FNIEMOP_STUB(iemOp_movhps_Vq_Mq__movlhps_Vq_Uq__movhpd_Vq_Mq__movshdup_Vq_Wq);
/** Opcode 0x0f 0x17. */
FNIEMOP_STUB(iemOp_movhps_Mq_Vq__movhpd_Mq_Vq);
/** Opcode 0x0f 0x18. */
FNIEMOP_STUB(iemOp_prefetch_Grp16);


/** Opcode 0x0f 0x20. */
FNIEMOP_DEF(iemOp_mov_Rd_Cd)
{
    /* mod is ignored, as is operand size overrides. */
    IEMOP_MNEMONIC("mov Rd,Cd");
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
        pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_64BIT;
    else
        pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_32BIT;

    /** @todo Verify that the the invalid lock sequence exception (\#UD) is raised
     *        before the privilege level violation (\#GP). */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    uint8_t iCrReg = ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg;
    if (pIemCpu->fPrefixes & IEM_OP_PRF_LOCK)
    {
        /* The lock prefix can be used to encode CR8 accesses on some CPUs. */
        if (!IEM_IS_AMD_CPUID_FEATURE_PRESENT_ECX(X86_CPUID_AMD_FEATURE_ECX_CR8L))
            return IEMOP_RAISE_INVALID_LOCK_PREFIX();
        iCrReg |= 8;
    }
    switch (iCrReg)
    {
        case 0: case 2: case 3: case 4: case 8:
            break;
        default:
            return IEMOP_RAISE_INVALID_OPCODE();
    }

    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_mov_Rd_Cd, (X86_MODRM_RM_MASK & bRm) | pIemCpu->uRexB, iCrReg);
}


/** Opcode 0x0f 0x21. */
FNIEMOP_STUB(iemOp_mov_Rd_Dd);


/** Opcode 0x0f 0x22. */
FNIEMOP_DEF(iemOp_mov_Cd_Rd)
{
    /* mod is ignored, as is operand size overrides. */
    IEMOP_MNEMONIC("mov Cd,Rd");
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
        pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_64BIT;
    else
        pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_32BIT;

    /** @todo Verify that the the invalid lock sequence exception (\#UD) is raised
     *        before the privilege level violation (\#GP). */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    uint8_t iCrReg = ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg;
    if (pIemCpu->fPrefixes & IEM_OP_PRF_LOCK)
    {
        /* The lock prefix can be used to encode CR8 accesses on some CPUs. */
        if (!IEM_IS_AMD_CPUID_FEATURE_PRESENT_ECX(X86_CPUID_AMD_FEATURE_ECX_CR8L))
            return IEMOP_RAISE_INVALID_LOCK_PREFIX();
        iCrReg |= 8;
    }
    switch (iCrReg)
    {
        case 0: case 2: case 3: case 4: case 8:
            break;
        default:
            return IEMOP_RAISE_INVALID_OPCODE();
    }

    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_mov_Cd_Rd, iCrReg, (X86_MODRM_RM_MASK & bRm) | pIemCpu->uRexB);
}


/** Opcode 0x0f 0x23. */
FNIEMOP_STUB(iemOp_mov_Dd_Rd);
/** Opcode 0x0f 0x24. */
FNIEMOP_STUB(iemOp_mov_Rd_Td);
/** Opcode 0x0f 0x26. */
FNIEMOP_STUB(iemOp_mov_Td_Rd);
/** Opcode 0x0f 0x28. */
FNIEMOP_STUB(iemOp_movaps_Vps_Wps__movapd_Vpd_Wpd);
/** Opcode 0x0f 0x29. */
FNIEMOP_STUB(iemOp_movaps_Wps_Vps__movapd_Wpd_Vpd);
/** Opcode 0x0f 0x2a. */
FNIEMOP_STUB(iemOp_cvtpi2ps_Vps_Qpi__cvtpi2pd_Vpd_Qpi__cvtsi2ss_Vss_Ey__cvtsi2sd_Vsd_Ey);
/** Opcode 0x0f 0x2b. */
FNIEMOP_STUB(iemOp_movntps_Mps_Vps__movntpd_Mpd_Vpd);
/** Opcode 0x0f 0x2c. */
FNIEMOP_STUB(iemOp_cvttps2pi_Ppi_Wps__cvttpd2pi_Ppi_Wpd__cvttss2si_Gy_Wss__cvttsd2si_Yu_Wsd);
/** Opcode 0x0f 0x2d. */
FNIEMOP_STUB(iemOp_cvtps2pi_Ppi_Wps__cvtpd2pi_QpiWpd__cvtss2si_Gy_Wss__cvtsd2si_Gy_Wsd);
/** Opcode 0x0f 0x2e. */
FNIEMOP_STUB(iemOp_ucomiss_Vss_Wss__ucomisd_Vsd_Wsd);
/** Opcode 0x0f 0x2f. */
FNIEMOP_STUB(iemOp_comiss_Vss_Wss__comisd_Vsd_Wsd);
/** Opcode 0x0f 0x30. */
FNIEMOP_STUB(iemOp_wrmsr);
/** Opcode 0x0f 0x31. */
FNIEMOP_STUB(iemOp_rdtsc);
/** Opcode 0x0f 0x33. */
FNIEMOP_STUB(iemOp_rdmsr);
/** Opcode 0x0f 0x34. */
FNIEMOP_STUB(iemOp_rdpmc);
/** Opcode 0x0f 0x34. */
FNIEMOP_STUB(iemOp_sysenter);
/** Opcode 0x0f 0x35. */
FNIEMOP_STUB(iemOp_sysexit);
/** Opcode 0x0f 0x37. */
FNIEMOP_STUB(iemOp_getsec);
/** Opcode 0x0f 0x38. */
FNIEMOP_STUB(iemOp_3byte_Esc_A4);
/** Opcode 0x0f 0x39. */
FNIEMOP_STUB(iemOp_3byte_Esc_A5);
/** Opcode 0x0f 0x3c (?). */
FNIEMOP_STUB(iemOp_movnti_Gv_Ev);
/** Opcode 0x0f 0x40. */
FNIEMOP_STUB(iemOp_cmovo_Gv_Ev);
/** Opcode 0x0f 0x41. */
FNIEMOP_STUB(iemOp_cmovno_Gv_Ev);
/** Opcode 0x0f 0x42. */
FNIEMOP_STUB(iemOp_cmovc_Gv_Ev);
/** Opcode 0x0f 0x43. */
FNIEMOP_STUB(iemOp_cmovnc_Gv_Ev);
/** Opcode 0x0f 0x44. */
FNIEMOP_STUB(iemOp_cmove_Gv_Ev);
/** Opcode 0x0f 0x45. */
FNIEMOP_STUB(iemOp_cmovne_Gv_Ev);
/** Opcode 0x0f 0x46. */
FNIEMOP_STUB(iemOp_cmovbe_Gv_Ev);
/** Opcode 0x0f 0x47. */
FNIEMOP_STUB(iemOp_cmovnbe_Gv_Ev);
/** Opcode 0x0f 0x48. */
FNIEMOP_STUB(iemOp_cmovs_Gv_Ev);
/** Opcode 0x0f 0x49. */
FNIEMOP_STUB(iemOp_cmovns_Gv_Ev);
/** Opcode 0x0f 0x4a. */
FNIEMOP_STUB(iemOp_cmovp_Gv_Ev);
/** Opcode 0x0f 0x4b. */
FNIEMOP_STUB(iemOp_cmovnp_Gv_Ev);
/** Opcode 0x0f 0x4c. */
FNIEMOP_STUB(iemOp_cmovl_Gv_Ev);
/** Opcode 0x0f 0x4d. */
FNIEMOP_STUB(iemOp_cmovnl_Gv_Ev);
/** Opcode 0x0f 0x4e. */
FNIEMOP_STUB(iemOp_cmovle_Gv_Ev);
/** Opcode 0x0f 0x4f. */
FNIEMOP_STUB(iemOp_cmovnle_Gv_Ev);
/** Opcode 0x0f 0x50. */
FNIEMOP_STUB(iemOp_movmskps_Gy_Ups__movmskpd_Gy_Upd);
/** Opcode 0x0f 0x51. */
FNIEMOP_STUB(iemOp_sqrtps_Wps_Vps__sqrtpd_Wpd_Vpd__sqrtss_Vss_Wss__sqrtsd_Vsd_Wsd);
/** Opcode 0x0f 0x52. */
FNIEMOP_STUB(iemOp_rsqrtps_Wps_Vps__rsqrtss_Vss_Wss);
/** Opcode 0x0f 0x53. */
FNIEMOP_STUB(iemOp_rcpps_Wps_Vps__rcpss_Vs_Wss);
/** Opcode 0x0f 0x54. */
FNIEMOP_STUB(iemOp_andps_Vps_Wps__andpd_Wpd_Vpd);
/** Opcode 0x0f 0x55. */
FNIEMOP_STUB(iemOp_andnps_Vps_Wps__andnpd_Wpd_Vpd);
/** Opcode 0x0f 0x56. */
FNIEMOP_STUB(iemOp_orps_Wpd_Vpd__orpd_Wpd_Vpd);
/** Opcode 0x0f 0x57. */
FNIEMOP_STUB(iemOp_xorps_Vps_Wps__xorpd_Wpd_Vpd);
/** Opcode 0x0f 0x58. */
FNIEMOP_STUB(iemOp_addps_Vps_Wps__addpd_Vpd_Wpd__addss_Vss_Wss__addsd_Vsd_Wsd);
/** Opcode 0x0f 0x59. */
FNIEMOP_STUB(iemOp_mulps_Vps_Wps__mulpd_Vpd_Wpd__mulss_Vss__Wss__mulsd_Vsd_Wsd);
/** Opcode 0x0f 0x5a. */
FNIEMOP_STUB(iemOp_cvtps2pd_Vpd_Wps__cvtpd2ps_Vps_Wpd__cvtss2sd_Vsd_Wss__cvtsd2ss_Vss_Wsd);
/** Opcode 0x0f 0x5b. */
FNIEMOP_STUB(iemOp_cvtdq2ps_Vps_Wdq__cvtps2dq_Vdq_Wps__cvtps2dq_Vdq_Wps);
/** Opcode 0x0f 0x5c. */
FNIEMOP_STUB(iemOp_subps_Vps_Wps__subpd_Vps_Wdp__subss_Vss_Wss__subsd_Vsd_Wsd);
/** Opcode 0x0f 0x5d. */
FNIEMOP_STUB(iemOp_minps_Vps_Wps__minpd_Vpd_Wpd__minss_Vss_Wss__minsd_Vsd_Wsd);
/** Opcode 0x0f 0x5e. */
FNIEMOP_STUB(iemOp_divps_Vps_Wps__divpd_Vpd_Wpd__divss_Vss_Wss__divsd_Vsd_Wsd);
/** Opcode 0x0f 0x5f. */
FNIEMOP_STUB(iemOp_maxps_Vps_Wps__maxpd_Vpd_Wpd__maxss_Vss_Wss__maxsd_Vsd_Wsd);
/** Opcode 0x0f 0x60. */
FNIEMOP_STUB(iemOp_punpcklbw_Pq_Qd__punpcklbw_Vdq_Wdq);
/** Opcode 0x0f 0x61. */
FNIEMOP_STUB(iemOp_punpcklwd_Pq_Qd__punpcklwd_Vdq_Wdq);
/** Opcode 0x0f 0x62. */
FNIEMOP_STUB(iemOp_punpckldq_Pq_Qd__punpckldq_Vdq_Wdq);
/** Opcode 0x0f 0x63. */
FNIEMOP_STUB(iemOp_packsswb_Pq_Qq__packsswb_Vdq_Wdq);
/** Opcode 0x0f 0x64. */
FNIEMOP_STUB(iemOp_pcmpgtb_Pq_Qq__pcmpgtb_Vdq_Wdq);
/** Opcode 0x0f 0x65. */
FNIEMOP_STUB(iemOp_pcmpgtw_Pq_Qq__pcmpgtw_Vdq_Wdq);
/** Opcode 0x0f 0x66. */
FNIEMOP_STUB(iemOp_pcmpgtd_Pq_Qq__pcmpgtd_Vdq_Wdq);
/** Opcode 0x0f 0x67. */
FNIEMOP_STUB(iemOp_packuswb_Pq_Qq__packuswb_Vdq_Wdq);
/** Opcode 0x0f 0x68. */
FNIEMOP_STUB(iemOp_punpckhbw_Pq_Qq__punpckhbw_Vdq_Wdq);
/** Opcode 0x0f 0x69. */
FNIEMOP_STUB(iemOp_punpckhwd_Pq_Qd__punpckhwd_Vdq_Wdq);
/** Opcode 0x0f 0x6a. */
FNIEMOP_STUB(iemOp_punpckhdq_Pq_Qd__punpckhdq_Vdq_Wdq);
/** Opcode 0x0f 0x6b. */
FNIEMOP_STUB(iemOp_packssdw_Pq_Qd__packssdq_Vdq_Wdq);
/** Opcode 0x0f 0x6c. */
FNIEMOP_STUB(iemOp_punpcklqdq_Vdq_Wdq);
/** Opcode 0x0f 0x6d. */
FNIEMOP_STUB(iemOp_punpckhqdq_Vdq_Wdq);
/** Opcode 0x0f 0x6e. */
FNIEMOP_STUB(iemOp_movd_q_Pd_Ey__movd_q_Vy_Ey);
/** Opcode 0x0f 0x6f. */
FNIEMOP_STUB(iemOp_movq_Pq_Qq__movdqa_Vdq_Wdq__movdqu_Vdq_Wdq);
/** Opcode 0x0f 0x70. */
FNIEMOP_STUB(iemOp_pshufw_Pq_Qq_Ib__pshufd_Vdq_Wdq_Ib__pshufhw_Vdq_Wdq_Ib__pshuflq_Vdq_Wdq_Ib);
/** Opcode 0x0f 0x71. */
FNIEMOP_STUB(iemOp_Grp12);
/** Opcode 0x0f 0x72. */
FNIEMOP_STUB(iemOp_Grp13);
/** Opcode 0x0f 0x73. */
FNIEMOP_STUB(iemOp_Grp14);
/** Opcode 0x0f 0x74. */
FNIEMOP_STUB(iemOp_pcmpeqb_Pq_Qq__pcmpeqb_Vdq_Wdq);
/** Opcode 0x0f 0x75. */
FNIEMOP_STUB(iemOp_pcmpeqw_Pq_Qq__pcmpeqw_Vdq_Wdq);
/** Opcode 0x0f 0x76. */
FNIEMOP_STUB(iemOp_pcmped_Pq_Qq__pcmpeqd_Vdq_Wdq);
/** Opcode 0x0f 0x77. */
FNIEMOP_STUB(iemOp_emms);
/** Opcode 0x0f 0x78. */
FNIEMOP_STUB(iemOp_vmread);
/** Opcode 0x0f 0x79. */
FNIEMOP_STUB(iemOp_vmwrite);
/** Opcode 0x0f 0x7c. */
FNIEMOP_STUB(iemOp_haddpd_Vdp_Wpd__haddps_Vps_Wps);
/** Opcode 0x0f 0x7d. */
FNIEMOP_STUB(iemOp_hsubpd_Vpd_Wpd__hsubps_Vps_Wps);
/** Opcode 0x0f 0x7e. */
FNIEMOP_STUB(iemOp_movd_q_Ey_Pd__movd_q_Ey_Vy__movq_Vq_Wq);
/** Opcode 0x0f 0x7f. */
FNIEMOP_STUB(iemOp_movq_Qq_Pq__movq_movdqa_Wdq_Vdq__movdqu_Wdq_Vdq);


/** Opcode 0x0f 0x80. */
FNIEMOP_DEF(iemOp_jo_Jv)
{
    IEMOP_MNEMONIC("jo  Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x81. */
FNIEMOP_DEF(iemOp_jno_Jv)
{
    IEMOP_MNEMONIC("jno Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x82. */
FNIEMOP_DEF(iemOp_jc_Jv)
{
    IEMOP_MNEMONIC("jc/jb/jnae Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x83. */
FNIEMOP_DEF(iemOp_jnc_Jv)
{
    IEMOP_MNEMONIC("jnc/jnb/jae Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x84. */
FNIEMOP_DEF(iemOp_je_Jv)
{
    IEMOP_MNEMONIC("je/jz Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x85. */
FNIEMOP_DEF(iemOp_jne_Jv)
{
    IEMOP_MNEMONIC("jne/jnz Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x86. */
FNIEMOP_DEF(iemOp_jbe_Jv)
{
    IEMOP_MNEMONIC("jbe/jna Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x87. */
FNIEMOP_DEF(iemOp_jnbe_Jv)
{
    IEMOP_MNEMONIC("jnbe/ja Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x88. */
FNIEMOP_DEF(iemOp_js_Jv)
{
    IEMOP_MNEMONIC("js  Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x89. */
FNIEMOP_DEF(iemOp_jns_Jv)
{
    IEMOP_MNEMONIC("jns Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8a. */
FNIEMOP_DEF(iemOp_jp_Jv)
{
    IEMOP_MNEMONIC("jp  Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8b. */
FNIEMOP_DEF(iemOp_jnp_Jv)
{
    IEMOP_MNEMONIC("jo  Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8c. */
FNIEMOP_DEF(iemOp_jl_Jv)
{
    IEMOP_MNEMONIC("jl/jnge Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8d. */
FNIEMOP_DEF(iemOp_jnl_Jv)
{
    IEMOP_MNEMONIC("jnl/jge Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8e. */
FNIEMOP_DEF(iemOp_jle_Jv)
{
    IEMOP_MNEMONIC("jle/jng Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ELSE() {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x8f. */
FNIEMOP_DEF(iemOp_jnle_Jv)
{
    IEMOP_MNEMONIC("jnle/jg Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
    {
        uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    else
    {
        uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(0, 0);
        IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
            IEM_MC_ADVANCE_RIP();
        } IEM_MC_ELSE() {
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
        } IEM_MC_ENDIF();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x0f 0x90. */
FNIEMOP_STUB(iemOp_seto_Jv);
/** Opcode 0x0f 0x91. */
FNIEMOP_STUB(iemOp_setno_Jv);
/** Opcode 0x0f 0x92. */
FNIEMOP_STUB(iemOp_setc_Jv);
/** Opcode 0x0f 0x93. */
FNIEMOP_STUB(iemOp_setnc_Jv);
/** Opcode 0x0f 0x94. */
FNIEMOP_STUB(iemOp_sete_Jv);
/** Opcode 0x0f 0x95. */
FNIEMOP_STUB(iemOp_setne_Jv);
/** Opcode 0x0f 0x96. */
FNIEMOP_STUB(iemOp_setbe_Jv);
/** Opcode 0x0f 0x97. */
FNIEMOP_STUB(iemOp_setnbe_Jv);
/** Opcode 0x0f 0x98. */
FNIEMOP_STUB(iemOp_sets_Jv);
/** Opcode 0x0f 0x99. */
FNIEMOP_STUB(iemOp_setns_Jv);
/** Opcode 0x0f 0x9a. */
FNIEMOP_STUB(iemOp_setp_Jv);
/** Opcode 0x0f 0x9b. */
FNIEMOP_STUB(iemOp_setnp_Jv);
/** Opcode 0x0f 0x9c. */
FNIEMOP_STUB(iemOp_setl_Jv);
/** Opcode 0x0f 0x9d. */
FNIEMOP_STUB(iemOp_setnl_Jv);
/** Opcode 0x0f 0x9e. */
FNIEMOP_STUB(iemOp_setle_Jv);
/** Opcode 0x0f 0x9f. */
FNIEMOP_STUB(iemOp_setnle_Jv);
/** Opcode 0x0f 0xa0. */
FNIEMOP_STUB(iemOp_push_fs);
/** Opcode 0x0f 0xa1. */
FNIEMOP_STUB(iemOp_pop_fs);
/** Opcode 0x0f 0xa2. */
FNIEMOP_STUB(iemOp_cpuid);
/** Opcode 0x0f 0xa3. */
FNIEMOP_STUB(iemOp_bt_Ev_Gv);
/** Opcode 0x0f 0xa4. */
FNIEMOP_STUB(iemOp_shld_Ev_Gv_Ib);
/** Opcode 0x0f 0xa7. */
FNIEMOP_STUB(iemOp_shld_Ev_Gv_CL);
/** Opcode 0x0f 0xa8. */
FNIEMOP_STUB(iemOp_push_gs);
/** Opcode 0x0f 0xa9. */
FNIEMOP_STUB(iemOp_pop_gs);
/** Opcode 0x0f 0xaa. */
FNIEMOP_STUB(iemOp_rsm);
/** Opcode 0x0f 0xab. */
FNIEMOP_STUB(iemOp_bts_Ev_Gv);
/** Opcode 0x0f 0xac. */
FNIEMOP_STUB(iemOp_shrd_Ev_Gv_Ib);
/** Opcode 0x0f 0xad. */
FNIEMOP_STUB(iemOp_shrd_Ev_Gv_CL);
/** Opcode 0x0f 0xae. */
FNIEMOP_STUB(iemOp_Grp15);
/** Opcode 0x0f 0xaf. */
FNIEMOP_STUB(iemOp_imul_Gv_Ev);
/** Opcode 0x0f 0xb0. */
FNIEMOP_STUB(iemOp_cmpxchg_Eb_Gb);
/** Opcode 0x0f 0xb1. */
FNIEMOP_STUB(iemOp_cmpxchg_Ev_Gv);
/** Opcode 0x0f 0xb2. */
FNIEMOP_STUB(iemOp_lss_Gv_Mp);
/** Opcode 0x0f 0xb3. */
FNIEMOP_STUB(iemOp_btr_Ev_Gv);
/** Opcode 0x0f 0xb4. */
FNIEMOP_STUB(iemOp_lfs_Gv_Mp);
/** Opcode 0x0f 0xb5. */
FNIEMOP_STUB(iemOp_lgs_Gv_Mp);
/** Opcode 0x0f 0xb6. */
FNIEMOP_STUB(iemOp_movzx_Gv_Eb);
/** Opcode 0x0f 0xb7. */
FNIEMOP_STUB(iemOp_movzx_Gv_Ew);
/** Opcode 0x0f 0xb8. */
FNIEMOP_STUB(iemOp_popcnt_Gv_Ev_jmpe);
/** Opcode 0x0f 0xb9. */
FNIEMOP_STUB(iemOp_Grp10);
/** Opcode 0x0f 0xba. */
FNIEMOP_STUB(iemOp_Grp11);
/** Opcode 0x0f 0xbb. */
FNIEMOP_STUB(iemOp_btc_Ev_Gv);
/** Opcode 0x0f 0xbc. */
FNIEMOP_STUB(iemOp_bsf_Gv_Ev);
/** Opcode 0x0f 0xbd. */
FNIEMOP_STUB(iemOp_bsr_Gv_Ev);
/** Opcode 0x0f 0xbe. */
FNIEMOP_STUB(iemOp_movsx_Gv_Eb);
/** Opcode 0x0f 0xbf. */
FNIEMOP_STUB(iemOp_movsx_Gv_Ew);
/** Opcode 0x0f 0xc0. */
FNIEMOP_STUB(iemOp_xadd_Eb_Gb);
/** Opcode 0x0f 0xc1. */
FNIEMOP_STUB(iemOp_xadd_Ev_Gv);
/** Opcode 0x0f 0xc2. */
FNIEMOP_STUB(iemOp_cmpps_Vps_Wps_Ib__cmppd_Vpd_Wpd_Ib__cmpss_Vss_Wss_Ib__cmpsd_Vsd_Wsd_Ib);
/** Opcode 0x0f 0xc3. */
FNIEMOP_STUB(iemOp_movnti_My_Gy);
/** Opcode 0x0f 0xc4. */
FNIEMOP_STUB(iemOp_pinsrw_Pq_Ry_Mw_Ib__pinsrw_Vdq_Ry_Mw_Ib);
/** Opcode 0x0f 0xc5. */
FNIEMOP_STUB(iemOp_pextrw_Gd_Nq_Ib__pextrw_Gd_Udq_Ib);
/** Opcode 0x0f 0xc6. */
FNIEMOP_STUB(iemOp_shufps_Vps_Wps_Ib__shufdp_Vpd_Wpd_Ib);
/** Opcode 0x0f 0xc7. */
FNIEMOP_STUB(iemOp_Grp9);
/** Opcode 0x0f 0xc8. */
FNIEMOP_STUB(iemOp_bswap_rAX_r8);
/** Opcode 0x0f 0xc9. */
FNIEMOP_STUB(iemOp_bswap_rCX_r9);
/** Opcode 0x0f 0xca. */
FNIEMOP_STUB(iemOp_bswap_rDX_r10);
/** Opcode 0x0f 0xcb. */
FNIEMOP_STUB(iemOp_bswap_rBX_r11);
/** Opcode 0x0f 0xcc. */
FNIEMOP_STUB(iemOp_bswap_rSP_r12);
/** Opcode 0x0f 0xcd. */
FNIEMOP_STUB(iemOp_bswap_rBP_r13);
/** Opcode 0x0f 0xce. */
FNIEMOP_STUB(iemOp_bswap_rSI_r14);
/** Opcode 0x0f 0xcf. */
FNIEMOP_STUB(iemOp_bswap_rDI_r15);
/** Opcode 0x0f 0xd0. */
FNIEMOP_STUB(iemOp_addsubpd_Vpd_Wpd__addsubps_Vps_Wps);
/** Opcode 0x0f 0xd1. */
FNIEMOP_STUB(iemOp_psrlw_Pp_Qp__psrlw_Vdp_Wdq);
/** Opcode 0x0f 0xd2. */
FNIEMOP_STUB(iemOp_psrld_Pq_Qq__psrld_Vdq_Wdq);
/** Opcode 0x0f 0xd3. */
FNIEMOP_STUB(iemOp_psrlq_Pq_Qq__psrlq_Vdq_Wdq);
/** Opcode 0x0f 0xd4. */
FNIEMOP_STUB(iemOp_paddq_Pq_Qq__paddq_Vdq_Wdq);
/** Opcode 0x0f 0xd5. */
FNIEMOP_STUB(iemOp_pmulq_Pq_Qq__pmullw_Vdq_Wdq);
/** Opcode 0x0f 0xd6. */
FNIEMOP_STUB(iemOp_movq_Wq_Vq__movq2dq_Vdq_Nq__movdq2q_Pq_Uq);
/** Opcode 0x0f 0xd7. */
FNIEMOP_STUB(iemOp_pmovmskb_Gd_Nq__pmovmskb_Gd_Udq);
/** Opcode 0x0f 0xd8. */
FNIEMOP_STUB(iemOp_psubusb_Pq_Qq__psubusb_Vdq_Wdq);
/** Opcode 0x0f 0xd9. */
FNIEMOP_STUB(iemOp_psubusw_Pq_Qq__psubusw_Vdq_Wdq);
/** Opcode 0x0f 0xda. */
FNIEMOP_STUB(iemOp_pminub_Pq_Qq__pminub_Vdq_Wdq);
/** Opcode 0x0f 0xdb. */
FNIEMOP_STUB(iemOp_pand_Pq_Qq__pand_Vdq_Wdq);
/** Opcode 0x0f 0xdc. */
FNIEMOP_STUB(iemOp_paddusb_Pq_Qq__paddusb_Vdq_Wdq);
/** Opcode 0x0f 0xdd. */
FNIEMOP_STUB(iemOp_paddusw_Pq_Qq__paddusw_Vdq_Wdq);
/** Opcode 0x0f 0xde. */
FNIEMOP_STUB(iemOp_pmaxub_Pq_Qq__pamxub_Vdq_Wdq);
/** Opcode 0x0f 0xdf. */
FNIEMOP_STUB(iemOp_pandn_Pq_Qq__pandn_Vdq_Wdq);
/** Opcode 0x0f 0xe0. */
FNIEMOP_STUB(iemOp_pavgb_Pq_Qq__pavgb_Vdq_Wdq);
/** Opcode 0x0f 0xe1. */
FNIEMOP_STUB(iemOp_psraw_Pq_Qq__psraw_Vdq_Wdq);
/** Opcode 0x0f 0xe2. */
FNIEMOP_STUB(iemOp_psrad_Pq_Qq__psrad_Vdq_Wdq);
/** Opcode 0x0f 0xe3. */
FNIEMOP_STUB(iemOp_pavgw_Pq_Qq__pavgw_Vdq_Wdq);
/** Opcode 0x0f 0xe4. */
FNIEMOP_STUB(iemOp_pmulhuw_Pq_Qq__pmulhuw_Vdq_Wdq);
/** Opcode 0x0f 0xe5. */
FNIEMOP_STUB(iemOp_pmulhw_Pq_Qq__pmulhw_Vdq_Wdq);
/** Opcode 0x0f 0xe6. */
FNIEMOP_STUB(iemOp_cvttpd2dq_Vdq_Wdp__cvtdq2pd_Vdq_Wpd__cvtpd2dq_Vdq_Wpd);
/** Opcode 0x0f 0xe7. */
FNIEMOP_STUB(iemOp_movntq_Mq_Pq__movntdq_Mdq_Vdq);
/** Opcode 0x0f 0xe8. */
FNIEMOP_STUB(iemOp_psubsb_Pq_Qq__psubsb_Vdq_Wdq);
/** Opcode 0x0f 0xe9. */
FNIEMOP_STUB(iemOp_psubsw_Pq_Qq__psubsw_Vdq_Wdq);
/** Opcode 0x0f 0xea. */
FNIEMOP_STUB(iemOp_pminsw_Pq_Qq__pminsw_Vdq_Wdq);
/** Opcode 0x0f 0xeb. */
FNIEMOP_STUB(iemOp_por_Pq_Qq__por_Vdq_Wdq);
/** Opcode 0x0f 0xec. */
FNIEMOP_STUB(iemOp_paddsb_Pq_Qq__paddsb_Vdq_Wdq);
/** Opcode 0x0f 0xed. */
FNIEMOP_STUB(iemOp_paddsw_Pq_Qq__paddsw_Vdq_Wdq);
/** Opcode 0x0f 0xee. */
FNIEMOP_STUB(iemOp_pmaxsw_Pq_Qq__pmaxsw_Vdq_Wdq);
/** Opcode 0x0f 0xef. */
FNIEMOP_STUB(iemOp_pxor_Pq_Qq__pxor_Vdq_Wdq);
/** Opcode 0x0f 0xf0. */
FNIEMOP_STUB(iemOp_lddqu_Vdq_Mdq);
/** Opcode 0x0f 0xf1. */
FNIEMOP_STUB(iemOp_psllw_Pq_Qq__pslw_Vdq_Wdq);
/** Opcode 0x0f 0xf2. */
FNIEMOP_STUB(iemOp_psld_Pq_Qq__pslld_Vdq_Wdq);
/** Opcode 0x0f 0xf3. */
FNIEMOP_STUB(iemOp_psllq_Pq_Qq__pslq_Vdq_Wdq);
/** Opcode 0x0f 0xf4. */
FNIEMOP_STUB(iemOp_pmuludq_Pq_Qq__pmuludq_Vdq_Wdq);
/** Opcode 0x0f 0xf5. */
FNIEMOP_STUB(iemOp_pmaddwd_Pq_Qq__pmaddwd_Vdq_Wdq);
/** Opcode 0x0f 0xf6. */
FNIEMOP_STUB(iemOp_psadbw_Pq_Qq__psadbw_Vdq_Wdq);
/** Opcode 0x0f 0xf7. */
FNIEMOP_STUB(iemOp_maskmovq_Pq_Nq__maskmovdqu_Vdq_Udq);
/** Opcode 0x0f 0xf8. */
FNIEMOP_STUB(iemOp_psubb_Pq_Qq_psubb_Vdq_Wdq);
/** Opcode 0x0f 0xf9. */
FNIEMOP_STUB(iemOp_psubw_Pq_Qq__psubw_Vdq_Wdq);
/** Opcode 0x0f 0xfa. */
FNIEMOP_STUB(iemOp_psubd_Pq_Qq__psubd_Vdq_Wdq);
/** Opcode 0x0f 0xfb. */
FNIEMOP_STUB(iemOp_psubq_Pq_Qq__psbuq_Vdq_Wdq);
/** Opcode 0x0f 0xfc. */
FNIEMOP_STUB(iemOp_paddb_Pq_Qq__paddb_Vdq_Wdq);
/** Opcode 0x0f 0xfd. */
FNIEMOP_STUB(iemOp_paddw_Pq_Qq__paddw_Vdq_Wdq);
/** Opcode 0x0f 0xfe. */
FNIEMOP_STUB(iemOp_paddd_Pq_Qq__paddd_Vdq_Wdq);


const PFNIEMOP g_apfnTwoByteMap[256] =
{
    /* 0x00 */  iemOp_Grp6,             iemOp_Grp7,             iemOp_lar_Gv_Ew,        iemOp_lsl_Gv_Ew,
    /* 0x04 */  iemOp_Invalid,          iemOp_syscall,          iemOp_clts,             iemOp_sysret,
    /* 0x08 */  iemOp_invd,             iemOp_wbinvd,           iemOp_Invalid,          iemOp_ud2,
    /* 0x0c */  iemOp_Invalid,          iemOp_nop_Ev_prefetch,  iemOp_femms,            iemOp_3Dnow,
    /* 0x10 */  iemOp_movups_Vps_Wps__movupd_Vpd_Wpd__movss_Vss_Wss__movsd_Vsd_Wsd,
    /* 0x11 */  iemOp_movups_Wps_Vps__movupd_Wpd_Vpd__movss_Wss_Vss__movsd_Vsd_Wsd,
    /* 0x12 */  iemOp_movlps_Vq_Mq__movhlps_Vq_Uq__movlpd_Vq_Mq__movsldup_Vq_Wq__movddup_Vq_Wq,
    /* 0x13 */  iemOp_movlps_Mq_Vq__movlpd_Mq_Vq,
    /* 0x14 */  iemOp_unpckhlps_Vps_Wq__unpcklpd_Vpd_Wq,
    /* 0x15 */  iemOp_unpckhps_Vps_Wq__unpckhpd_Vpd_Wq,
    /* 0x16 */  iemOp_movhps_Vq_Mq__movlhps_Vq_Uq__movhpd_Vq_Mq__movshdup_Vq_Wq,
    /* 0x17 */  iemOp_movhps_Mq_Vq__movhpd_Mq_Vq,
    /* 0x18 */  iemOp_prefetch_Grp16,   iemOp_Invalid,          iemOp_Invalid,          iemOp_Invalid,
    /* 0x1c */  iemOp_Invalid,          iemOp_Invalid,          iemOp_Invalid,          iemOp_Invalid,
    /* 0x20 */  iemOp_mov_Rd_Cd,        iemOp_mov_Rd_Dd,        iemOp_mov_Cd_Rd,        iemOp_mov_Dd_Rd,
    /* 0x24 */  iemOp_mov_Rd_Td,        iemOp_Invalid,          iemOp_mov_Td_Rd,        iemOp_Invalid,
    /* 0x28 */  iemOp_movaps_Vps_Wps__movapd_Vpd_Wpd,
    /* 0x29 */  iemOp_movaps_Wps_Vps__movapd_Wpd_Vpd,
    /* 0x2a */  iemOp_cvtpi2ps_Vps_Qpi__cvtpi2pd_Vpd_Qpi__cvtsi2ss_Vss_Ey__cvtsi2sd_Vsd_Ey,
    /* 0x2b */  iemOp_movntps_Mps_Vps__movntpd_Mpd_Vpd,
    /* 0x2c */  iemOp_cvttps2pi_Ppi_Wps__cvttpd2pi_Ppi_Wpd__cvttss2si_Gy_Wss__cvttsd2si_Yu_Wsd,
    /* 0x2d */  iemOp_cvtps2pi_Ppi_Wps__cvtpd2pi_QpiWpd__cvtss2si_Gy_Wss__cvtsd2si_Gy_Wsd,
    /* 0x2e */  iemOp_ucomiss_Vss_Wss__ucomisd_Vsd_Wsd,
    /* 0x2f */  iemOp_comiss_Vss_Wss__comisd_Vsd_Wsd,
    /* 0x30 */  iemOp_wrmsr,            iemOp_rdtsc,            iemOp_rdmsr,            iemOp_rdpmc,
    /* 0x34 */  iemOp_sysenter,         iemOp_sysexit,          iemOp_Invalid,          iemOp_getsec,
    /* 0x38 */  iemOp_3byte_Esc_A4,     iemOp_Invalid,          iemOp_3byte_Esc_A5,     iemOp_Invalid,
    /* 0x3c */  iemOp_movnti_Gv_Ev/*?*/,iemOp_Invalid,          iemOp_Invalid,          iemOp_Invalid,
    /* 0x40 */  iemOp_cmovo_Gv_Ev,      iemOp_cmovno_Gv_Ev,     iemOp_cmovc_Gv_Ev,      iemOp_cmovnc_Gv_Ev,
    /* 0x44 */  iemOp_cmove_Gv_Ev,      iemOp_cmovne_Gv_Ev,     iemOp_cmovbe_Gv_Ev,     iemOp_cmovnbe_Gv_Ev,
    /* 0x48 */  iemOp_cmovs_Gv_Ev,      iemOp_cmovns_Gv_Ev,     iemOp_cmovp_Gv_Ev,      iemOp_cmovnp_Gv_Ev,
    /* 0x4c */  iemOp_cmovl_Gv_Ev,      iemOp_cmovnl_Gv_Ev,     iemOp_cmovle_Gv_Ev,     iemOp_cmovnle_Gv_Ev,
    /* 0x50 */  iemOp_movmskps_Gy_Ups__movmskpd_Gy_Upd,
    /* 0x51 */  iemOp_sqrtps_Wps_Vps__sqrtpd_Wpd_Vpd__sqrtss_Vss_Wss__sqrtsd_Vsd_Wsd,
    /* 0x52 */  iemOp_rsqrtps_Wps_Vps__rsqrtss_Vss_Wss,
    /* 0x53 */  iemOp_rcpps_Wps_Vps__rcpss_Vs_Wss,
    /* 0x54 */  iemOp_andps_Vps_Wps__andpd_Wpd_Vpd,
    /* 0x55 */  iemOp_andnps_Vps_Wps__andnpd_Wpd_Vpd,
    /* 0x56 */  iemOp_orps_Wpd_Vpd__orpd_Wpd_Vpd,
    /* 0x57 */  iemOp_xorps_Vps_Wps__xorpd_Wpd_Vpd,
    /* 0x58 */  iemOp_addps_Vps_Wps__addpd_Vpd_Wpd__addss_Vss_Wss__addsd_Vsd_Wsd,
    /* 0x59 */  iemOp_mulps_Vps_Wps__mulpd_Vpd_Wpd__mulss_Vss__Wss__mulsd_Vsd_Wsd,
    /* 0x5a */  iemOp_cvtps2pd_Vpd_Wps__cvtpd2ps_Vps_Wpd__cvtss2sd_Vsd_Wss__cvtsd2ss_Vss_Wsd,
    /* 0x5b */  iemOp_cvtdq2ps_Vps_Wdq__cvtps2dq_Vdq_Wps__cvtps2dq_Vdq_Wps,
    /* 0x5c */  iemOp_subps_Vps_Wps__subpd_Vps_Wdp__subss_Vss_Wss__subsd_Vsd_Wsd,
    /* 0x5d */  iemOp_minps_Vps_Wps__minpd_Vpd_Wpd__minss_Vss_Wss__minsd_Vsd_Wsd,
    /* 0x5e */  iemOp_divps_Vps_Wps__divpd_Vpd_Wpd__divss_Vss_Wss__divsd_Vsd_Wsd,
    /* 0x5f */  iemOp_maxps_Vps_Wps__maxpd_Vpd_Wpd__maxss_Vss_Wss__maxsd_Vsd_Wsd,
    /* 0x60 */  iemOp_punpcklbw_Pq_Qd__punpcklbw_Vdq_Wdq,
    /* 0x61 */  iemOp_punpcklwd_Pq_Qd__punpcklwd_Vdq_Wdq,
    /* 0x62 */  iemOp_punpckldq_Pq_Qd__punpckldq_Vdq_Wdq,
    /* 0x63 */  iemOp_packsswb_Pq_Qq__packsswb_Vdq_Wdq,
    /* 0x64 */  iemOp_pcmpgtb_Pq_Qq__pcmpgtb_Vdq_Wdq,
    /* 0x65 */  iemOp_pcmpgtw_Pq_Qq__pcmpgtw_Vdq_Wdq,
    /* 0x66 */  iemOp_pcmpgtd_Pq_Qq__pcmpgtd_Vdq_Wdq,
    /* 0x67 */  iemOp_packuswb_Pq_Qq__packuswb_Vdq_Wdq,
    /* 0x68 */  iemOp_punpckhbw_Pq_Qq__punpckhbw_Vdq_Wdq,
    /* 0x69 */  iemOp_punpckhwd_Pq_Qd__punpckhwd_Vdq_Wdq,
    /* 0x6a */  iemOp_punpckhdq_Pq_Qd__punpckhdq_Vdq_Wdq,
    /* 0x6b */  iemOp_packssdw_Pq_Qd__packssdq_Vdq_Wdq,
    /* 0x6c */  iemOp_punpcklqdq_Vdq_Wdq,
    /* 0x6d */  iemOp_punpckhqdq_Vdq_Wdq,
    /* 0x6e */  iemOp_movd_q_Pd_Ey__movd_q_Vy_Ey,
    /* 0x6f */  iemOp_movq_Pq_Qq__movdqa_Vdq_Wdq__movdqu_Vdq_Wdq,
    /* 0x70 */  iemOp_pshufw_Pq_Qq_Ib__pshufd_Vdq_Wdq_Ib__pshufhw_Vdq_Wdq_Ib__pshuflq_Vdq_Wdq_Ib,
    /* 0x71 */  iemOp_Grp12,
    /* 0x72 */  iemOp_Grp13,
    /* 0x73 */  iemOp_Grp14,
    /* 0x74 */  iemOp_pcmpeqb_Pq_Qq__pcmpeqb_Vdq_Wdq,
    /* 0x75 */  iemOp_pcmpeqw_Pq_Qq__pcmpeqw_Vdq_Wdq,
    /* 0x76 */  iemOp_pcmped_Pq_Qq__pcmpeqd_Vdq_Wdq,
    /* 0x77 */  iemOp_emms,
    /* 0x78 */  iemOp_vmread,           iemOp_vmwrite,          iemOp_Invalid,          iemOp_Invalid,
    /* 0x7c */  iemOp_haddpd_Vdp_Wpd__haddps_Vps_Wps,
    /* 0x7d */  iemOp_hsubpd_Vpd_Wpd__hsubps_Vps_Wps,
    /* 0x7e */  iemOp_movd_q_Ey_Pd__movd_q_Ey_Vy__movq_Vq_Wq,
    /* 0x7f */  iemOp_movq_Qq_Pq__movq_movdqa_Wdq_Vdq__movdqu_Wdq_Vdq,
    /* 0x80 */  iemOp_jo_Jv,            iemOp_jno_Jv,           iemOp_jc_Jv,            iemOp_jnc_Jv,
    /* 0x84 */  iemOp_je_Jv,            iemOp_jne_Jv,           iemOp_jbe_Jv,           iemOp_jnbe_Jv,
    /* 0x88 */  iemOp_js_Jv,            iemOp_jns_Jv,           iemOp_jp_Jv,            iemOp_jnp_Jv,
    /* 0x8c */  iemOp_jl_Jv,            iemOp_jnl_Jv,           iemOp_jle_Jv,           iemOp_jnle_Jv,
    /* 0x90 */  iemOp_seto_Jv,          iemOp_setno_Jv,         iemOp_setc_Jv,          iemOp_setnc_Jv,
    /* 0x94 */  iemOp_sete_Jv,          iemOp_setne_Jv,         iemOp_setbe_Jv,         iemOp_setnbe_Jv,
    /* 0x98 */  iemOp_sets_Jv,          iemOp_setns_Jv,         iemOp_setp_Jv,          iemOp_setnp_Jv,
    /* 0x9c */  iemOp_setl_Jv,          iemOp_setnl_Jv,         iemOp_setle_Jv,         iemOp_setnle_Jv,
    /* 0xa0 */  iemOp_push_fs,          iemOp_pop_fs,           iemOp_cpuid,            iemOp_bt_Ev_Gv,
    /* 0xa4 */  iemOp_shld_Ev_Gv_Ib,    iemOp_shld_Ev_Gv_CL,    iemOp_Invalid,          iemOp_Invalid,
    /* 0xa8 */  iemOp_push_gs,          iemOp_pop_gs,           iemOp_rsm,              iemOp_bts_Ev_Gv,
    /* 0xac */  iemOp_shrd_Ev_Gv_Ib,    iemOp_shrd_Ev_Gv_CL,    iemOp_Grp15,            iemOp_imul_Gv_Ev,
    /* 0xb0 */  iemOp_cmpxchg_Eb_Gb,    iemOp_cmpxchg_Ev_Gv,    iemOp_lss_Gv_Mp,        iemOp_btr_Ev_Gv,
    /* 0xb4 */  iemOp_lfs_Gv_Mp,        iemOp_lgs_Gv_Mp,        iemOp_movzx_Gv_Eb,      iemOp_movzx_Gv_Ew,
    /* 0xb8 */  iemOp_popcnt_Gv_Ev_jmpe,iemOp_Grp10,            iemOp_Grp11,            iemOp_btc_Ev_Gv,
    /* 0xbc */  iemOp_bsf_Gv_Ev,        iemOp_bsr_Gv_Ev,        iemOp_movsx_Gv_Eb,      iemOp_movsx_Gv_Ew,
    /* 0xc0 */  iemOp_xadd_Eb_Gb,
    /* 0xc1 */  iemOp_xadd_Ev_Gv,
    /* 0xc2 */  iemOp_cmpps_Vps_Wps_Ib__cmppd_Vpd_Wpd_Ib__cmpss_Vss_Wss_Ib__cmpsd_Vsd_Wsd_Ib,
    /* 0xc3 */  iemOp_movnti_My_Gy,
    /* 0xc4 */  iemOp_pinsrw_Pq_Ry_Mw_Ib__pinsrw_Vdq_Ry_Mw_Ib,
    /* 0xc5 */  iemOp_pextrw_Gd_Nq_Ib__pextrw_Gd_Udq_Ib,
    /* 0xc6 */  iemOp_shufps_Vps_Wps_Ib__shufdp_Vpd_Wpd_Ib,
    /* 0xc7 */  iemOp_Grp9,
    /* 0xc8 */  iemOp_bswap_rAX_r8,     iemOp_bswap_rCX_r9,     iemOp_bswap_rDX_r10,    iemOp_bswap_rBX_r11,
    /* 0xcc */  iemOp_bswap_rSP_r12,    iemOp_bswap_rBP_r13,    iemOp_bswap_rSI_r14,    iemOp_bswap_rDI_r15,
    /* 0xd0 */  iemOp_addsubpd_Vpd_Wpd__addsubps_Vps_Wps,
    /* 0xd1 */  iemOp_psrlw_Pp_Qp__psrlw_Vdp_Wdq,
    /* 0xd2 */  iemOp_psrld_Pq_Qq__psrld_Vdq_Wdq,
    /* 0xd3 */  iemOp_psrlq_Pq_Qq__psrlq_Vdq_Wdq,
    /* 0xd4 */  iemOp_paddq_Pq_Qq__paddq_Vdq_Wdq,
    /* 0xd5 */  iemOp_pmulq_Pq_Qq__pmullw_Vdq_Wdq,
    /* 0xd6 */  iemOp_movq_Wq_Vq__movq2dq_Vdq_Nq__movdq2q_Pq_Uq,
    /* 0xd7 */  iemOp_pmovmskb_Gd_Nq__pmovmskb_Gd_Udq,
    /* 0xd8 */  iemOp_psubusb_Pq_Qq__psubusb_Vdq_Wdq,
    /* 0xd9 */  iemOp_psubusw_Pq_Qq__psubusw_Vdq_Wdq,
    /* 0xda */  iemOp_pminub_Pq_Qq__pminub_Vdq_Wdq,
    /* 0xdb */  iemOp_pand_Pq_Qq__pand_Vdq_Wdq,
    /* 0xdc */  iemOp_paddusb_Pq_Qq__paddusb_Vdq_Wdq,
    /* 0xdd */  iemOp_paddusw_Pq_Qq__paddusw_Vdq_Wdq,
    /* 0xde */  iemOp_pmaxub_Pq_Qq__pamxub_Vdq_Wdq,
    /* 0xdf */  iemOp_pandn_Pq_Qq__pandn_Vdq_Wdq,
    /* 0xe0 */  iemOp_pavgb_Pq_Qq__pavgb_Vdq_Wdq,
    /* 0xe1 */  iemOp_psraw_Pq_Qq__psraw_Vdq_Wdq,
    /* 0xe2 */  iemOp_psrad_Pq_Qq__psrad_Vdq_Wdq,
    /* 0xe3 */  iemOp_pavgw_Pq_Qq__pavgw_Vdq_Wdq,
    /* 0xe4 */  iemOp_pmulhuw_Pq_Qq__pmulhuw_Vdq_Wdq,
    /* 0xe5 */  iemOp_pmulhw_Pq_Qq__pmulhw_Vdq_Wdq,
    /* 0xe6 */  iemOp_cvttpd2dq_Vdq_Wdp__cvtdq2pd_Vdq_Wpd__cvtpd2dq_Vdq_Wpd,
    /* 0xe7 */  iemOp_movntq_Mq_Pq__movntdq_Mdq_Vdq,
    /* 0xe8 */  iemOp_psubsb_Pq_Qq__psubsb_Vdq_Wdq,
    /* 0xe9 */  iemOp_psubsw_Pq_Qq__psubsw_Vdq_Wdq,
    /* 0xea */  iemOp_pminsw_Pq_Qq__pminsw_Vdq_Wdq,
    /* 0xeb */  iemOp_por_Pq_Qq__por_Vdq_Wdq,
    /* 0xec */  iemOp_paddsb_Pq_Qq__paddsb_Vdq_Wdq,
    /* 0xed */  iemOp_paddsw_Pq_Qq__paddsw_Vdq_Wdq,
    /* 0xee */  iemOp_pmaxsw_Pq_Qq__pmaxsw_Vdq_Wdq,
    /* 0xef */  iemOp_pxor_Pq_Qq__pxor_Vdq_Wdq,
    /* 0xf0 */  iemOp_lddqu_Vdq_Mdq,
    /* 0xf1 */  iemOp_psllw_Pq_Qq__pslw_Vdq_Wdq,
    /* 0xf2 */  iemOp_psld_Pq_Qq__pslld_Vdq_Wdq,
    /* 0xf3 */  iemOp_psllq_Pq_Qq__pslq_Vdq_Wdq,
    /* 0xf4 */  iemOp_pmuludq_Pq_Qq__pmuludq_Vdq_Wdq,
    /* 0xf5 */  iemOp_pmaddwd_Pq_Qq__pmaddwd_Vdq_Wdq,
    /* 0xf6 */  iemOp_psadbw_Pq_Qq__psadbw_Vdq_Wdq,
    /* 0xf7 */  iemOp_maskmovq_Pq_Nq__maskmovdqu_Vdq_Udq,
    /* 0xf8 */  iemOp_psubb_Pq_Qq_psubb_Vdq_Wdq,
    /* 0xf9 */  iemOp_psubw_Pq_Qq__psubw_Vdq_Wdq,
    /* 0xfa */  iemOp_psubd_Pq_Qq__psubd_Vdq_Wdq,
    /* 0xfb */  iemOp_psubq_Pq_Qq__psbuq_Vdq_Wdq,
    /* 0xfc */  iemOp_paddb_Pq_Qq__paddb_Vdq_Wdq,
    /* 0xfd */  iemOp_paddw_Pq_Qq__paddw_Vdq_Wdq,
    /* 0xfe */  iemOp_paddd_Pq_Qq__paddd_Vdq_Wdq,
    /* 0xff */  iemOp_Invalid
};

/** @}  */


/** @name One byte opcodes.
 *
 * @{
 */

/** Opcode 0x00. */
FNIEMOP_DEF(iemOp_add_Eb_Gb)
{
    IEMOP_MNEMONIC("add Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_add);
}


/** Opcode 0x01. */
FNIEMOP_DEF(iemOp_add_Ev_Gv)
{
    IEMOP_MNEMONIC("add Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_add);
}


/** Opcode 0x02. */
FNIEMOP_DEF(iemOp_add_Gb_Eb)
{
    IEMOP_MNEMONIC("add Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_add);
}


/** Opcode 0x03. */
FNIEMOP_DEF(iemOp_add_Gv_Ev)
{
    IEMOP_MNEMONIC("add Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_add);
}


/** Opcode 0x04. */
FNIEMOP_DEF(iemOp_add_Al_Ib)
{
    IEMOP_MNEMONIC("add al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_add);
}


/** Opcode 0x05. */
FNIEMOP_DEF(iemOp_add_eAX_Iz)
{
    IEMOP_MNEMONIC("add rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_add);
}


/**
 * Common 'push segment-register' helper.
 */
FNIEMOP_DEF_1(iemOpCommonPushSReg, uint8_t, iReg)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    if (iReg < X86_SREG_FS)
        IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, u16Value);
            IEM_MC_FETCH_SREG_U16(u16Value, iReg);
            IEM_MC_PUSH_U16(u16Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, u32Value);
            IEM_MC_FETCH_SREG_U32_ZX(u32Value, iReg);
            IEM_MC_PUSH_U32(u32Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64Value);
            IEM_MC_FETCH_SREG_U64_ZX(u64Value, iReg);
            IEM_MC_PUSH_U64(u64Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
    }

    return VINF_SUCCESS;
}


/** Opcode 0x06. */
FNIEMOP_DEF(iemOp_push_ES)
{
    IEMOP_MNEMONIC("push es");
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_ES);
}


/** Opcode 0x07. */
FNIEMOP_DEF(iemOp_pop_ES)
{
    IEMOP_MNEMONIC("pop es");
    IEMOP_HLP_NO_64BIT();
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemOpCImpl_pop_Sreg, X86_SREG_ES, pIemCpu->enmEffOpSize);
}


/** Opcode 0x08. */
FNIEMOP_DEF(iemOp_or_Eb_Gb)
{
    IEMOP_MNEMONIC("or  Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_or);
}


/** Opcode 0x09. */
FNIEMOP_DEF(iemOp_or_Ev_Gv)
{
    IEMOP_MNEMONIC("or  Ev,Gv ");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_or);
}


/** Opcode 0x0a. */
FNIEMOP_DEF(iemOp_or_Gb_Eb)
{
    IEMOP_MNEMONIC("or  Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_or);
}


/** Opcode 0x0b. */
FNIEMOP_DEF(iemOp_or_Gv_Ev)
{
    IEMOP_MNEMONIC("or  Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_or);
}


/** Opcode 0x0c. */
FNIEMOP_DEF(iemOp_or_Al_Ib)
{
    IEMOP_MNEMONIC("or  al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_or);
}


/** Opcode 0x0d. */
FNIEMOP_DEF(iemOp_or_eAX_Iz)
{
    IEMOP_MNEMONIC("or  rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_or);
}


/** Opcode 0x0e. */
FNIEMOP_DEF(iemOp_push_CS)
{
    IEMOP_MNEMONIC("push cs");
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_CS);
}


/** Opcode 0x0f. */
FNIEMOP_DEF(iemOp_2byteEscape)
{
    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnTwoByteMap[b]);
}

/** Opcode 0x10. */
FNIEMOP_DEF(iemOp_adc_Eb_Gb)
{
    IEMOP_MNEMONIC("adc Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_adc);
}


/** Opcode 0x11. */
FNIEMOP_DEF(iemOp_adc_Ev_Gv)
{
    IEMOP_MNEMONIC("adc Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_adc);
}


/** Opcode 0x12. */
FNIEMOP_DEF(iemOp_adc_Gb_Eb)
{
    IEMOP_MNEMONIC("adc Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_adc);
}


/** Opcode 0x13. */
FNIEMOP_DEF(iemOp_adc_Gv_Ev)
{
    IEMOP_MNEMONIC("adc Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_adc);
}


/** Opcode 0x14. */
FNIEMOP_DEF(iemOp_adc_Al_Ib)
{
    IEMOP_MNEMONIC("adc al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_adc);
}


/** Opcode 0x15. */
FNIEMOP_DEF(iemOp_adc_eAX_Iz)
{
    IEMOP_MNEMONIC("adc rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_adc);
}


/** Opcode 0x16. */
FNIEMOP_DEF(iemOp_push_SS)
{
    IEMOP_MNEMONIC("push ss");
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_SS);
}


/** Opcode 0x17. */
FNIEMOP_DEF(iemOp_pop_SS)
{
    IEMOP_MNEMONIC("pop ss"); /** @todo implies instruction fusing? */
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    return IEM_MC_DEFER_TO_CIMPL_2(iemOpCImpl_pop_Sreg, X86_SREG_SS, pIemCpu->enmEffOpSize);
}


/** Opcode 0x18. */
FNIEMOP_DEF(iemOp_sbb_Eb_Gb)
{
    IEMOP_MNEMONIC("sbb Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_sbb);
}


/** Opcode 0x19. */
FNIEMOP_DEF(iemOp_sbb_Ev_Gv)
{
    IEMOP_MNEMONIC("sbb Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_sbb);
}


/** Opcode 0x1a. */
FNIEMOP_DEF(iemOp_sbb_Gb_Eb)
{
    IEMOP_MNEMONIC("sbb Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_sbb);
}


/** Opcode 0x1b. */
FNIEMOP_DEF(iemOp_sbb_Gv_Ev)
{
    IEMOP_MNEMONIC("sbb Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_sbb);
}


/** Opcode 0x1c. */
FNIEMOP_DEF(iemOp_sbb_Al_Ib)
{
    IEMOP_MNEMONIC("sbb al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_sbb);
}


/** Opcode 0x1d. */
FNIEMOP_DEF(iemOp_sbb_eAX_Iz)
{
    IEMOP_MNEMONIC("sbb rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_sbb);
}


/** Opcode 0x1e. */
FNIEMOP_DEF(iemOp_push_DS)
{
    IEMOP_MNEMONIC("push ds");
    return FNIEMOP_CALL_1(iemOpCommonPushSReg, X86_SREG_DS);
}


/** Opcode 0x1f. */
FNIEMOP_DEF(iemOp_pop_DS)
{
    IEMOP_MNEMONIC("pop ds");
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_NO_64BIT();
    return IEM_MC_DEFER_TO_CIMPL_2(iemOpCImpl_pop_Sreg, X86_SREG_DS, pIemCpu->enmEffOpSize);
}


/** Opcode 0x20. */
FNIEMOP_DEF(iemOp_and_Eb_Gb)
{
    IEMOP_MNEMONIC("and Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_and);
}


/** Opcode 0x21. */
FNIEMOP_DEF(iemOp_and_Ev_Gv)
{
    IEMOP_MNEMONIC("and Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_and);
}


/** Opcode 0x22. */
FNIEMOP_DEF(iemOp_and_Gb_Eb)
{
    IEMOP_MNEMONIC("and Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_and);
}


/** Opcode 0x23. */
FNIEMOP_DEF(iemOp_and_Gv_Ev)
{
    IEMOP_MNEMONIC("and Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_and);
}


/** Opcode 0x24. */
FNIEMOP_DEF(iemOp_and_Al_Ib)
{
    IEMOP_MNEMONIC("and al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_and);
}


/** Opcode 0x25. */
FNIEMOP_DEF(iemOp_and_eAX_Iz)
{
    IEMOP_MNEMONIC("and rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_and);
}


/** Opcode 0x26. */
FNIEMOP_DEF(iemOp_seg_ES)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_ES;
    pIemCpu->iEffSeg    = X86_SREG_ES;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x27. */
FNIEMOP_STUB(iemOp_daa);


/** Opcode 0x28. */
FNIEMOP_DEF(iemOp_sub_Eb_Gb)
{
    IEMOP_MNEMONIC("sub Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_sub);
}


/** Opcode 0x29. */
FNIEMOP_DEF(iemOp_sub_Ev_Gv)
{
    IEMOP_MNEMONIC("sub Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_sub);
}


/** Opcode 0x2a. */
FNIEMOP_DEF(iemOp_sub_Gb_Eb)
{
    IEMOP_MNEMONIC("sub Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_sub);
}


/** Opcode 0x2b. */
FNIEMOP_DEF(iemOp_sub_Gv_Ev)
{
    IEMOP_MNEMONIC("sub Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_sub);
}


/** Opcode 0x2c. */
FNIEMOP_DEF(iemOp_sub_Al_Ib)
{
    IEMOP_MNEMONIC("sub al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_sub);
}


/** Opcode 0x2d. */
FNIEMOP_DEF(iemOp_sub_eAX_Iz)
{
    IEMOP_MNEMONIC("sub rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_sub);
}


/** Opcode 0x2e. */
FNIEMOP_DEF(iemOp_seg_CS)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_CS;
    pIemCpu->iEffSeg    = X86_SREG_CS;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x2f. */
FNIEMOP_STUB(iemOp_das);


/** Opcode 0x30. */
FNIEMOP_DEF(iemOp_xor_Eb_Gb)
{
    IEMOP_MNEMONIC("xor Eb,Gb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_xor);
}


/** Opcode 0x31. */
FNIEMOP_DEF(iemOp_xor_Ev_Gv)
{
    IEMOP_MNEMONIC("xor Ev,Gv");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_xor);
}


/** Opcode 0x32. */
FNIEMOP_DEF(iemOp_xor_Gb_Eb)
{
    IEMOP_MNEMONIC("xor Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_xor);
}


/** Opcode 0x33. */
FNIEMOP_DEF(iemOp_xor_Gv_Ev)
{
    IEMOP_MNEMONIC("xor Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_xor);
}


/** Opcode 0x34. */
FNIEMOP_DEF(iemOp_xor_Al_Ib)
{
    IEMOP_MNEMONIC("xor al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_xor);
}


/** Opcode 0x35. */
FNIEMOP_DEF(iemOp_xor_eAX_Iz)
{
    IEMOP_MNEMONIC("xor rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_xor);
}


/** Opcode 0x36. */
FNIEMOP_DEF(iemOp_seg_SS)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_SS;
    pIemCpu->iEffSeg    = X86_SREG_SS;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x37. */
FNIEMOP_STUB(iemOp_aaa);


/** Opcode 0x38. */
FNIEMOP_DEF(iemOp_cmp_Eb_Gb)
{
    IEMOP_MNEMONIC("cmp Eb,Gb");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo do we have to decode the whole instruction first?  */
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_cmp);
}


/** Opcode 0x39. */
FNIEMOP_DEF(iemOp_cmp_Ev_Gv)
{
    IEMOP_MNEMONIC("cmp Ev,Gv");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo do we have to decode the whole instruction first?  */
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_cmp);
}


/** Opcode 0x3a. */
FNIEMOP_DEF(iemOp_cmp_Gb_Eb)
{
    IEMOP_MNEMONIC("cmp Gb,Eb");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_r8_rm, &g_iemAImpl_cmp);
}


/** Opcode 0x3b. */
FNIEMOP_DEF(iemOp_cmp_Gv_Ev)
{
    IEMOP_MNEMONIC("cmp Gv,Ev");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rv_rm, &g_iemAImpl_cmp);
}


/** Opcode 0x3c. */
FNIEMOP_DEF(iemOp_cmp_Al_Ib)
{
    IEMOP_MNEMONIC("cmp al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_cmp);
}


/** Opcode 0x3d. */
FNIEMOP_DEF(iemOp_cmp_eAX_Iz)
{
    IEMOP_MNEMONIC("cmp rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_cmp);
}


/** Opcode 0x3e. */
FNIEMOP_DEF(iemOp_seg_DS)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_DS;
    pIemCpu->iEffSeg    = X86_SREG_DS;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x3f. */
FNIEMOP_STUB(iemOp_aas);

/**
 * Common 'inc/dec/not/neg register' helper.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryGReg, PCIEMOPUNARYSIZES, pImpl, uint8_t, iReg)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint16_t *,  pu16Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U16(pu16Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU16, pu16Dst, pEFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint32_t *,  pu32Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U32(pu32Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU32, pu32Dst, pEFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint64_t *,  pu64Dst, 0);
            IEM_MC_ARG(uint32_t *,  pEFlags, 1);
            IEM_MC_REF_GREG_U64(pu64Dst, iReg);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU64, pu64Dst, pEFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
    }
    return VINF_SUCCESS;
}


/** Opcode 0x40. */
FNIEMOP_DEF(iemOp_inc_eAX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eAX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xAX);
}


/** Opcode 0x41. */
FNIEMOP_DEF(iemOp_inc_eCX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B;
        pIemCpu->uRexB     = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eCX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xCX);
}


/** Opcode 0x42. */
FNIEMOP_DEF(iemOp_inc_eDX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_X;
        pIemCpu->uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eDX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xDX);
}



/** Opcode 0x43. */
FNIEMOP_DEF(iemOp_inc_eBX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X;
        pIemCpu->uRexB     = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eBX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xBX);
}


/** Opcode 0x44. */
FNIEMOP_DEF(iemOp_inc_eSP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R;
        pIemCpu->uRexReg   = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eSP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xSP);
}


/** Opcode 0x45. */
FNIEMOP_DEF(iemOp_inc_eBP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexB     = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eBP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xBP);
}


/** Opcode 0x46. */
FNIEMOP_DEF(iemOp_inc_eSI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_X;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eSI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xSI);
}


/** Opcode 0x47. */
FNIEMOP_DEF(iemOp_inc_eDI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexB     = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("inc eDI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_inc, X86_GREG_xDI);
}


/** Opcode 0x48. */
FNIEMOP_DEF(iemOp_dec_eAX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_SIZE_REX_W;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eAX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xAX);
}


/** Opcode 0x49. */
FNIEMOP_DEF(iemOp_dec_eCX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexB     = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eCX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xCX);
}


/** Opcode 0x4a. */
FNIEMOP_DEF(iemOp_dec_eDX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexIndex = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eDX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xDX);
}


/** Opcode 0x4b. */
FNIEMOP_DEF(iemOp_dec_eBX)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexB     = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eBX");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xBX);
}


/** Opcode 0x4c. */
FNIEMOP_DEF(iemOp_dec_eSP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexReg   = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eSP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xSP);
}


/** Opcode 0x4d. */
FNIEMOP_DEF(iemOp_dec_eBP)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexB     = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eBP");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xBP);
}


/** Opcode 0x4e. */
FNIEMOP_DEF(iemOp_dec_eSI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eSI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xSI);
}


/** Opcode 0x4f. */
FNIEMOP_DEF(iemOp_dec_eDI)
{
    /*
     * This is a REX prefix in 64-bit mode.
     */
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        pIemCpu->fPrefixes |= IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W;
        pIemCpu->uRexReg   = 1 << 3;
        pIemCpu->uRexB     = 1 << 3;
        pIemCpu->uRexIndex = 1 << 3;
        iemRecalEffOpSize(pIemCpu);

        uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
        return FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }

    IEMOP_MNEMONIC("dec eDI");
    return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, &g_iemAImpl_dec, X86_GREG_xDI);
}


/**
 * Common 'push register' helper.
 */
FNIEMOP_DEF_1(iemOpCommonPushGReg, uint8_t, iReg)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        iReg |= pIemCpu->uRexB;
        pIemCpu->enmDefOpSize = IEMMODE_64BIT;
        pIemCpu->enmEffOpSize = !(pIemCpu->fPrefixes & IEM_OP_PRF_SIZE_OP) ? IEMMODE_64BIT : IEMMODE_16BIT;
    }

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, u16Value);
            IEM_MC_FETCH_GREG_U16(u16Value, iReg);
            IEM_MC_PUSH_U16(u16Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, u32Value);
            IEM_MC_FETCH_GREG_U32(u32Value, iReg);
            IEM_MC_PUSH_U32(u32Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64Value);
            IEM_MC_FETCH_GREG_U64(u64Value, iReg);
            IEM_MC_PUSH_U64(u64Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
    }

    return VINF_SUCCESS;
}


/** Opcode 0x50. */
FNIEMOP_DEF(iemOp_push_eAX)
{
    IEMOP_MNEMONIC("push rAX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xAX);
}


/** Opcode 0x51. */
FNIEMOP_DEF(iemOp_push_eCX)
{
    IEMOP_MNEMONIC("push rCX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xCX);
}


/** Opcode 0x52. */
FNIEMOP_DEF(iemOp_push_eDX)
{
    IEMOP_MNEMONIC("push rDX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xDX);
}


/** Opcode 0x53. */
FNIEMOP_DEF(iemOp_push_eBX)
{
    IEMOP_MNEMONIC("push rBX");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xBX);
}


/** Opcode 0x54. */
FNIEMOP_DEF(iemOp_push_eSP)
{
    IEMOP_MNEMONIC("push rSP");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xSP);
}


/** Opcode 0x55. */
FNIEMOP_DEF(iemOp_push_eBP)
{
    IEMOP_MNEMONIC("push rBP");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xBP);
}


/** Opcode 0x56. */
FNIEMOP_DEF(iemOp_push_eSI)
{
    IEMOP_MNEMONIC("push rSI");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xSI);
}


/** Opcode 0x57. */
FNIEMOP_DEF(iemOp_push_eDI)
{
    IEMOP_MNEMONIC("push rDI");
    return FNIEMOP_CALL_1(iemOpCommonPushGReg, X86_GREG_xDI);
}


/**
 * Common 'pop register' helper.
 */
FNIEMOP_DEF_1(iemOpCommonPopGReg, uint8_t, iReg)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    if (pIemCpu->enmCpuMode == IEMMODE_64BIT)
    {
        iReg |= pIemCpu->uRexB;
        pIemCpu->enmDefOpSize = IEMMODE_64BIT;
        pIemCpu->enmEffOpSize = !(pIemCpu->fPrefixes & IEM_OP_PRF_SIZE_OP) ? IEMMODE_64BIT : IEMMODE_16BIT;
    }

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, *pu16Dst);
            IEM_MC_REF_GREG_U16(pu16Dst, iReg);
            IEM_MC_POP_U16(pu16Dst);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, *pu32Dst);
            IEM_MC_REF_GREG_U32(pu32Dst, iReg);
            IEM_MC_POP_U32(pu32Dst);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, *pu64Dst);
            IEM_MC_REF_GREG_U64(pu64Dst, iReg);
            IEM_MC_POP_U64(pu64Dst);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
    }

    return VINF_SUCCESS;
}


/** Opcode 0x58. */
FNIEMOP_DEF(iemOp_pop_eAX)
{
    IEMOP_MNEMONIC("pop rAX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xAX);
}


/** Opcode 0x59. */
FNIEMOP_DEF(iemOp_pop_eCX)
{
    IEMOP_MNEMONIC("pop rCX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xCX);
}


/** Opcode 0x5a. */
FNIEMOP_DEF(iemOp_pop_eDX)
{
    IEMOP_MNEMONIC("pop rDX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xDX);
}


/** Opcode 0x5b. */
FNIEMOP_DEF(iemOp_pop_eBX)
{
    IEMOP_MNEMONIC("pop rBX");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xBX);
}


/** Opcode 0x5c. */
FNIEMOP_DEF(iemOp_pop_eSP)
{
    IEMOP_MNEMONIC("pop rSP");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xSP);
}


/** Opcode 0x5d. */
FNIEMOP_DEF(iemOp_pop_eBP)
{
    IEMOP_MNEMONIC("pop rBP");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xBP);
}


/** Opcode 0x5e. */
FNIEMOP_DEF(iemOp_pop_eSI)
{
    IEMOP_MNEMONIC("pop rSI");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xSI);
}


/** Opcode 0x5f. */
FNIEMOP_DEF(iemOp_pop_eDI)
{
    IEMOP_MNEMONIC("pop rDI");
    return FNIEMOP_CALL_1(iemOpCommonPopGReg, X86_GREG_xDI);
}


/** Opcode 0x60. */
FNIEMOP_DEF(iemOp_pusha)
{
    IEMOP_MNEMONIC("pusha");
    IEMOP_HLP_NO_64BIT();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
        return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_pusha_16);
    Assert(pIemCpu->enmEffOpSize == IEMMODE_32BIT);
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_pusha_32);
}


/** Opcode 0x61. */
FNIEMOP_DEF(iemOp_popa)
{
    IEMOP_MNEMONIC("popa");
    IEMOP_HLP_NO_64BIT();
    if (pIemCpu->enmEffOpSize == IEMMODE_16BIT)
        return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_popa_16);
    Assert(pIemCpu->enmEffOpSize == IEMMODE_32BIT);
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_popa_32);
}


/** Opcode 0x62. */
FNIEMOP_STUB(iemOp_bound_Gv_Ma);
/** Opcode 0x63. */
FNIEMOP_STUB(iemOp_arpl_Ew_Gw);


/** Opcode 0x64. */
FNIEMOP_DEF(iemOp_seg_FS)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_FS;
    pIemCpu->iEffSeg    = X86_SREG_FS;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x65. */
FNIEMOP_DEF(iemOp_seg_GS)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SEG_GS;
    pIemCpu->iEffSeg    = X86_SREG_GS;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x66. */
FNIEMOP_DEF(iemOp_op_size)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SIZE_OP;
    iemRecalEffOpSize(pIemCpu);

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x67. */
FNIEMOP_DEF(iemOp_addr_size)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_SIZE_ADDR;
    switch (pIemCpu->enmDefAddrMode)
    {
        case IEMMODE_16BIT: pIemCpu->enmEffAddrMode = IEMMODE_32BIT; break;
        case IEMMODE_32BIT: pIemCpu->enmEffAddrMode = IEMMODE_16BIT; break;
        case IEMMODE_64BIT: pIemCpu->enmEffAddrMode = IEMMODE_32BIT; break;
        default: AssertFailed();
    }

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0x68. */
FNIEMOP_DEF(iemOp_push_Iz)
{
    IEMOP_MNEMONIC("push Iz");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U16(u16Imm);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U32(u32Imm);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            IEM_MC_BEGIN(0,0);
            IEM_MC_PUSH_U64(u64Imm);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0x69. */
FNIEMOP_DEF(iemOp_imul_Gv_Ev_Iz)
{
    IEMOP_MNEMONIC("imul Gv,Ev,Iz"); /* Gv = Ev * Iz; */
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);

                IEM_MC_FETCH_GREG_U16(u16Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U16(u16Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);

                IEM_MC_FETCH_GREG_U32(u32Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u32, pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U32(u32Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u32, pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);

                IEM_MC_FETCH_GREG_U64(u64Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,            2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U64(u64Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
        }
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x6a. */
FNIEMOP_DEF(iemOp_push_Ib)
{
    IEMOP_MNEMONIC("push Ib");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0,0);
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_PUSH_U16(i8Imm);
            break;
        case IEMMODE_32BIT:
            IEM_MC_PUSH_U32(i8Imm);
            break;
        case IEMMODE_64BIT:
            IEM_MC_PUSH_U64(i8Imm);
            break;
    }
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x6b. */
FNIEMOP_DEF(iemOp_imul_Gv_Ev_Ib)
{
    IEMOP_MNEMONIC("imul Gv,Ev,Ib"); /* Gv = Ev * Iz; */
    uint8_t bRm;   IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);

                IEM_MC_FETCH_GREG_U16(u16Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint16_t,      u16Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U16(u16Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu16Dst, u16Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);

                IEM_MC_FETCH_GREG_U32(u32Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u32, pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint32_t,      u32Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U32(u32Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu32Dst, u32Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u32, pu32Dst, u32Src, pEFlags);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register operand */
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);

                IEM_MC_FETCH_GREG_U64(u64Tmp, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory operand */
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ (int8_t)u8Imm, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);
                IEM_MC_LOCAL(uint64_t,      u64Tmp);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U64(u64Tmp, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_LOCAL(pu64Dst, u64Tmp);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_imul_two_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Tmp);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            return VINF_SUCCESS;
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x6c. */
FNIEMOP_DEF(iemOp_insb_Yb_DX)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep ins Yb,DX");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op8_addr16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op8_addr32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op8_addr64);
        }
    }
    else
    {
        IEMOP_MNEMONIC("ins Yb,DX");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op8_addr16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op8_addr32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op8_addr64);
        }
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x6d. */
FNIEMOP_DEF(iemOp_inswd_Yv_DX)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ))
    {
        IEMOP_MNEMONIC("rep ins Yv,DX");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op16_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op16_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op16_addr64);
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op32_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op32_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_ins_op32_addr64);
                }
                break;
        }
    }
    else
    {
        IEMOP_MNEMONIC("ins Yv,DX");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op16_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op16_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op16_addr64);
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op32_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op32_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_ins_op32_addr64);
                }
                break;
        }
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x6e. */
FNIEMOP_DEF(iemOp_outsb_Yb_DX)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep out DX,Yb");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op8_addr16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op8_addr32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op8_addr64);
        }
    }
    else
    {
        IEMOP_MNEMONIC("out DX,Yb");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op8_addr16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op8_addr32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op8_addr64);
        }
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x6f. */
FNIEMOP_DEF(iemOp_outswd_Yv_DX)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ))
    {
        IEMOP_MNEMONIC("rep outs DX,Yv");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op16_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op16_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op16_addr64);
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op32_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op32_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_rep_outs_op32_addr64);
                }
                break;
        }
    }
    else
    {
        IEMOP_MNEMONIC("outs DX,Yv");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op16_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op16_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op16_addr64);
                }
                break;
            case IEMMODE_64BIT:
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op32_addr16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op32_addr32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_outs_op32_addr64);
                }
                break;
        }
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_3);
}


/** Opcode 0x70. */
FNIEMOP_DEF(iemOp_jo_Jb)
{
    IEMOP_MNEMONIC("jo  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x71. */
FNIEMOP_DEF(iemOp_jno_Jb)
{
    IEMOP_MNEMONIC("jno Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}

/** Opcode 0x72. */
FNIEMOP_DEF(iemOp_jc_Jb)
{
    IEMOP_MNEMONIC("jc/jnae Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x73. */
FNIEMOP_DEF(iemOp_jnc_Jb)
{
    IEMOP_MNEMONIC("jnc/jnb Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_CF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x74. */
FNIEMOP_DEF(iemOp_je_Jb)
{
    IEMOP_MNEMONIC("je/jz   Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x75. */
FNIEMOP_DEF(iemOp_jne_Jb)
{
    IEMOP_MNEMONIC("jne/jnz Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_ZF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x76. */
FNIEMOP_DEF(iemOp_jbe_Jb)
{
    IEMOP_MNEMONIC("jbe/jna Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x77. */
FNIEMOP_DEF(iemOp_jnbe_Jb)
{
    IEMOP_MNEMONIC("jnbe/ja Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_ANY_BITS_SET(X86_EFL_CF | X86_EFL_ZF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x78. */
FNIEMOP_DEF(iemOp_js_Jb)
{
    IEMOP_MNEMONIC("js  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x79. */
FNIEMOP_DEF(iemOp_jns_Jb)
{
    IEMOP_MNEMONIC("jns Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_SF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7a. */
FNIEMOP_DEF(iemOp_jp_Jb)
{
    IEMOP_MNEMONIC("jp  Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7b. */
FNIEMOP_DEF(iemOp_jnp_Jb)
{
    IEMOP_MNEMONIC("jnp Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET(X86_EFL_PF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7c. */
FNIEMOP_DEF(iemOp_jl_Jb)
{
    IEMOP_MNEMONIC("jl/jnge Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7d. */
FNIEMOP_DEF(iemOp_jnl_Jb)
{
    IEMOP_MNEMONIC("jnl/jge Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BITS_NE(X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7e. */
FNIEMOP_DEF(iemOp_jle_Jb)
{
    IEMOP_MNEMONIC("jle/jng Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ELSE() {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x7f. */
FNIEMOP_DEF(iemOp_jnle_Jb)
{
    IEMOP_MNEMONIC("jnle/jg Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(X86_EFL_ZF, X86_EFL_SF, X86_EFL_OF) {
        IEM_MC_ADVANCE_RIP();
    } IEM_MC_ELSE() {
        IEM_MC_REL_JMP_S8(i8Imm);
    } IEM_MC_ENDIF();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x80. */
FNIEMOP_DEF(iemOp_Grp1_Eb_Ib_80)
{
    uint8_t bRm;   IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_MNEMONIC2("add\0or\0\0adc\0sbb\0and\0sub\0xor\0cmp" + ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)*4, "Eb,Ib");
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[(bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK];

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register target */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,                 0);
        IEM_MC_ARG_CONST(uint8_t,   u8Src, /*=*/ u8Imm,     1);
        IEM_MC_ARG(uint32_t *,      pEFlags,                2);

        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory target */
        uint32_t fAccess;
        if (pImpl->pfnLockedU8)
            fAccess = IEM_ACCESS_DATA_RW;
        else
        {   /* CMP */
            IEMOP_HLP_NO_LOCK_PREFIX();
            fAccess = IEM_ACCESS_DATA_R;
        }
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,                 0);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEM_MC_ARG_CONST(uint8_t,   u8Src, /*=*/ u8Imm,     1);

        IEM_MC_MEM_MAP(pu8Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, fAccess);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x81. */
FNIEMOP_DEF(iemOp_Grp1_Ev_Iz)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_MNEMONIC2("add\0or\0\0adc\0sbb\0and\0sub\0xor\0cmp" + ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)*4, "Ev,Iz");
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[(bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK];

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register target */
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src, /*=*/ u16Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU16)
                    fAccess = IEM_ACCESS_DATA_RW;
                else
                {   /* CMP, TEST */
                    IEMOP_HLP_NO_LOCK_PREFIX();
                    fAccess = IEM_ACCESS_DATA_R;
                }
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG(uint16_t,        u16Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            break;
        }

        case IEMMODE_32BIT:
        {
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register target */
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src, /*=*/ u32Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU32)
                    fAccess = IEM_ACCESS_DATA_RW;
                else
                {   /* CMP, TEST */
                    IEMOP_HLP_NO_LOCK_PREFIX();
                    fAccess = IEM_ACCESS_DATA_R;
                }
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG(uint32_t,        u32Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            break;
        }

        case IEMMODE_64BIT:
        {
            if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
            {
                /* register target */
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src, /*=*/ u64Imm,   1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);

                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            else
            {
                /* memory target */
                uint32_t fAccess;
                if (pImpl->pfnLockedU64)
                    fAccess = IEM_ACCESS_DATA_RW;
                else
                {   /* CMP */
                    IEMOP_HLP_NO_LOCK_PREFIX();
                    fAccess = IEM_ACCESS_DATA_R;
                }
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG(uint64_t,        u64Src,                 1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
            }
            break;
        }
    }
    return VINF_SUCCESS;
}


/** Opcode 0x82. */
    FNIEMOP_DEF(iemOp_Grp1_Eb_Ib_82)
{
    IEMOP_HLP_NO_64BIT(); /** @todo do we need to decode the whole instruction or is this ok? */
    return FNIEMOP_CALL(iemOp_Grp1_Eb_Ib_80);
}


/** Opcode 0x83. */
FNIEMOP_DEF(iemOp_Grp1_Ev_Ib)
{
    uint8_t bRm;   IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_MNEMONIC2("add\0or\0\0adc\0sbb\0and\0sub\0xor\0cmp" + ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)*4, "Ev,Ib");
    PCIEMOPBINSIZES pImpl = g_apIemImplGrp1[(bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK];

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /*
         * Register target
         */
        IEMOP_HLP_NO_LOCK_PREFIX();
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src, /*=*/ (int8_t)u8Imm,1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                    2);

                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }
        }
    }
    else
    {
        /*
         * Memory target.
         */
        uint32_t fAccess;
        if (pImpl->pfnLockedU16)
            fAccess = IEM_ACCESS_DATA_RW;
        else
        {   /* CMP */
            IEMOP_HLP_NO_LOCK_PREFIX();
            fAccess = IEM_ACCESS_DATA_R;
        }

        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                    0);
                IEM_MC_ARG(uint16_t,        u16Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
                IEM_MC_ASSIGN(u16Src, (int8_t)u8Imm);
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                    0);
                IEM_MC_ARG(uint32_t,        u32Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
                IEM_MC_ASSIGN(u32Src, (int8_t)u8Imm);
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                    0);
                IEM_MC_ARG(uint64_t,        u64Src,                     1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,            2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
                IEM_MC_ASSIGN(u64Src, (int8_t)u8Imm);
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
            }
        }
    }
    return VINF_SUCCESS;
}


/** Opcode 0x84. */
FNIEMOP_DEF(iemOp_test_Eb_Gb)
{
    IEMOP_MNEMONIC("test Eb,Gb");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo do we have to decode the whole instruction first?  */
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_r8, &g_iemAImpl_test);
}


/** Opcode 0x85. */
FNIEMOP_DEF(iemOp_test_Ev_Gv)
{
    IEMOP_MNEMONIC("test Ev,Gv");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo do we have to decode the whole instruction first?  */
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rm_rv, &g_iemAImpl_test);
}


/** Opcode 0x86. */
FNIEMOP_STUB(iemOp_xchg_Eb_Gb);
/** Opcode 0x87. */
FNIEMOP_STUB(iemOp_xchg_Ev_Gv);


/** Opcode 0x88. */
FNIEMOP_DEF(iemOp_mov_Eb_Gb)
{
    IEMOP_MNEMONIC("mov Eb,Gb");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_FETCH_GREG_U8(u8Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_STORE_GREG_U8((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u8Value);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're writing a register to memory.
         */
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_GREG_U8(u8Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
        IEM_MC_STORE_MEM_U8(pIemCpu->iEffSeg, GCPtrEffDst, u8Value);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;

}


/** Opcode 0x89. */
FNIEMOP_DEF(iemOp_mov_Ev_Gv)
{
    IEMOP_MNEMONIC("mov Ev,Gv");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_GREG_U16(u16Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_GREG_U16((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u16Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_GREG_U32(u32Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_GREG_U32((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u32Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_GREG_U64(u64Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_GREG_U64((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u64Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're writing a register to memory.
         */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U16(u16Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_MEM_U16(pIemCpu->iEffSeg, GCPtrEffDst, u16Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U32(u32Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_MEM_U32(pIemCpu->iEffSeg, GCPtrEffDst, u32Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U64(u64Value, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg);
                IEM_MC_STORE_MEM_U64(pIemCpu->iEffSeg, GCPtrEffDst, u64Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/** Opcode 0x8a. */
FNIEMOP_DEF(iemOp_mov_Gb_Eb)
{
    IEMOP_MNEMONIC("mov Gb,Eb");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_FETCH_GREG_U8(u8Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_STORE_GREG_U8(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u8Value);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're loading a register from memory.
         */
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint8_t, u8Value);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_MEM_U8(u8Value, pIemCpu->iEffSeg, GCPtrEffDst);
        IEM_MC_STORE_GREG_U8(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u8Value);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x8b. */
FNIEMOP_DEF(iemOp_mov_Gv_Ev)
{
    IEMOP_MNEMONIC("mov Gv,Ev");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_GREG_U16(u16Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_GREG_U32(u32Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_GREG_U64(u64Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're loading a register from memory.
         */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
Log(("GCPtrEffDst=%RGv\n", GCPtrEffDst));
                IEM_MC_FETCH_MEM_U16(u16Value, pIemCpu->iEffSeg, GCPtrEffDst);
Log(("u16Value=%#x\n", u16Value));
                IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u16Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U32(u32Value, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u32Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U64(u64Value, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, u64Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/** Opcode 0x8c. */
FNIEMOP_DEF(iemOp_mov_Ev_Sw)
{
    IEMOP_MNEMONIC("mov Ev,Sw");

    uint8_t bRm;
    IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * Check that the destination register exists. The REX.R prefix is ignored.
     */
    uint8_t const iSegReg = ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK);
    if (   iSegReg > X86_SREG_GS)
        return IEMOP_RAISE_INVALID_OPCODE(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     * In that case, the operand size is respected and the upper bits are
     * cleared (starting with some pentium).
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Value);
                IEM_MC_FETCH_SREG_U16(u16Value, iSegReg);
                IEM_MC_STORE_GREG_U16((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u16Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Value);
                IEM_MC_FETCH_SREG_U32_ZX(u32Value, iSegReg);
                IEM_MC_STORE_GREG_U32((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u32Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Value);
                IEM_MC_FETCH_SREG_U64_ZX(u64Value, iSegReg);
                IEM_MC_STORE_GREG_U64((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u64Value);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're saving the register to memory.  The access is word sized
         * regardless of operand size prefixes.
         */
#if 0 /* not necessary */
        pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_16BIT;
#endif
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(uint16_t,  u16Value);
        IEM_MC_LOCAL(RTGCPTR,   GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_SREG_U16(u16Value, iSegReg);
        IEM_MC_STORE_MEM_U16(pIemCpu->iEffSeg, GCPtrEffDst, u16Value);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}




/** Opcode 0x8d. */
FNIEMOP_DEF(iemOp_lea_Gv_M)
{
    IEMOP_MNEMONIC("lea Gv,M");
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
        return IEMOP_RAISE_INVALID_LOCK_PREFIX(); /* no register form */

    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_STORE_GREG_U16(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, GCPtrEffSrc);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_STORE_GREG_U32(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, GCPtrEffSrc);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_STORE_GREG_U64(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pIemCpu->uRexReg, GCPtrEffSrc);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_5);
}


/** Opcode 0x8e. */
FNIEMOP_DEF(iemOp_mov_Sw_Ev)
{
    IEMOP_MNEMONIC("mov Sw,Ev");

    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * The practical operand size is 16-bit.
     */
#if 0 /* not necessary */
    pIemCpu->enmEffOpSize = pIemCpu->enmDefOpSize = IEMMODE_16BIT;
#endif

    /*
     * Check that the destination register exists and can be used with this
     * instruction.  The REX.R prefix is ignored.
     */
    uint8_t const iSegReg = ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK);
    if (   iSegReg == X86_SREG_CS
        || iSegReg > X86_SREG_GS)
        return IEMOP_RAISE_INVALID_OPCODE(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEM_MC_BEGIN(2, 0);
        IEM_MC_ARG_CONST(uint8_t, iSRegArg, iSegReg, 0);
        IEM_MC_ARG(uint16_t,      u16Value,          1);
        IEM_MC_FETCH_GREG_U16(u16Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_CALL_CIMPL_2(iemCImpl_LoadSReg, iSRegArg, u16Value);
        IEM_MC_END();
    }
    else
    {
        /*
         * We're loading the register from memory.  The access is word sized
         * regardless of operand size prefixes.
         */
        IEM_MC_BEGIN(2, 1);
        IEM_MC_ARG_CONST(uint8_t, iSRegArg, iSegReg, 0);
        IEM_MC_ARG(uint16_t,      u16Value,          1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_MEM_U16(u16Value, pIemCpu->iEffSeg, GCPtrEffDst);
        IEM_MC_CALL_CIMPL_2(iemCImpl_LoadSReg, iSRegArg, u16Value);
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0x8f. */
FNIEMOP_STUB(iemOp_pop_Ev);


/**
 * Common 'xchg reg,rAX' helper.
 */
FNIEMOP_DEF_1(iemOpCommonXchgGRegRax, uint8_t, iReg)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    iReg |= pIemCpu->uRexB;
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint16_t, u16Tmp1);
            IEM_MC_LOCAL(uint16_t, u16Tmp2);
            IEM_MC_FETCH_GREG_U16(u16Tmp1, iReg);
            IEM_MC_FETCH_GREG_U16(u16Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U16(X86_GREG_xAX, u16Tmp1);
            IEM_MC_STORE_GREG_U16(iReg,         u16Tmp2);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint32_t, u32Tmp1);
            IEM_MC_LOCAL(uint32_t, u32Tmp2);
            IEM_MC_FETCH_GREG_U32(u32Tmp1, iReg);
            IEM_MC_FETCH_GREG_U32(u32Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U32(X86_GREG_xAX, u32Tmp1);
            IEM_MC_STORE_GREG_U32(iReg,         u32Tmp2);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint64_t, u64Tmp1);
            IEM_MC_LOCAL(uint64_t, u64Tmp2);
            IEM_MC_FETCH_GREG_U64(u64Tmp1, iReg);
            IEM_MC_FETCH_GREG_U64(u64Tmp2, X86_GREG_xAX);
            IEM_MC_STORE_GREG_U64(X86_GREG_xAX, u64Tmp1);
            IEM_MC_STORE_GREG_U64(iReg,         u64Tmp2);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0x90. */
FNIEMOP_DEF(iemOp_nop)
{
    /* R8/R8D and RAX/EAX can be exchanged. */
    if (pIemCpu->fPrefixes & IEM_OP_PRF_REX_B)
    {
        IEMOP_MNEMONIC("xchg r8,rAX");
        return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xAX);
    }

    if (pIemCpu->fPrefixes & IEM_OP_PRF_LOCK)
        IEMOP_MNEMONIC("pause");
    else
        IEMOP_MNEMONIC("nop");
    IEM_MC_BEGIN(0, 0);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0x91. */
FNIEMOP_DEF(iemOp_xchg_eCX_eAX)
{
    IEMOP_MNEMONIC("xchg rCX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xCX);
}


/** Opcode 0x92. */
FNIEMOP_DEF(iemOp_xchg_eDX_eAX)
{
    IEMOP_MNEMONIC("xchg rDX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xDX);
}


/** Opcode 0x93. */
FNIEMOP_DEF(iemOp_xchg_eBX_eAX)
{
    IEMOP_MNEMONIC("xchg rBX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xBX);
}


/** Opcode 0x94. */
FNIEMOP_DEF(iemOp_xchg_eSP_eAX)
{
    IEMOP_MNEMONIC("xchg rSX,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xSP);
}


/** Opcode 0x95. */
FNIEMOP_DEF(iemOp_xchg_eBP_eAX)
{
    IEMOP_MNEMONIC("xchg rBP,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xBP);
}


/** Opcode 0x96. */
FNIEMOP_DEF(iemOp_xchg_eSI_eAX)
{
    IEMOP_MNEMONIC("xchg rSI,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xSI);
}


/** Opcode 0x97. */
FNIEMOP_DEF(iemOp_xchg_eDI_eAX)
{
    IEMOP_MNEMONIC("xchg rDI,rAX");
    return FNIEMOP_CALL_1(iemOpCommonXchgGRegRax, X86_GREG_xDI);
}


/** Opcode 0x98. */
FNIEMOP_STUB(iemOp_cbw);
/** Opcode 0x99. */
FNIEMOP_STUB(iemOp_cwd);
/** Opcode 0x9a. */
FNIEMOP_STUB(iemOp_call_Ap);
/** Opcode 0x9b. */
FNIEMOP_STUB(iemOp_wait);


/** Opcode 0x9c. */
FNIEMOP_DEF(iemOp_pushf_Fv)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_pushf, pIemCpu->enmEffOpSize);
}


/** Opcode 0x9d. */
FNIEMOP_DEF(iemOp_popf_Fv)
{
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_popf, pIemCpu->enmEffOpSize);
}


/** Opcode 0x9e. */
FNIEMOP_STUB(iemOp_sahf);
/** Opcode 0x9f. */
FNIEMOP_STUB(iemOp_lahf);

/**
 * Macro used by iemOp_mov_Al_Ob, iemOp_mov_rAX_Ov, iemOp_mov_Ob_AL and
 * iemOp_mov_Ov_rAX to fetch the moffsXX bit of the opcode and fend of lock
 * prefixes.  Will return on failures.
 * @param   a_GCPtrMemOff   The variable to store the offset in.
 */
#define IEMOP_FETCH_MOFFS_XX(a_GCPtrMemOff) \
    do \
    { \
        switch (pIemCpu->enmEffAddrMode) \
        { \
            case IEMMODE_16BIT: \
            { \
                uint16_t u16Off; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Off); \
                (a_GCPtrMemOff) = u16Off; \
                break; \
            } \
            case IEMMODE_32BIT: \
            { \
                uint32_t u32Off; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Off); \
                (a_GCPtrMemOff) = u32Off; \
                break; \
            } \
            case IEMMODE_64BIT: \
                IEM_OPCODE_GET_NEXT_U64(pIemCpu, &(a_GCPtrMemOff)); \
                break; \
            IEM_NOT_REACHED_DEFAULT_CASE_RET(); \
        } \
        IEMOP_HLP_NO_LOCK_PREFIX(); \
    } while (0)

/** Opcode 0xa0. */
FNIEMOP_DEF(iemOp_mov_Al_Ob)
{
    /*
     * Get the offset and fend of lock prefixes.
     */
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Fetch AL.
     */
    IEM_MC_BEGIN(0,1);
    IEM_MC_LOCAL(uint8_t, u8Tmp);
    IEM_MC_FETCH_MEM_U8(u8Tmp, pIemCpu->iEffSeg, GCPtrMemOff);
    IEM_MC_STORE_GREG_U8(X86_GREG_xAX, u8Tmp);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xa1. */
FNIEMOP_DEF(iemOp_mov_rAX_Ov)
{
    /*
     * Get the offset and fend of lock prefixes.
     */
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Fetch rAX.
     */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint16_t, u16Tmp);
            IEM_MC_FETCH_MEM_U16(u16Tmp, pIemCpu->iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U16(X86_GREG_xAX, u16Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint32_t, u32Tmp);
            IEM_MC_FETCH_MEM_U32(u32Tmp, pIemCpu->iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U32(X86_GREG_xAX, u32Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint64_t, u64Tmp);
            IEM_MC_FETCH_MEM_U64(u64Tmp, pIemCpu->iEffSeg, GCPtrMemOff);
            IEM_MC_STORE_GREG_U64(X86_GREG_xAX, u64Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xa2. */
FNIEMOP_DEF(iemOp_mov_Ob_AL)
{
    /*
     * Get the offset and fend of lock prefixes.
     */
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Store AL.
     */
    IEM_MC_BEGIN(0,1);
    IEM_MC_LOCAL(uint8_t, u8Tmp);
    IEM_MC_FETCH_GREG_U8(u8Tmp, X86_GREG_xAX);
    IEM_MC_STORE_MEM_U8(pIemCpu->iEffSeg, GCPtrMemOff, u8Tmp);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xa3. */
FNIEMOP_DEF(iemOp_mov_Ov_rAX)
{
    /*
     * Get the offset and fend of lock prefixes.
     */
    RTGCPTR GCPtrMemOff;
    IEMOP_FETCH_MOFFS_XX(GCPtrMemOff);

    /*
     * Store rAX.
     */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint16_t, u16Tmp);
            IEM_MC_FETCH_GREG_U16(u16Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U16(pIemCpu->iEffSeg, GCPtrMemOff, u16Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint32_t, u32Tmp);
            IEM_MC_FETCH_GREG_U32(u32Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U32(pIemCpu->iEffSeg, GCPtrMemOff, u32Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,1);
            IEM_MC_LOCAL(uint64_t, u64Tmp);
            IEM_MC_FETCH_GREG_U64(u64Tmp, X86_GREG_xAX);
            IEM_MC_STORE_MEM_U64(pIemCpu->iEffSeg, GCPtrMemOff, u64Tmp);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}

/** Macro used by iemOp_movsb_Xb_Yb and iemOp_movswd_Xv_Yv */
#define IEM_MOVS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(0, 2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue); \
        IEM_MC_LOCAL(uint##AddrBits##_t, uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits(uAddr, X86_GREG_xSI); \
        IEM_MC_FETCH_MEM_U##ValBits(uValue, pIemCpu->iEffSeg, uAddr); \
        IEM_MC_FETCH_GREG_U##AddrBits(uAddr, X86_GREG_xDI); \
        IEM_MC_STORE_MEM_U##ValBits(X86_SREG_ES, uAddr, uValue); \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xSI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP(); \
        IEM_MC_END(); \


/** Opcode 0xa4. */
FNIEMOP_DEF(iemOp_movsb_Xb_Yb)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep movsb Xb,Yb");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr16, pIemCpu->iEffSeg);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr32, pIemCpu->iEffSeg);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op8_addr64, pIemCpu->iEffSeg);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC("movsb Xb,Yb");

    /*
     * Sharing case implementation with movs[wdq] below.
     */
    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_MOVS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_MOVS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_MOVS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xa5. */
FNIEMOP_DEF(iemOp_movswd_Xv_Yv)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep movs Xv,Yv");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr16, pIemCpu->iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr32, pIemCpu->iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op16_addr64, pIemCpu->iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr16, pIemCpu->iEffSeg);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr32, pIemCpu->iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op32_addr64, pIemCpu->iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_INTERNAL_ERROR_3);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op64_addr32, pIemCpu->iEffSeg);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_rep_movs_op64_addr64, pIemCpu->iEffSeg);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC("movs Xv,Yv");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with movsb.
     */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_MOVS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_MOVS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_INTERNAL_ERROR_4); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_MOVS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_MOVS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    return VINF_SUCCESS;
}

#undef IEM_MOVS_CASE

/** Opcode 0xa6. */
FNIEMOP_STUB(iemOp_cmpsb_Xb_Yb);
/** Opcode 0xa7. */
FNIEMOP_STUB(iemOp_cmpswd_Xv_Yv);


/** Opcode 0xa8. */
FNIEMOP_DEF(iemOp_test_AL_Ib)
{
    IEMOP_MNEMONIC("test al,Ib");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_AL_Ib, &g_iemAImpl_test);
}


/** Opcode 0xa9. */
FNIEMOP_DEF(iemOp_test_eAX_Iz)
{
    IEMOP_MNEMONIC("test rAX,Iz");
    return FNIEMOP_CALL_1(iemOpHlpBinaryOperator_rAX_Iz, &g_iemAImpl_test);
}


/** Macro used by iemOp_stosb_Yb_AL and iemOp_stoswd_Yv_eAX */
#define IEM_STOS_CASE(ValBits, AddrBits) \
        IEM_MC_BEGIN(0, 2); \
        IEM_MC_LOCAL(uint##ValBits##_t, uValue); \
        IEM_MC_LOCAL(uint##AddrBits##_t, uAddr); \
        IEM_MC_FETCH_GREG_U##ValBits(uValue, X86_GREG_xAX); \
        IEM_MC_FETCH_GREG_U##AddrBits(uAddr,  X86_GREG_xDI); \
        IEM_MC_STORE_MEM_U##ValBits(X86_SREG_ES, uAddr, uValue); \
        IEM_MC_IF_EFL_BIT_SET(X86_EFL_DF) { \
            IEM_MC_SUB_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ELSE() { \
            IEM_MC_ADD_GREG_U##AddrBits(X86_GREG_xDI, ValBits / 8); \
        } IEM_MC_ENDIF(); \
        IEM_MC_ADVANCE_RIP(); \
        IEM_MC_END(); \

/** Opcode 0xaa. */
FNIEMOP_DEF(iemOp_stosb_Yb_AL)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep stos Yb,al");
        switch (pIemCpu->enmEffAddrMode)
        {
            case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m16);
            case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m32);
            case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_al_m64);
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC("stos Yb,al");

    /*
     * Sharing case implementation with stos[wdq] below.
     */
    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT: IEM_STOS_CASE(8, 16); break;
        case IEMMODE_32BIT: IEM_STOS_CASE(8, 32); break;
        case IEMMODE_64BIT: IEM_STOS_CASE(8, 64); break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xab. */
FNIEMOP_DEF(iemOp_stoswd_Yv_eAX)
{
    IEMOP_HLP_NO_LOCK_PREFIX();

    /*
     * Use the C implementation if a repeate prefix is encountered.
     */
    if (pIemCpu->fPrefixes & (IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ))
    {
        IEMOP_MNEMONIC("rep stos Yv,rAX");
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_ax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
                break;
            case IEMMODE_32BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m16);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_eax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            case IEMMODE_64BIT:
                switch (pIemCpu->enmEffAddrMode)
                {
                    case IEMMODE_16BIT: AssertFailedReturn(VERR_INTERNAL_ERROR_3);
                    case IEMMODE_32BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_rax_m32);
                    case IEMMODE_64BIT: return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_stos_rax_m64);
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    IEMOP_MNEMONIC("stos Yv,rAX");

    /*
     * Annoying double switch here.
     * Using ugly macro for implementing the cases, sharing it with stosb.
     */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_STOS_CASE(16, 16); break;
                case IEMMODE_32BIT: IEM_STOS_CASE(16, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(16, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_32BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: IEM_STOS_CASE(32, 16); break;
                case IEMMODE_32BIT: IEM_STOS_CASE(32, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(32, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;

        case IEMMODE_64BIT:
            switch (pIemCpu->enmEffAddrMode)
            {
                case IEMMODE_16BIT: AssertFailedReturn(VERR_INTERNAL_ERROR_4); /* cannot be encoded */ break;
                case IEMMODE_32BIT: IEM_STOS_CASE(64, 32); break;
                case IEMMODE_64BIT: IEM_STOS_CASE(64, 64); break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    return VINF_SUCCESS;
}

#undef IEM_STOS_CASE

/** Opcode 0xac. */
FNIEMOP_STUB(iemOp_lodsb_AL_Xb);
/** Opcode 0xad. */
FNIEMOP_STUB(iemOp_lodswd_eAX_Xv);
/** Opcode 0xae. */
FNIEMOP_STUB(iemOp_scasb_AL_Xb);
/** Opcode 0xaf. */
FNIEMOP_STUB(iemOp_scaswd_eAX_Xv);

/**
 * Common 'mov r8, imm8' helper.
 */
FNIEMOP_DEF_1(iemOpCommonMov_r8_Ib, uint8_t, iReg)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(0, 1);
    IEM_MC_LOCAL_CONST(uint8_t, u8Value,/*=*/ u8Imm);
    IEM_MC_STORE_GREG_U8(iReg, u8Value);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();

    return VINF_SUCCESS;
}


/** Opcode 0xb0. */
FNIEMOP_DEF(iemOp_mov_AL_Ib)
{
    IEMOP_MNEMONIC("mov AL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xAX);
}


/** Opcode 0xb1. */
FNIEMOP_DEF(iemOp_CL_Ib)
{
    IEMOP_MNEMONIC("mov CL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xCX);
}


/** Opcode 0xb2. */
FNIEMOP_DEF(iemOp_DL_Ib)
{
    IEMOP_MNEMONIC("mov DL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xDX);
}


/** Opcode 0xb3. */
FNIEMOP_DEF(iemOp_BL_Ib)
{
    IEMOP_MNEMONIC("mov BL,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xBX);
}


/** Opcode 0xb4. */
FNIEMOP_DEF(iemOp_mov_AH_Ib)
{
    IEMOP_MNEMONIC("mov AH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xSP);
}


/** Opcode 0xb5. */
FNIEMOP_DEF(iemOp_CH_Ib)
{
    IEMOP_MNEMONIC("mov CH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xBP);
}


/** Opcode 0xb6. */
FNIEMOP_DEF(iemOp_DH_Ib)
{
    IEMOP_MNEMONIC("mov DH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xSI);
}


/** Opcode 0xb7. */
FNIEMOP_DEF(iemOp_BH_Ib)
{
    IEMOP_MNEMONIC("mov BH,Ib");
    return FNIEMOP_CALL_1(iemOpCommonMov_r8_Ib, X86_GREG_xDI);
}


/**
 * Common 'mov regX,immX' helper.
 */
FNIEMOP_DEF_1(iemOpCommonMov_Rv_Iv, uint8_t, iReg)
{
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint16_t, u16Value,/*=*/ u16Imm);
            IEM_MC_STORE_GREG_U16(iReg, u16Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint32_t, u32Value,/*=*/ u32Imm);
            IEM_MC_STORE_GREG_U32(iReg, u32Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
        }
        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_U64(pIemCpu, &u64Imm);
            IEMOP_HLP_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL_CONST(uint64_t, u64Value,/*=*/ u64Imm);
            IEM_MC_STORE_GREG_U64(iReg, u64Value);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            break;
        }
    }

    return VINF_SUCCESS;
}


/** Opcode 0xb8. */
FNIEMOP_DEF(iemOp_eAX_Iv)
{
    IEMOP_MNEMONIC("mov rAX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xAX);
}


/** Opcode 0xb9. */
FNIEMOP_DEF(iemOp_eCX_Iv)
{
    IEMOP_MNEMONIC("mov rCX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xCX);
}


/** Opcode 0xba. */
FNIEMOP_DEF(iemOp_eDX_Iv)
{
    IEMOP_MNEMONIC("mov rDX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xDX);
}


/** Opcode 0xbb. */
FNIEMOP_DEF(iemOp_eBX_Iv)
{
    IEMOP_MNEMONIC("mov rBX,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xBX);
}


/** Opcode 0xbc. */
FNIEMOP_DEF(iemOp_eSP_Iv)
{
    IEMOP_MNEMONIC("mov rSP,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xSP);
}


/** Opcode 0xbd. */
FNIEMOP_DEF(iemOp_eBP_Iv)
{
    IEMOP_MNEMONIC("mov rBP,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xBP);
}


/** Opcode 0xbe. */
FNIEMOP_DEF(iemOp_eSI_Iv)
{
    IEMOP_MNEMONIC("mov rSI,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xSI);
}


/** Opcode 0xbf. */
FNIEMOP_DEF(iemOp_eDI_Iv)
{
    IEMOP_MNEMONIC("mov rDI,IV");
    return FNIEMOP_CALL_1(iemOpCommonMov_Rv_Iv, X86_GREG_xDI);
}


/** Opcode 0xc0. */
FNIEMOP_DEF(iemOp_Grp2_Eb_Ib)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Eb,Ib"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Eb,Ib"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Eb,Ib"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Eb,Ib"); break;
        case 4: pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Eb,Ib"); break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Eb,Ib"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Eb,Ib"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
        IEMOP_HLP_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,            0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
        IEM_MC_ARG(uint32_t *,      pEFlags,           2);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,   pu8Dst,    0);
        IEM_MC_ARG(uint8_t,     cShiftArg,  1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
        IEM_MC_ASSIGN(cShiftArg, cShift);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xc1. */
FNIEMOP_DEF(iemOp_Grp2_Ev_Ib)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Ev,Ib"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Ev,Ib"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Ev,Ib"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Ev,Ib"); break;
        case 4:
            pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Ev,Ib");
#ifdef IEM_VERIFICATION_MODE
            pIemCpu->fShlHack = true;
#endif
            break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Ev,Ib"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Ev,Ib"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
        IEMOP_HLP_NO_LOCK_PREFIX();
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg, cShift, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,  pu16Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,  pu32Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,  pu64Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint8_t cShift; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &cShift);
                IEM_MC_ASSIGN(cShiftArg, cShift);
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;


            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xc2. */
FNIEMOP_STUB(iemOp_retn_Iw);


/** Opcode 0xc3. */
FNIEMOP_DEF(iemOp_retn)
{
    IEMOP_MNEMONIC("retn");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint16_t, u16NewIP);
            IEM_MC_POP_U16(&u16NewIP);
            /** @todo This should raise GP(0) if u16NewIP > csHid.u32Limit.
             *        The intel manual does not indicate that this is the
             *        case for 32-bit or 64-bit (canonical check). Needs to
             *        be tested. */
            IEM_MC_SET_RIP_U16(u16NewIP);
            IEM_MC_END()
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint32_t, u32NewIP);
            IEM_MC_POP_U32(&u32NewIP);
            IEM_MC_SET_RIP_U32(u32NewIP);
            IEM_MC_END()
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 1);
            IEM_MC_LOCAL(uint64_t, u64NewIP);
            IEM_MC_POP_U64(&u64NewIP);
            IEM_MC_SET_RIP_U64(u64NewIP);
            IEM_MC_END()
            return VINF_SUCCESS;

        default:
            AssertFailedReturn(VERR_INTERNAL_ERROR_2);
    }
}


/** Opcode 0xc4. */
FNIEMOP_STUB(iemOp_les_Gv_Mp);
/** Opcode 0xc5. */
FNIEMOP_STUB(iemOp_lds_Gv_Mp);


/** Opcode 0xc6. */
FNIEMOP_DEF(iemOp_Grp11_Eb_Ib)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
    if ((bRm & X86_MODRM_REG_MASK) != (0 << X86_MODRM_REG_SHIFT)) /* only mov Eb,Ib in this group. */
        return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    IEMOP_MNEMONIC("mov Eb,Ib");

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEM_MC_BEGIN(0, 0);
        IEM_MC_STORE_GREG_U8((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u8Imm);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(0, 1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEM_MC_STORE_MEM_U8(pIemCpu->iEffSeg, GCPtrEffDst, u8Imm);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xc7. */
FNIEMOP_DEF(iemOp_Grp11_Ev_Iz)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
    if ((bRm & X86_MODRM_REG_MASK) != (0 << X86_MODRM_REG_SHIFT)) /* only mov Eb,Ib in this group. */
        return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    IEMOP_MNEMONIC("mov Ev,Iz");

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 0);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEM_MC_STORE_GREG_U16((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u16Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 0);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEM_MC_STORE_GREG_U32((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u32Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 0);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_U64(pIemCpu, &u64Imm);
                IEM_MC_STORE_GREG_U64((bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB, u64Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEM_MC_STORE_MEM_U16(pIemCpu->iEffSeg, GCPtrEffDst, u16Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEM_MC_STORE_MEM_U32(pIemCpu->iEffSeg, GCPtrEffDst, u32Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_U64(pIemCpu, &u64Imm);
                IEM_MC_STORE_MEM_U64(pIemCpu->iEffSeg, GCPtrEffDst, u64Imm);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}




/** Opcode 0xc8. */
FNIEMOP_STUB(iemOp_enter_Iw_Ib);
/** Opcode 0xc9. */
FNIEMOP_STUB(iemOp_leave);


/** Opcode 0xca. */
FNIEMOP_DEF(iemOp_retf_Iw)
{
    IEMOP_MNEMONIC("retf Iw");
    uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_retf, pIemCpu->enmEffOpSize, u16Imm);
}


/** Opcode 0xcb. */
FNIEMOP_DEF(iemOp_retf)
{
    IEMOP_MNEMONIC("retf");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_retf, pIemCpu->enmEffOpSize, 0);
}


/** Opcode 0xcc. */
FNIEMOP_DEF(iemOp_int_3)
{
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_int, X86_XCPT_BP, true /*fIsBpInstr*/);
}


/** Opcode 0xcd. */
FNIEMOP_DEF(iemOp_int_Ib)
{
    uint8_t u8Int; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Int);
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_int, u8Int, false /*fIsBpInstr*/);
}


/** Opcode 0xce. */
FNIEMOP_DEF(iemOp_into)
{
    IEM_MC_BEGIN(2, 0);
    IEM_MC_ARG_CONST(uint8_t,   u8Int,      /*=*/ X86_XCPT_OF, 0);
    IEM_MC_ARG_CONST(bool,      fIsBpInstr, /*=*/ false, 1);
    IEM_MC_CALL_CIMPL_2(iemCImpl_int, u8Int, fIsBpInstr);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xcf. */
FNIEMOP_DEF(iemOp_iret)
{
    IEMOP_MNEMONIC("iret");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_iret, pIemCpu->enmEffOpSize);
}


/** Opcode 0xd0. */
FNIEMOP_DEF(iemOp_Grp2_Eb_1)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Eb,1"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Eb,1"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Eb,1"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Eb,1"); break;
        case 4: pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Eb,1"); break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Eb,1"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Eb,1"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        IEMOP_HLP_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=*/1,   1);
        IEM_MC_ARG(uint32_t *,      pEFlags,            2);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=*/1,   1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}



/** Opcode 0xd1. */
FNIEMOP_DEF(iemOp_Grp2_Ev_1)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Ev,1"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Ev,1"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Ev,1"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Ev,1"); break;
        case 4: pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Ev,1"); break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Ev,1"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Ev,1"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        IEMOP_HLP_NO_LOCK_PREFIX();
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,           0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1, 1);
                IEM_MC_ARG(uint32_t *,      pEFlags,           2);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG_CONST(uint8_t,   cShiftArg,/*=1*/1,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags,        2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xd2. */
FNIEMOP_DEF(iemOp_Grp2_Eb_CL)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Eb,CL"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Eb,CL"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Eb,CL"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Eb,CL"); break;
        case 4: pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Eb,CL"); break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Eb,CL"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Eb,CL"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        IEMOP_HLP_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,   pu8Dst,     0);
        IEM_MC_ARG(uint8_t,     cShiftArg,  1);
        IEM_MC_ARG(uint32_t *,  pEFlags,    2);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,   pu8Dst,          0);
        IEM_MC_ARG(uint8_t,     cShiftArg,       1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, cShiftArg, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xd3. */
FNIEMOP_DEF(iemOp_Grp2_Ev_CL)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    PCIEMOPSHIFTSIZES pImpl;
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0: pImpl = &g_iemAImpl_rol; IEMOP_MNEMONIC("rol Ev,CL"); break;
        case 1: pImpl = &g_iemAImpl_ror; IEMOP_MNEMONIC("ror Ev,CL"); break;
        case 2: pImpl = &g_iemAImpl_rcl; IEMOP_MNEMONIC("rcl Ev,CL"); break;
        case 3: pImpl = &g_iemAImpl_rcr; IEMOP_MNEMONIC("rcr Ev,CL"); break;
        case 4: pImpl = &g_iemAImpl_shl; IEMOP_MNEMONIC("shl Ev,CL"); break;
        case 5: pImpl = &g_iemAImpl_shr; IEMOP_MNEMONIC("shr Ev,CL"); break;
        case 7: pImpl = &g_iemAImpl_sar; IEMOP_MNEMONIC("sar Ev,CL"); break;
        case 6: return IEMOP_RAISE_INVALID_LOCK_PREFIX();
    }

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register */
        IEMOP_HLP_NO_LOCK_PREFIX();
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,    0);
                IEM_MC_ARG(uint8_t,         cShiftArg,  1);
                IEM_MC_ARG(uint32_t *,      pEFlags,    2);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,  pu16Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,  pu32Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,  pu64Dst,    0);
                IEM_MC_ARG(uint8_t,     cShiftArg,  1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_GREG_U8(cShiftArg, X86_GREG_xCX);
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, cShiftArg, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}

/** Opcode 0xd4. */
FNIEMOP_STUB(iemOp_aam_Ib);
/** Opcode 0xd5. */
FNIEMOP_STUB(iemOp_aad_Ib);
/** Opcode 0xd7. */
FNIEMOP_STUB(iemOp_xlat);
/** Opcode 0xd8. */
FNIEMOP_STUB(iemOp_EscF0);
/** Opcode 0xd9. */
FNIEMOP_STUB(iemOp_EscF1);
/** Opcode 0xda. */
FNIEMOP_STUB(iemOp_EscF2);
/** Opcode 0xdb. */
FNIEMOP_STUB(iemOp_EscF3);
/** Opcode 0xdc. */
FNIEMOP_STUB(iemOp_EscF4);
/** Opcode 0xdd. */
FNIEMOP_STUB(iemOp_EscF5);
/** Opcode 0xde. */
FNIEMOP_STUB(iemOp_EscF6);
/** Opcode 0xdf. */
FNIEMOP_STUB(iemOp_EscF7);


/** Opcode 0xe0. */
FNIEMOP_DEF(iemOp_loopne_Jb)
{
    IEMOP_MNEMONIC("loopne Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xe1. */
FNIEMOP_DEF(iemOp_loope_Jb)
{
    IEMOP_MNEMONIC("loope Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(X86_EFL_ZF) {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xe2. */
FNIEMOP_DEF(iemOp_loop_Jb)
{
    IEMOP_MNEMONIC("loop Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    /** @todo Check out the #GP case if EIP < CS.Base or EIP > CS.Limit when
     * using the 32-bit operand size override.  How can that be restarted?  See
     * weird pseudo code in intel manual. */
    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U16(X86_GREG_xCX, 1);
            IEM_MC_IF_CX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U32(X86_GREG_xCX, 1);
            IEM_MC_IF_ECX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_SUB_GREG_U64(X86_GREG_xCX, 1);
            IEM_MC_IF_RCX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xe3. */
FNIEMOP_DEF(iemOp_jecxz_Jb)
{
    IEMOP_MNEMONIC("jecxz Jb");
    int8_t i8Imm; IEM_OPCODE_GET_NEXT_S8(pIemCpu, &i8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    switch (pIemCpu->enmEffAddrMode)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_CX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_ECX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0,0);
            IEM_MC_IF_RCX_IS_NZ() {
                IEM_MC_REL_JMP_S8(i8Imm);
            } IEM_MC_ELSE() {
                IEM_MC_ADVANCE_RIP();
            } IEM_MC_ENDIF();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xe4 */
FNIEMOP_DEF(iemOp_in_AL_Ib)
{
    IEMOP_MNEMONIC("in eAX,Ib");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_in, u8Imm, 1);
}


/** Opcode 0xe5 */
FNIEMOP_DEF(iemOp_in_eAX_Ib)
{
    IEMOP_MNEMONIC("in eAX,Ib");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_in, u8Imm, pIemCpu->enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xe6 */
FNIEMOP_DEF(iemOp_out_Ib_AL)
{
    IEMOP_MNEMONIC("out Ib,AL");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_out, u8Imm, 1);
}


/** Opcode 0xe7 */
FNIEMOP_DEF(iemOp_out_Ib_eAX)
{
    IEMOP_MNEMONIC("out Ib,eAX");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_out, u8Imm, pIemCpu->enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xe8. */
FNIEMOP_DEF(iemOp_call_Jv)
{
    IEMOP_MNEMONIC("call Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_16, (int32_t)u16Imm);
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_32, (int32_t)u32Imm);
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
            return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_call_rel_64, u64Imm);
        }
        default:
            AssertFailedReturn(VERR_INTERNAL_ERROR_3);
    }
}


/** Opcode 0xe9. */
FNIEMOP_DEF(iemOp_jmp_Jv)
{
    IEMOP_MNEMONIC("jmp Jv");
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
            IEM_MC_BEGIN(0, 0);
            IEM_MC_REL_JMP_S16((int16_t)u16Imm);
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
            IEM_MC_BEGIN(0, 0);
            IEM_MC_REL_JMP_S32((int32_t)u32Imm);
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xea. */
FNIEMOP_DEF(iemOp_jmp_Ap)
{
    IEMOP_MNEMONIC("jmp Ap");
    IEMOP_HLP_NO_64BIT();

    /* Decode the far pointer address and pass it on to the far call C implementation. */
    uint32_t offSeg;
    if (pIemCpu->enmEffOpSize != IEMMODE_16BIT)
        IEM_OPCODE_GET_NEXT_U32(pIemCpu, &offSeg);
    else
    {
        uint16_t offSeg16; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &offSeg16);
        offSeg = offSeg16;
    }
    uint16_t uSel;  IEM_OPCODE_GET_NEXT_U16(pIemCpu, &uSel);
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_2(iemCImpl_FarJmp, uSel, offSeg);
}


/** Opcode 0xeb. */
FNIEMOP_DEF(iemOp_jmp_Jb)
{
    IEMOP_MNEMONIC("jmp Jb");
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    IEM_MC_BEGIN(0, 0);
    IEM_MC_REL_JMP_S8((int8_t)u8Imm);
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xec */
FNIEMOP_DEF(iemOp_in_AL_DX)
{
    IEMOP_MNEMONIC("in  AL,DX");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_in_eAX_DX, 1);
}


/** Opcode 0xed */
FNIEMOP_DEF(iemOp_eAX_DX)
{
    IEMOP_MNEMONIC("in  eAX,DX");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_in_eAX_DX, pIemCpu->enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xee */
FNIEMOP_DEF(iemOp_out_DX_AL)
{
    IEMOP_MNEMONIC("out DX,AL");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_out_DX_eAX, 1);
}


/** Opcode 0xef */
FNIEMOP_DEF(iemOp_out_DX_eAX)
{
    IEMOP_MNEMONIC("out DX,eAX");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_1(iemCImpl_out_DX_eAX, pIemCpu->enmEffOpSize == IEMMODE_16BIT ? 2 : 4);
}


/** Opcode 0xf0. */
FNIEMOP_DEF(iemOp_lock)
{
    pIemCpu->fPrefixes |= IEM_OP_PRF_LOCK;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0xf2. */
FNIEMOP_DEF(iemOp_repne)
{
    /* This overrides any previous REPE prefix. */
    pIemCpu->fPrefixes &= ~IEM_OP_PRF_REPZ;
    pIemCpu->fPrefixes |= IEM_OP_PRF_REPNZ;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0xf3. */
FNIEMOP_DEF(iemOp_repe)
{
    /* This overrides any previous REPNE prefix. */
    pIemCpu->fPrefixes &= ~IEM_OP_PRF_REPNZ;
    pIemCpu->fPrefixes |= IEM_OP_PRF_REPZ;

    uint8_t b; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &b);
    return FNIEMOP_CALL(g_apfnOneByteMap[b]);
}


/** Opcode 0xf4. */
FNIEMOP_STUB(iemOp_hlt);
/** Opcode 0xf5. */
FNIEMOP_STUB(iemOp_cmc);


/**
 * Common implementation of 'inc/dec/not/neg Eb'.
 *
 * @param   bRm             The RM byte.
 * @param   pImpl           The instruction implementation.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryEb, uint8_t, bRm, PCIEMOPUNARYSIZES, pImpl)
{
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        IEM_MC_BEGIN(2, 0);
        IEM_MC_ARG(uint8_t *,   pu8Dst, 0);
        IEM_MC_ARG(uint32_t *,  pEFlags, 1);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU8, pu8Dst, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEM_MC_BEGIN(2, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,          0);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU8, pu8Dst, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU8, pu8Dst, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_RW);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/**
 * Common implementation of 'inc/dec/not/neg Ev'.
 *
 * @param   bRm             The RM byte.
 * @param   pImpl           The instruction implementation.
 */
FNIEMOP_DEF_2(iemOpCommonUnaryEv, uint8_t, bRm, PCIEMOPUNARYSIZES, pImpl)
{
    /* Registers are handled by a common worker. */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
        return FNIEMOP_CALL_2(iemOpCommonUnaryGReg, pImpl, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);

    /* Memory we do here. */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint16_t *,      pu16Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
            IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU16, pu16Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU16, pu16Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint32_t *,      pu32Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
            IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU32, pu32Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU32, pu32Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(2, 2);
            IEM_MC_ARG(uint64_t *,      pu64Dst,         0);
            IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags, 1);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
            IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_RW, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
            IEM_MC_FETCH_EFLAGS(EFlags);
            if (!(pIemCpu->fPrefixes & IEM_OP_PRF_LOCK))
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnNormalU64, pu64Dst, pEFlags);
            else
                IEM_MC_CALL_VOID_AIMPL_2(pImpl->pfnLockedU64, pu64Dst, pEFlags);

            IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_RW);
            IEM_MC_COMMIT_EFLAGS(EFlags);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xf6 /0. */
FNIEMOP_DEF_1(iemOp_grp3_test_Eb, uint8_t, bRm)
{
    IEMOP_MNEMONIC("test Eb,Ib");

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEMOP_HLP_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG_CONST(uint8_t,   u8Src,/*=*/u8Imm,   1);
        IEM_MC_ARG(uint32_t *,      pEFlags,            2);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u8, pu8Dst, u8Src, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
        IEM_MC_ARG(uint8_t,         u8Src,              1);
        IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        uint8_t u8Imm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &u8Imm);
        IEM_MC_ASSIGN(u8Src, u8Imm);
        IEM_MC_MEM_MAP(pu8Dst, IEM_ACCESS_DATA_R, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_EFLAGS(EFlags);
        IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, IEM_ACCESS_DATA_R);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xf7 /0. */
FNIEMOP_DEF_1(iemOp_grp3_test_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC("test Ev,Iv");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
                IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/u16Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u16, pu16Dst, u16Src, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_32BIT:
            {
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
                IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/u32Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u32, pu32Dst, u32Src, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_64BIT:
            {
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
                IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/u64Imm,     1);
                IEM_MC_ARG(uint32_t *,      pEFlags,                2);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u64, pu64Dst, u64Src, pEFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16Dst,            0);
                IEM_MC_ARG(uint16_t,        u16Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(pIemCpu, &u16Imm);
                IEM_MC_ASSIGN(u16Src, u16Imm);
                IEM_MC_MEM_MAP(pu16Dst, IEM_ACCESS_DATA_R, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_32BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32Dst,            0);
                IEM_MC_ARG(uint32_t,        u32Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(pIemCpu, &u32Imm);
                IEM_MC_ASSIGN(u32Src, u32Imm);
                IEM_MC_MEM_MAP(pu32Dst, IEM_ACCESS_DATA_R, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_64BIT:
            {
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64Dst,            0);
                IEM_MC_ARG(uint64_t,        u64Src,             1);
                IEM_MC_ARG_LOCAL_EFLAGS(    pEFlags, EFlags,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(pIemCpu, &u64Imm);
                IEM_MC_ASSIGN(u64Src, u64Imm);
                IEM_MC_MEM_MAP(pu64Dst, IEM_ACCESS_DATA_R, pIemCpu->iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_EFLAGS(EFlags);
                IEM_MC_CALL_VOID_AIMPL_3(iemAImpl_test_u64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, IEM_ACCESS_DATA_R);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                return VINF_SUCCESS;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}


/** Opcode 0xf6 /4, /5, /6 and /7. */
FNIEMOP_DEF_2(iemOpCommonGrp3MulDivEb, uint8_t, bRm, PFNIEMAIMPLMULDIVU8, pfnU8)
{
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
#ifdef IEM_VERIFICATION_MODE
    pIemCpu->fMulDivHack = true;
#endif

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        IEMOP_HLP_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint16_t *,      pu16AX,     0);
        IEM_MC_ARG(uint8_t,         u8Value,    1);
        IEM_MC_ARG(uint32_t *,      pEFlags,    2);
        IEM_MC_FETCH_GREG_U8(u8Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
        IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pfnU8, pu16AX, u8Value, pEFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /* memory access. */
        IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */

        IEM_MC_BEGIN(3, 1);
        IEM_MC_ARG(uint16_t *,      pu16AX,     0);
        IEM_MC_ARG(uint8_t,         u8Value,    1);
        IEM_MC_ARG(uint32_t *,      pEFlags,    2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
        IEM_MC_FETCH_MEM_U8(u8Value, pIemCpu->iEffSeg, GCPtrEffDst);
        IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pfnU8, pu16AX, u8Value, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/** Opcode 0xf7 /4, /5, /6 and /7. */
FNIEMOP_DEF_2(iemOpCommonGrp3MulDivEv, uint8_t, bRm, PCIEMOPMULDIVSIZES, pImpl)
{
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo should probably not be raised until we've fetched all the opcode bytes? */
#ifdef IEM_VERIFICATION_MODE
    pIemCpu->fMulDivHack = true;
#endif

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* register access */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *,      pu16AX,     0);
                IEM_MC_ARG(uint16_t *,      pu16DX,     1);
                IEM_MC_ARG(uint16_t,        u16Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U16(u16Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U16(pu16DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU16, pu16AX, pu16DX, u16Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_32BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *,      pu32AX,     0);
                IEM_MC_ARG(uint32_t *,      pu32DX,     1);
                IEM_MC_ARG(uint32_t,        u32Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U32(u32Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U32(pu32AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U32(pu32DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU32, pu32AX, pu32DX, u32Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_64BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *,      pu64AX,     0);
                IEM_MC_ARG(uint64_t *,      pu64DX,     1);
                IEM_MC_ARG(uint64_t,        u64Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_FETCH_GREG_U64(u64Value, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_REF_GREG_U64(pu64AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U64(pu64DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU64, pu64AX, pu64DX, u64Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* memory access. */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *,      pu16AX,     0);
                IEM_MC_ARG(uint16_t *,      pu16DX,     1);
                IEM_MC_ARG(uint16_t,        u16Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U16(u16Value, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U16(pu16AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U16(pu16DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU16, pu16AX, pu16DX, u16Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_32BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *,      pu32AX,     0);
                IEM_MC_ARG(uint32_t *,      pu32DX,     1);
                IEM_MC_ARG(uint32_t,        u32Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U32(u32Value, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U32(pu32AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U32(pu32DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU32, pu32AX, pu32DX, u32Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            case IEMMODE_64BIT:
            {
                IEMOP_HLP_NO_LOCK_PREFIX();
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *,      pu64AX,     0);
                IEM_MC_ARG(uint64_t *,      pu64DX,     1);
                IEM_MC_ARG(uint64_t,        u64Value,   2);
                IEM_MC_ARG(uint32_t *,      pEFlags,    3);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);
                IEM_MC_LOCAL(int32_t,       rc);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm);
                IEM_MC_FETCH_MEM_U64(u64Value, pIemCpu->iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U64(pu64AX, X86_GREG_xAX);
                IEM_MC_REF_GREG_U64(pu64DX, X86_GREG_xDX);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_AIMPL_4(rc, pImpl->pfnU64, pu64AX, pu64DX, u64Value, pEFlags);
                IEM_MC_IF_LOCAL_IS_Z(rc) {
                    IEM_MC_ADVANCE_RIP();
                } IEM_MC_ELSE() {
                    IEM_MC_RAISE_DIVIDE_ERROR();
                } IEM_MC_ENDIF();

                IEM_MC_END();
                return VINF_SUCCESS;
            }

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
}

/** Opcode 0xf6. */
FNIEMOP_DEF(iemOp_Grp3_Eb)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0:
            return FNIEMOP_CALL_1(iemOp_grp3_test_Eb, bRm);
        case 1:
            return IEMOP_RAISE_INVALID_LOCK_PREFIX();
        case 2:
            IEMOP_MNEMONIC("not Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_not);
        case 3:
            IEMOP_MNEMONIC("neg Eb");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_neg);
        case 4:
            IEMOP_MNEMONIC("mul Eb");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, &iemAImpl_mul_u8);
        case 5:
            IEMOP_MNEMONIC("imul Eb");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, &iemAImpl_imul_u8);
        case 6:
            IEMOP_MNEMONIC("div Eb");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, &iemAImpl_div_u8);
        case 7:
            IEMOP_MNEMONIC("idiv Eb");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEb, bRm, &iemAImpl_idiv_u8);
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xf7. */
FNIEMOP_DEF(iemOp_Grp3_Ev)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0:
            return FNIEMOP_CALL_1(iemOp_grp3_test_Ev, bRm);
        case 1:
            return IEMOP_RAISE_INVALID_LOCK_PREFIX();
        case 2:
            IEMOP_MNEMONIC("not Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_not);
        case 3:
            IEMOP_MNEMONIC("neg Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_neg);
        case 4:
            IEMOP_MNEMONIC("mul Ev");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, &g_iemAImpl_mul);
        case 5:
            IEMOP_MNEMONIC("imul Ev");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, &g_iemAImpl_imul);
        case 6:
            IEMOP_MNEMONIC("div Ev");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, &g_iemAImpl_div);
        case 7:
            IEMOP_MNEMONIC("idiv Ev");
            return FNIEMOP_CALL_2(iemOpCommonGrp3MulDivEv, bRm, &g_iemAImpl_idiv);
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcode 0xf8. */
FNIEMOP_DEF(iemOp_clc)
{
    IEMOP_MNEMONIC("clc");
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_CLEAR_EFL_BIT(X86_EFL_CF);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xf9. */
FNIEMOP_DEF(iemOp_stc)
{
    IEMOP_MNEMONIC("slc");
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_SET_EFL_BIT(X86_EFL_CF);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xfa. */
FNIEMOP_DEF(iemOp_cli)
{
    IEMOP_MNEMONIC("cli");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_cli);
}


FNIEMOP_DEF(iemOp_sti)
{
    IEMOP_MNEMONIC("sti");
    IEMOP_HLP_NO_LOCK_PREFIX();
    return IEM_MC_DEFER_TO_CIMPL_0(iemCImpl_sti);
}


/** Opcode 0xfc. */
FNIEMOP_DEF(iemOp_cld)
{
    IEMOP_MNEMONIC("cld");
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_CLEAR_EFL_BIT(X86_EFL_DF);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xfd. */
FNIEMOP_DEF(iemOp_std)
{
    IEMOP_MNEMONIC("std");
    IEMOP_HLP_NO_LOCK_PREFIX();
    IEM_MC_BEGIN(0, 0);
    IEM_MC_SET_EFL_BIT(X86_EFL_DF);
    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/** Opcode 0xfe. */
FNIEMOP_DEF(iemOp_Grp4)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0:
            IEMOP_MNEMONIC("inc Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_inc);
        case 1:
            IEMOP_MNEMONIC("dec Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEb, bRm, &g_iemAImpl_dec);
        default:
            IEMOP_MNEMONIC("grp4-ud");
            return IEMOP_RAISE_INVALID_OPCODE();
    }
}


/**
 * Opcode 0xff /2.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_calln_Ev, uint8_t, bRm)
{
    AssertFailed(); // FNIEMOP_STUB
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Opcode 0xff /3.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_callf_Ep, uint8_t, bRm)
{
    IEMOP_MNEMONIC("callf Ep");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo Too early? */

    /* Registers? How?? */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /** @todo How the heck does a 'callf eax' work? Probably just have to
         *        search the docs... */
        AssertFailedReturn(VERR_NOT_IMPLEMENTED);
    }

    /* Far pointer loaded from memory. */
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(3, 1);
            IEM_MC_ARG(uint16_t,        u16Sel,                         0);
            IEM_MC_ARG(uint16_t,        offSeg,                         1);
            IEM_MC_ARG_CONST(uint16_t,  enmEffOpSize, IEMMODE_16BIT,    2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_FETCH_MEM_U16(offSeg, pIemCpu->iEffSeg, GCPtrEffSrc);
            IEM_MC_FETCH_MEM_U16(u16Sel, pIemCpu->iEffSeg, GCPtrEffSrc + 2);
            IEM_MC_CALL_CIMPL_3(iemCImpl_callf, u16Sel, offSeg, IEMMODE_16BIT);
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            if (pIemCpu->enmCpuMode != IEMMODE_64BIT)
            {
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t,        u16Sel,                         0);
                IEM_MC_ARG(uint32_t,        offSeg,                         1);
                IEM_MC_ARG_CONST(uint16_t,  enmEffOpSize, IEMMODE_16BIT,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
                IEM_MC_FETCH_MEM_U32(offSeg, pIemCpu->iEffSeg, GCPtrEffSrc);
                IEM_MC_FETCH_MEM_U16(u16Sel, pIemCpu->iEffSeg, GCPtrEffSrc + 4);
                IEM_MC_CALL_CIMPL_3(iemCImpl_callf, u16Sel, offSeg, IEMMODE_16BIT);
                IEM_MC_END();
            }
            else
            {
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t,        u16Sel,                         0);
                IEM_MC_ARG(uint64_t,        offSeg,                         1);
                IEM_MC_ARG_CONST(uint16_t,  enmEffOpSize, IEMMODE_16BIT,    2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
                IEM_MC_FETCH_MEM_S32_SX_U64(offSeg, pIemCpu->iEffSeg, GCPtrEffSrc);
                IEM_MC_FETCH_MEM_U16(u16Sel, pIemCpu->iEffSeg, GCPtrEffSrc + 4);
                IEM_MC_CALL_CIMPL_3(iemCImpl_callf, u16Sel, offSeg, IEMMODE_16BIT);
                IEM_MC_END();
            }
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(3, 1);
            IEM_MC_ARG(uint16_t,        u16Sel,                         0);
            IEM_MC_ARG(uint64_t,        offSeg,                         1);
            IEM_MC_ARG_CONST(uint16_t,  enmEffOpSize, IEMMODE_16BIT,    2);
            IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_FETCH_MEM_U64(offSeg, pIemCpu->iEffSeg, GCPtrEffSrc);
            IEM_MC_FETCH_MEM_U16(u16Sel, pIemCpu->iEffSeg, GCPtrEffSrc + 8);
            IEM_MC_CALL_CIMPL_3(iemCImpl_callf, u16Sel, offSeg, IEMMODE_16BIT);
            IEM_MC_END();
            return VINF_SUCCESS;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/**
 * Opcode 0xff /4.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_jmpn_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC("callf Ep");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo Too early? */
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();

    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        /* The new RIP is taken from a register. */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint16_t, u16Target);
                IEM_MC_FETCH_GREG_U16(u16Target, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_SET_RIP_U16(u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint32_t, u32Target);
                IEM_MC_FETCH_GREG_U32(u32Target, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_SET_RIP_U32(u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 1);
                IEM_MC_LOCAL(uint64_t, u64Target);
                IEM_MC_FETCH_GREG_U64(u64Target, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);
                IEM_MC_SET_RIP_U64(u64Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    else
    {
        /* The new RIP is taken from a register. */
        switch (pIemCpu->enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint16_t, u16Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
                IEM_MC_FETCH_MEM_U16(u16Target, pIemCpu->iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U16(u16Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
                IEM_MC_FETCH_MEM_U32(u32Target, pIemCpu->iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U32(u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(0, 2);
                IEM_MC_LOCAL(uint32_t, u32Target);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffSrc);
                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
                IEM_MC_FETCH_MEM_U32(u32Target, pIemCpu->iEffSeg, GCPtrEffSrc);
                IEM_MC_SET_RIP_U32(u32Target);
                IEM_MC_END()
                return VINF_SUCCESS;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }

    }
}


/**
 * Opcode 0xff /5.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_jmpf_Ep, uint8_t, bRm)
{
    /* decode and use a C worker.  */
    AssertFailed(); // FNIEMOP_STUB
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Opcode 0xff /6.
 * @param   bRm             The RM byte.
 */
FNIEMOP_DEF_1(iemOp_Grp5_push_Ev, uint8_t, bRm)
{
    IEMOP_MNEMONIC("push Ev");
    IEMOP_HLP_NO_LOCK_PREFIX(); /** @todo Too early? */

    /* Registers are handled by a common worker. */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
        return FNIEMOP_CALL_1(iemOpCommonPushGReg, (bRm & X86_MODRM_RM_MASK) | pIemCpu->uRexB);

    /* Memory we do here. */
    IEMOP_HLP_DEFAULT_64BIT_OP_SIZE();
    switch (pIemCpu->enmEffOpSize)
    {
        case IEMMODE_16BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint16_t,  u16Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_FETCH_MEM_U16(u16Src, pIemCpu->iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U16(u16Src);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_32BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint32_t,  u32Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_FETCH_MEM_U32(u32Src, pIemCpu->iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U32(u32Src);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;

        case IEMMODE_64BIT:
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(uint64_t,  u64Src);
            IEM_MC_LOCAL(RTGCPTR,   GCPtrEffSrc);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm);
            IEM_MC_FETCH_MEM_U64(u64Src, pIemCpu->iEffSeg, GCPtrEffSrc);
            IEM_MC_PUSH_U64(u64Src);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
    }
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}


/** Opcode 0xff. */
FNIEMOP_DEF(iemOp_Grp5)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_BYTE(pIemCpu, &bRm);
    switch ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)
    {
        case 0:
            IEMOP_MNEMONIC("inc Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_inc);
        case 1:
            IEMOP_MNEMONIC("dec Ev");
            return FNIEMOP_CALL_2(iemOpCommonUnaryEv, bRm, &g_iemAImpl_dec);
        case 2:
            return FNIEMOP_CALL_1(iemOp_Grp5_calln_Ev, bRm);
        case 3:
            return FNIEMOP_CALL_1(iemOp_Grp5_callf_Ep, bRm);
        case 4:
            return FNIEMOP_CALL_1(iemOp_Grp5_jmpn_Ev, bRm);
        case 5:
            return FNIEMOP_CALL_1(iemOp_Grp5_jmpf_Ep, bRm);
        case 6:
            return FNIEMOP_CALL_1(iemOp_Grp5_push_Ev, bRm);
        case 7:
            IEMOP_MNEMONIC("grp5-ud");
            return IEMOP_RAISE_INVALID_OPCODE();
    }
    AssertFailedReturn(VERR_INTERNAL_ERROR_2);
}



const PFNIEMOP g_apfnOneByteMap[256] =
{
    /* 0x00 */  iemOp_add_Eb_Gb,        iemOp_add_Ev_Gv,        iemOp_add_Gb_Eb,        iemOp_add_Gv_Ev,
    /* 0x04 */  iemOp_add_Al_Ib,        iemOp_add_eAX_Iz,       iemOp_push_ES,          iemOp_pop_ES,
    /* 0x08 */  iemOp_or_Eb_Gb,         iemOp_or_Ev_Gv,         iemOp_or_Gb_Eb,         iemOp_or_Gv_Ev,
    /* 0x0c */  iemOp_or_Al_Ib,         iemOp_or_eAX_Iz,        iemOp_push_CS,          iemOp_2byteEscape,
    /* 0x10 */  iemOp_adc_Eb_Gb,        iemOp_adc_Ev_Gv,        iemOp_adc_Gb_Eb,        iemOp_adc_Gv_Ev,
    /* 0x14 */  iemOp_adc_Al_Ib,        iemOp_adc_eAX_Iz,       iemOp_push_SS,          iemOp_pop_SS,
    /* 0x18 */  iemOp_sbb_Eb_Gb,        iemOp_sbb_Ev_Gv,        iemOp_sbb_Gb_Eb,        iemOp_sbb_Gv_Ev,
    /* 0x1c */  iemOp_sbb_Al_Ib,        iemOp_sbb_eAX_Iz,       iemOp_push_DS,          iemOp_pop_DS,
    /* 0x20 */  iemOp_and_Eb_Gb,        iemOp_and_Ev_Gv,        iemOp_and_Gb_Eb,        iemOp_and_Gv_Ev,
    /* 0x24 */  iemOp_and_Al_Ib,        iemOp_and_eAX_Iz,       iemOp_seg_ES,           iemOp_daa,
    /* 0x28 */  iemOp_sub_Eb_Gb,        iemOp_sub_Ev_Gv,        iemOp_sub_Gb_Eb,        iemOp_sub_Gv_Ev,
    /* 0x2c */  iemOp_sub_Al_Ib,        iemOp_sub_eAX_Iz,       iemOp_seg_CS,           iemOp_das,
    /* 0x30 */  iemOp_xor_Eb_Gb,        iemOp_xor_Ev_Gv,        iemOp_xor_Gb_Eb,        iemOp_xor_Gv_Ev,
    /* 0x34 */  iemOp_xor_Al_Ib,        iemOp_xor_eAX_Iz,       iemOp_seg_SS,           iemOp_aaa,
    /* 0x38 */  iemOp_cmp_Eb_Gb,        iemOp_cmp_Ev_Gv,        iemOp_cmp_Gb_Eb,        iemOp_cmp_Gv_Ev,
    /* 0x3c */  iemOp_cmp_Al_Ib,        iemOp_cmp_eAX_Iz,       iemOp_seg_DS,           iemOp_aas,
    /* 0x40 */  iemOp_inc_eAX,          iemOp_inc_eCX,          iemOp_inc_eDX,          iemOp_inc_eBX,
    /* 0x44 */  iemOp_inc_eSP,          iemOp_inc_eBP,          iemOp_inc_eSI,          iemOp_inc_eDI,
    /* 0x48 */  iemOp_dec_eAX,          iemOp_dec_eCX,          iemOp_dec_eDX,          iemOp_dec_eBX,
    /* 0x4c */  iemOp_dec_eSP,          iemOp_dec_eBP,          iemOp_dec_eSI,          iemOp_dec_eDI,
    /* 0x50 */  iemOp_push_eAX,         iemOp_push_eCX,         iemOp_push_eDX,         iemOp_push_eBX,
    /* 0x54 */  iemOp_push_eSP,         iemOp_push_eBP,         iemOp_push_eSI,         iemOp_push_eDI,
    /* 0x58 */  iemOp_pop_eAX,          iemOp_pop_eCX,          iemOp_pop_eDX,          iemOp_pop_eBX,
    /* 0x5c */  iemOp_pop_eSP,          iemOp_pop_eBP,          iemOp_pop_eSI,          iemOp_pop_eDI,
    /* 0x60 */  iemOp_pusha,            iemOp_popa,             iemOp_bound_Gv_Ma,      iemOp_arpl_Ew_Gw,
    /* 0x64 */  iemOp_seg_FS,           iemOp_seg_GS,           iemOp_op_size,          iemOp_addr_size,
    /* 0x68 */  iemOp_push_Iz,          iemOp_imul_Gv_Ev_Iz,    iemOp_push_Ib,          iemOp_imul_Gv_Ev_Ib,
    /* 0x6c */  iemOp_insb_Yb_DX,       iemOp_inswd_Yv_DX,      iemOp_outsb_Yb_DX,      iemOp_outswd_Yv_DX,
    /* 0x70 */  iemOp_jo_Jb,            iemOp_jno_Jb,           iemOp_jc_Jb,            iemOp_jnc_Jb,
    /* 0x74 */  iemOp_je_Jb,            iemOp_jne_Jb,           iemOp_jbe_Jb,           iemOp_jnbe_Jb,
    /* 0x78 */  iemOp_js_Jb,            iemOp_jns_Jb,           iemOp_jp_Jb,            iemOp_jnp_Jb,
    /* 0x7c */  iemOp_jl_Jb,            iemOp_jnl_Jb,           iemOp_jle_Jb,           iemOp_jnle_Jb,
    /* 0x80 */  iemOp_Grp1_Eb_Ib_80,    iemOp_Grp1_Ev_Iz,       iemOp_Grp1_Eb_Ib_82,    iemOp_Grp1_Ev_Ib,
    /* 0x84 */  iemOp_test_Eb_Gb,       iemOp_test_Ev_Gv,       iemOp_xchg_Eb_Gb,       iemOp_xchg_Ev_Gv,
    /* 0x88 */  iemOp_mov_Eb_Gb,        iemOp_mov_Ev_Gv,        iemOp_mov_Gb_Eb,        iemOp_mov_Gv_Ev,
    /* 0x8c */  iemOp_mov_Ev_Sw,        iemOp_lea_Gv_M,         iemOp_mov_Sw_Ev,        iemOp_pop_Ev,
    /* 0x90 */  iemOp_nop,              iemOp_xchg_eCX_eAX,     iemOp_xchg_eDX_eAX,     iemOp_xchg_eBX_eAX,
    /* 0x94 */  iemOp_xchg_eSP_eAX,     iemOp_xchg_eBP_eAX,     iemOp_xchg_eSI_eAX,     iemOp_xchg_eDI_eAX,
    /* 0x98 */  iemOp_cbw,              iemOp_cwd,              iemOp_call_Ap,          iemOp_wait,
    /* 0x9c */  iemOp_pushf_Fv,         iemOp_popf_Fv,          iemOp_sahf,             iemOp_lahf,
    /* 0xa0 */  iemOp_mov_Al_Ob,        iemOp_mov_rAX_Ov,       iemOp_mov_Ob_AL,        iemOp_mov_Ov_rAX,
    /* 0xa4 */  iemOp_movsb_Xb_Yb,      iemOp_movswd_Xv_Yv,     iemOp_cmpsb_Xb_Yb,      iemOp_cmpswd_Xv_Yv,
    /* 0xa8 */  iemOp_test_AL_Ib,       iemOp_test_eAX_Iz,      iemOp_stosb_Yb_AL,      iemOp_stoswd_Yv_eAX,
    /* 0xac */  iemOp_lodsb_AL_Xb,      iemOp_lodswd_eAX_Xv,    iemOp_scasb_AL_Xb,      iemOp_scaswd_eAX_Xv,
    /* 0xb0 */  iemOp_mov_AL_Ib,        iemOp_CL_Ib,            iemOp_DL_Ib,            iemOp_BL_Ib,
    /* 0xb4 */  iemOp_mov_AH_Ib,        iemOp_CH_Ib,            iemOp_DH_Ib,            iemOp_BH_Ib,
    /* 0xb8 */  iemOp_eAX_Iv,           iemOp_eCX_Iv,           iemOp_eDX_Iv,           iemOp_eBX_Iv,
    /* 0xbc */  iemOp_eSP_Iv,           iemOp_eBP_Iv,           iemOp_eSI_Iv,           iemOp_eDI_Iv,
    /* 0xc0 */  iemOp_Grp2_Eb_Ib,       iemOp_Grp2_Ev_Ib,       iemOp_retn_Iw,          iemOp_retn,
    /* 0xc4 */  iemOp_les_Gv_Mp,        iemOp_lds_Gv_Mp,        iemOp_Grp11_Eb_Ib,      iemOp_Grp11_Ev_Iz,
    /* 0xc8 */  iemOp_enter_Iw_Ib,      iemOp_leave,            iemOp_retf_Iw,          iemOp_retf,
    /* 0xcc */  iemOp_int_3,            iemOp_int_Ib,           iemOp_into,             iemOp_iret,
    /* 0xd0 */  iemOp_Grp2_Eb_1,        iemOp_Grp2_Ev_1,        iemOp_Grp2_Eb_CL,       iemOp_Grp2_Ev_CL,
    /* 0xd4 */  iemOp_aam_Ib,           iemOp_aad_Ib,           iemOp_Invalid,          iemOp_xlat,
    /* 0xd8 */  iemOp_EscF0,            iemOp_EscF1,            iemOp_EscF2,            iemOp_EscF3,
    /* 0xdc */  iemOp_EscF4,            iemOp_EscF5,            iemOp_EscF6,            iemOp_EscF7,
    /* 0xe0 */  iemOp_loopne_Jb,        iemOp_loope_Jb,         iemOp_loop_Jb,          iemOp_jecxz_Jb,
    /* 0xe4 */  iemOp_in_AL_Ib,         iemOp_in_eAX_Ib,        iemOp_out_Ib_AL,        iemOp_out_Ib_eAX,
    /* 0xe8 */  iemOp_call_Jv,          iemOp_jmp_Jv,           iemOp_jmp_Ap,           iemOp_jmp_Jb,
    /* 0xec */  iemOp_in_AL_DX,         iemOp_eAX_DX,           iemOp_out_DX_AL,        iemOp_out_DX_eAX,
    /* 0xf0 */  iemOp_lock,             iemOp_Invalid,          iemOp_repne,            iemOp_repe,
    /* 0xf4 */  iemOp_hlt,              iemOp_cmc,              iemOp_Grp3_Eb,          iemOp_Grp3_Ev,
    /* 0xf8 */  iemOp_clc,              iemOp_stc,              iemOp_cli,              iemOp_sti,
    /* 0xfc */  iemOp_cld,              iemOp_std,              iemOp_Grp4,             iemOp_Grp5,
};


/** @} */
