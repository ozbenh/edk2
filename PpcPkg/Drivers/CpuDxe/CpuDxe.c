/** @file

  Copyright 2015, IBM Corp. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"
#include "ExceptionHandler.h"

#include <Guid/IdleLoopEvent.h>

STATIC BOOLEAN mInterruptState   = FALSE;
STATIC UINT32 mInterruptNestLevel;
STATIC UINT64 mInterruptCount;

/**
  This function flushes the range of addresses from Start to Start+Length
  from the processor's data cache. If Start is not aligned to a cache line
  boundary, then the bytes before Start to the preceding cache line boundary
  are also flushed. If Start+Length is not aligned to a cache line boundary,
  then the bytes past Start+Length to the end of the next cache line boundary
  are also flushed. The FlushType of EfiCpuFlushTypeWriteBackInvalidate must be
  supported. If the data cache is fully coherent with all DMA operations, then
  this function can just return EFI_SUCCESS. If the processor does not support
  flushing a range of the data cache, then the entire data cache can be flushed.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  Start            The beginning physical address to flush from the processor's data
                           cache.
  @param  Length           The number of bytes to flush from the processor's data cache. This
                           function may flush more bytes than Length specifies depending upon
                           the granularity of the flush operation that the processor supports.
  @param  FlushType        Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS           The address range from Start to Start+Length was flushed from
                                the processor's data cache.
  @retval EFI_UNSUPPORTEDT      The processor does not support the cache flush type specified
                                by FlushType.
  @retval EFI_DEVICE_ERROR      The address range from Start to Start+Length could not be flushed
                                from the processor's data cache.

**/
EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_PHYSICAL_ADDRESS            Start,
  IN UINT64                          Length,
  IN EFI_CPU_FLUSH_TYPE              FlushType
  )
{

    // This should be mostly unnecessary on POWER ...
#if 0
  switch (FlushType) {
    case EfiCpuFlushTypeWriteBack:
      WriteBackDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    case EfiCpuFlushTypeInvalidate:
      InvalidateDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    case EfiCpuFlushTypeWriteBackInvalidate:
      WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }
#endif

  return EFI_SUCCESS;
}


/**
  This function enables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be enabled on the processor.

**/
EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
{
  mInterruptState  = TRUE;

  // We don't allow re-enabling of interrupts while inside a handler, I have
  // experienced cases of massive recursion and stack corruption because of
  // it. The core TPL doesn't know we are in interrupt state so any lock release
  // will pretty much re-enable them.
  //
  // The right fix would be to raise the core current TPL on interrupt entry
  //
  if (mInterruptNestLevel == 0) {
    PpcEnableInterrupts ();
  }

  return EFI_SUCCESS;
}


/**
  This function disables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are disabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be disabled on the processor.

**/
EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
{
  PpcDisableInterrupts ();

  mInterruptState = FALSE;
  return EFI_SUCCESS;
}


/**
  This function retrieves the processor's current interrupt state a returns it in
  State. If interrupts are currently enabled, then TRUE is returned. If interrupts
  are currently disabled, then FALSE is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  State            A pointer to the processor's current interrupt state. Set to TRUE if
                           interrupts are enabled and FALSE if interrupts are disabled.

  @retval EFI_SUCCESS           The processor's current interrupt state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.

**/
EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL         *This,
  OUT BOOLEAN                       *State
  )
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = mInterruptState;
  return EFI_SUCCESS;
}


/**
  This function generates an INIT on the processor. If this function succeeds, then the
  processor will be reset, and control will not be returned to the caller. If InitType is
  not supported by this processor, or the processor cannot programmatically generate an
  INIT without help from external hardware, then EFI_UNSUPPORTED is returned. If an error
  occurs attempting to generate an INIT, then EFI_DEVICE_ERROR is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InitType         The type of processor INIT to perform.

  @retval EFI_SUCCESS           The processor INIT was performed. This return code should never be seen.
  @retval EFI_UNSUPPORTED       The processor INIT operation specified by InitType is not supported
                                by this processor.
  @retval EFI_DEVICE_ERROR      The processor INIT failed.

**/
EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_CPU_INIT_TYPE               InitType
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL          *This,
  IN  UINT32                         TimerIndex,
  OUT UINT64                         *TimerValue,
  OUT UINT64                         *TimerPeriod   OPTIONAL
  )
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = mftb ();

  if (TimerPeriod != NULL) {
    // Should we get this from the device-tree ?
    *TimerPeriod = 512000000;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL    *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    EfiAttributes
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Callback function for idle events.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
IdleLoopEventCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  CpuSleep ();
}

