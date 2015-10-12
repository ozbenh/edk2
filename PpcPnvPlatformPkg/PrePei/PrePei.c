/** @file
*
*  Entry point
*
*  Copyright 2015, IBM Corp. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PnvOpalLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/SerialPortLib.h>

#include <Ppi/GuidedSectionExtraction.h>

#include <Guid/LzmaDecompress.h>
#include <Guid/FdtHob.h>

#include "PrePei.h"
#include "LzmaDecompress.h"

/* Main PrePi */
static
VOID
PrePiMain (
  IN  VOID                      *FDT,
  IN  UINTN                     IMASize
  )
{
  CHAR8                         Buffer[100];
  UINTN                         CharCount;
  EFI_HOB_HANDOFF_INFO_TABLE    *HobList;
  EFI_STATUS                    Status;
  UINTN				StackBase, StackSize, StackEnd;
  UINT64                        *FdtHobData;

  DEBUG((DEBUG_INIT, "PrePi entered, IMASize=0x%lx\n", IMASize));

  // Print a welcome string...
  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"UEFI firmware (version %s built at %a on %a)\n\r",
    (CHAR16*)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__);
  SerialPortWrite ((UINT8 *) Buffer, CharCount);
  
  DEBUG((DEBUG_INIT, "PcdFdBaseAddress = 0x%x\n", PcdGet64(PcdFdBaseAddress)));
  DEBUG((DEBUG_INIT, "PcdFvBaseAddress = 0x%x\n", PcdGet64(PcdFvBaseAddress)));

  // Declare the PI/UEFI memory region. We use the passed in IMA size for now,
  // we should probably be a bit smarter about it when I finally understand the
  // ins and outs of EFI memory management. We also mark as free the region
  // from 0x4000 to our image, so we avoid allocations over our image.
  HobList = HobConstructor (
    (VOID*)0x4000,
    IMASize - 0x4000,
    (VOID*)0x4000,
    (VOID*)PcdGet64(PcdFdBaseAddress)
    );
  PrePeiSetHobList (HobList);

  DEBUG((DEBUG_INIT, "HobList initialized: 0x%p\n", HobList));

  // Initialize Memory HOBs. We need to be careful to only expose HOB's that
  // represent *free* memory with the exception of what's represented by
  // the PHIT hob. To the best of my knowledge (granted quite poor at this
  // point), just creating memory allocation HOBs will not prevent Gcd from
  // picking up the region as free memory for DXE core. But I need to study
  // that code a bit more.
  Status = MemoryPeim (0, IMASize);
  ASSERT_EFI_ERROR (Status);

  DEBUG((DEBUG_INIT, "Memory initialized\n"));

  // Create the Stacks HOB, we need to create it despite the fact that
  // it overlaps our image because otherwise UpdateStackHob() will do
  // nothing and thus the final DXE stack won't be protected.
  //
  // XXX Our stack isn't aligned because currently our PrePei sections
  // aren't either, so we over-reserve
  StackSize = &boot_stack_top - &boot_stack;
  StackBase = (UINTN)&boot_stack;
  StackEnd = StackBase + StackSize;
  StackBase = StackBase & ~(EFI_PAGE_SIZE - 1);
  StackEnd = (StackEnd + (EFI_PAGE_SIZE - 1)) & ~(EFI_PAGE_SIZE - 1);
  BuildStackHob (StackBase, StackEnd - StackBase);

  // Create the FDT Hob, we don't copy the FDT to our own memory as it's
  // layed out inside OPAL memory which should be protected
  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)FDT;

  // Add the FV HOB so we can find our image
  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet64 (PcdFvSize));

  // We need a CPU hob
  BuildCpuHob (PcdGet8 (PcdPrePiCpuMemorySize), PcdGet8 (PcdPrePiCpuIoSize));

  // XXX ... Do more inits & create more HOBs here

  // SEC phase needs to run library constructors by hand.
  ExtractGuidedSectionLibConstructor ();
  LzmaDecompressLibConstructor ();

  // Build HOBs to pass up our version of stuff the DXE Core needs to save space
  BuildPeCoffLoaderHob ();
  BuildExtractSectionHob (
    &gLzmaCustomDecompressGuid,
    LzmaGuidedSectionGetInfo,
    LzmaGuidedSectionExtraction
    );

  // Assume the FV that contains the SEC (our code) also contains a
  // compressed FV.  
  DEBUG((DEBUG_INIT, "Decompressing FV...\n"));
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // Load the DXE Core and transfer control to it
  DEBUG((DEBUG_INIT, "Loading Dxe core...\n"));
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);

  DEBUG((DEBUG_INIT, "Load returned !!!\n"));

  for (;;)
	  opal_poll_events(NULL);
}

VOID
CEntryPoint (
  IN  VOID                      *FDT,
  IN  UINTN                     IMASize,
  IN  VOID                      *OpalBase,
  IN  VOID                      *OpalEntry
  )
{
  PnvOpalLibInit(OpalBase, OpalEntry);

  { // DEBUG
    UINT64 len;
    const CHAR8 EntryMessage[] = "Hello World from CEntryPoint !\n";

    len = SwapBytes64(AsciiStrLen(EntryMessage));
    opal_console_write(0, &len, (UINT8 *)&EntryMessage[0]);
  }

  // Some constructors called by hand, we can't call all of them yet because they
  // rely on memory allocators etc... being initialized
  SerialPortInitialize ();
  BaseDebugLibSerialPortConstructor();
  TimerConstructor();

  PrePiMain(FDT, IMASize);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}