struct {
  UINT32                     Vector;
  UINT32                     Flags;
#define PPC_EXC_FLAG_HV   0x00000001u
  EFI_CPU_INTERRUPT_HANDLER  Handler;
} gPpcExcConfig[EXCEPT_PPC64_COUNT] = {
  [EXCEPT_PPC64_SYS_RESET]         = { 0x100, 0 },
  [EXCEPT_PPC64_MACHINE_CHECK]     = { 0x200, 0 },
  [EXCEPT_PPC64_DATA_STORAGE]      = { 0x300, 0 },
  [EXCEPT_PPC64_DATA_SEGMENT]      = { 0x380, 0 },
  [EXCEPT_PPC64_INST_STORAGE]      = { 0x400, 0 },
  [EXCEPT_PPC64_INST_SEGMENT]      = { 0x480, 0 },
  [EXCEPT_PPC64_EXTERNAL_IRQ]      = { 0x500, 0 },
  [EXCEPT_PPC64_ALIGNMENT]         = { 0x600, 0 },
  [EXCEPT_PPC64_PROGRAM_CHECK]     = { 0x700, 0 },
  [EXCEPT_PPC64_FP_UNAVAILABLE]    = { 0x800, 0 },
  [EXCEPT_PPC64_DECREMENTER]       = { 0x900, 0 },
  [EXCEPT_PPC64_HV_DECREMENTER]    = { 0x980, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_SUP_DBELL]         = { 0xa00, 0 },
  [EXCEPT_PPC64_TRAP_B]            = { 0xb00, 0 },
  [EXCEPT_PPC64_SYSTEM_CALL]       = { 0xc00, 0 },
  [EXCEPT_PPC64_SINGLE_STEP]       = { 0xd00, 0 },
  [EXCEPT_PPC64_H_DATA_STORAGE]    = { 0xe00, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_H_INST_STORAGE]    = { 0xe20, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_H_EMU_ASSIST]      = { 0xe40, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_HW_MAINT]          = { 0xe60, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_HV_DBELL]          = { 0xe80, PPC_EXC_FLAG_HV },
  [EXCEPT_PPC64_PERFMON]           = { 0xf00, 0 },
  [EXCEPT_PPC64_VMX_UNAVAILABLE]   = { 0xf20, 0 },
  [EXCEPT_PPC64_VSX_UNAVAILABLE]   = { 0xf40, 0 },
  [EXCEPT_PPC64_FACIL_UNAVAILABLE] = { 0xf60, 0 },
  [EXCEPT_PPC64_HVFAC_UNAVAILABLE] = { 0xf80, PPC_EXC_FLAG_HV},
  [EXCEPT_PPC64_INST_BKPT]         = { 0x1300, 0 },
  [EXCEPT_PPC64_DENORM_ASSIST]     = { 0x1500, 0 },
  [EXCEPT_PPC64_VMX_ASSIST]        = { 0x1700, 0 },
};

STATIC
VOID
ExceptionStateDump(IN CONST EFI_SYSTEM_CONTEXT SystemContext)
{
  EFI_SYSTEM_CONTEXT_PPC64 *Sc = SystemContext.SystemContextPpc64;

  DEBUG((DEBUG_ERROR, " PC=%016lx MSR=%016lx CFAR=%016lx\n",
	 Sc->PC, Sc->MSR, Sc->CFAR));
  ASSERT(FALSE);
}

STATIC
VOID
DefaultExceptionHandler (
  IN CONST  EFI_EXCEPTION_TYPE  InterruptType,
  IN CONST  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_PPC64 *Sc = SystemContext.SystemContextPpc64;

  DEBUG((DEBUG_ERROR, "Unhandled exception %d (%x) at 0x%lx\n",
         InterruptType, gPpcExcConfig[InterruptType].Vector, Sc->PC));
  ExceptionStateDump(SystemContext);

  ASSERT(FALSE);
}        

STATIC
VOID
DefaultDecrementerHandler (
  IN CONST  EFI_EXCEPTION_TYPE  InterruptType,
  IN CONST  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  // Re-arm to max value
  mtspr(SPR_DEC, 0x7fffffff);
}

STATIC
VOID
UnrecoverableException (
  IN CONST  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_PPC64 *Sc = SystemContext.SystemContextPpc64;

  DEBUG((DEBUG_ERROR, "Unrecoverable exception %d (%x) at 0x%lx\n",
         Sc->ExNum, gPpcExcConfig[Sc->ExNum].Vector, Sc->PC));
  ExceptionStateDump(SystemContext);

  ASSERT(FALSE);
}        

STATIC
VOID __attribute__((noreturn))
CExceptionHandler (
  IN CONST EFI_SYSTEM_CONTEXT SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_PPC64 *Sc = SystemContext.SystemContextPpc64;
  EFI_CPU_INTERRUPT_HANDLER Handler;

  mInterruptNestLevel++;

  mInterruptCount++;

  //
  // WARNING: We must make *no* OPAL call until after we are done reading
  // the various exception related SPRs otherwise they would be clobbered
  //

  // Default handlers if nothing else registers
  if (Sc->ExNum == EXCEPT_PPC64_DECREMENTER) {
    // You can't stop the decrementer so we have a default handler
    // that just re-arms it to its max value
    Handler = DefaultDecrementerHandler;
  } else {
    Handler = DefaultExceptionHandler;
  }

  // Capture additional registers based on exception type. To simplify the
  // code we always capture (H)DAR and (H)DSISR.
  if (Sc->ExNum >= EXCEPT_PPC64_COUNT || Sc->ExNum == EXCEPT_PPC64_UNSUPPORTED) {
    // Unknown exception
    Sc->PC = 0;
    Sc->MSR = 0;
    Sc->DAR = 0;
    Sc->DSISR = 0;
    Sc->ExNum = EXCEPT_PPC64_UNSUPPORTED;
  } else {
    if (gPpcExcConfig[Sc->ExNum].Flags & PPC_EXC_FLAG_HV) {
      // Hypervisor exception
      Sc->PC = mfspr(SPR_HSRR0);
      Sc->MSR = mfspr(SPR_HSRR1);
      Sc->DAR = mfspr(SPR_HDAR);
      Sc->DSISR = mfspr(SPR_HDSISR);
    } else {
      // Standard exception
      Sc->PC = mfspr(SPR_SRR0);
      Sc->MSR = mfspr(SPR_SRR1);
      Sc->DAR = mfspr(SPR_DAR);
      Sc->DSISR = mfspr(SPR_DSISR);
    }
    if (gPpcExcConfig[Sc->ExNum].Handler) {
      Handler = gPpcExcConfig[Sc->ExNum].Handler;
    }
  }
  // We are now recoverable
  mtmsrd(mfmsr() | MSR_RI, 1);

  // Call the handler
  Handler(Sc->ExNum, SystemContext);

  // If it wasn't recoverable, we die
  if (!(Sc->MSR & MSR_RI)) {
    UnrecoverableException(SystemContext);
  }

  //
  // Fixup interrupt enable if necessary
  //
  mInterruptNestLevel--;
  if (mInterruptNestLevel == 0 && mInterruptState) {
    Sc->MSR |= MSR_EE;
  } else {
    DEBUG((DEBUG_ERROR, "WARNING ! Return from interrupt with EE off !\n"));
  }
  
  // Return
  ExceptionReturn(SystemContext);
}

STATIC
VOID
InitializeExceptions (
  VOID
  )
{
  UINT32 Idx;
  UINT64 TrampAddr;
  UINT64 Vector;
  UINT32 VNumPatchOffset;
  UINT32 BranchPatchOffset;
  UINT64 Lpcr;

  ASSERT(sizeof(EFI_SYSTEM_CONTEXT_PPC64) == EXC_SIZE);

  //
  // Configure LPCR to route external irqs to supervisor in LE
  // mode and disable AIL. Wakeup causes are EE and DEC
  // XXX Add MCs, HMIs, ...
  Lpcr = mfspr(SPR_LPCR);
  Lpcr &= ~(SPR_LPCR_AIL | SPR_LPCR_P8_PECE);
  Lpcr |= SPR_LPCR_ILE | SPR_LPCR_P8_PECE2 | SPR_LPCR_P8_PECE3 | SPR_LPCR_LPES;
  mtspr(SPR_LPCR, Lpcr);

  //
  // Ensure we take exceptions in LE mode in HV mode
  //
  PpcSetHid0(mfspr(SPR_HID0) | SPR_HID0_HILE);

  //
  // SPRG2 points to our C handler and will be used by the trampoline
  //
  mtspr(SPR_SPRG2, (UINT64)CExceptionHandler);

  //
  // Install trampoline. XXX For now had-code at 0x2000
  //
  TrampAddr = 0x2000;
  CopyMem((VOID *)TrampAddr, &__ExceptionTrampolineStart,
          &__ExceptionTrampolineEnd - &__ExceptionTrampolineStart);
  
  //
  // Install handler stubs
  //
  VNumPatchOffset = &__ExceptionStubPatchVNum - &__ExceptionStubStart;
  BranchPatchOffset = &__ExceptionStubPatchBranch - &__ExceptionStubStart;
  for (Idx = 0; Idx < EXCEPT_PPC64_UNSUPPORTED; Idx++) {
    //
    // Grab vector offset and copy raw stub over
    //
    Vector = gPpcExcConfig[Idx].Vector;
    CopyMem((VOID *)Vector, &__ExceptionStubStart,
            &__ExceptionStubEnd - &__ExceptionStubStart);
    //
    // Patch in the "li" instruction with the exception number
    //
    *((UINT32 *)(Vector + VNumPatchOffset)) |= Idx;

    //
    // Patch in the relative branch to the trampoline
    //
    *((UINT32 *)(Vector + BranchPatchOffset)) |= TrampAddr - (Vector + BranchPatchOffset);
  }

  //
  // Synchronize cache
  //
  asm volatile("sync; icbi 0,1; isync");

  //
  // Exceptions are now recoverable
  //
  mtmsrd(mfmsr() | MSR_RI, 1);
}

EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL          *This,
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{  
  if (InterruptType < 0 || InterruptType >= EXCEPT_PPC64_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }

  if (InterruptHandler == NULL && gPpcExcConfig[InterruptType].Handler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptHandler != NULL && gPpcExcConfig[InterruptType].Handler != NULL) {
    return EFI_ALREADY_STARTED;
  }

  gPpcExcConfig[InterruptType].Handler = InterruptHandler;

  return EFI_SUCCESS;
}

//
// Globals used to initialize the protocol
//
EFI_HANDLE            mCpuHandle = NULL;
EFI_CPU_ARCH_PROTOCOL mCpu = {
  CpuFlushCpuDataCache,
  CpuEnableInterrupt,
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler,
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  0,          // NumberOfTimers
  7,          // DmaBufferAlignment
};

EFI_STATUS
CpuDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT    IdleLoopEvent;

  InitializeExceptions ();

  Status = gBS->InstallMultipleProtocolInterfaces (
                &mCpuHandle,
                &gEfiCpuArchProtocolGuid,           &mCpu,
                NULL
                );

  //
  // Make sure GCD and MMU settings match. This API calls gDS->SetMemorySpaceAttributes ()
  // and that calls EFI_CPU_ARCH_PROTOCOL.SetMemoryAttributes, so this code needs to go
  // after the protocol is installed
  //
  //  SyncCacheConfig (&mCpu);

  // If the platform is a MPCore system then install the Configuration Table describing the
  // secondary core states
  //  if (ArmIsMpCore()) {
  //    PublishArmProcessorTable();
  //  }

  //
  // Setup a callback for idle events
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IdleLoopEventCallback,
                  NULL,
                  &gIdleLoopEventGuid,
                  &IdleLoopEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
