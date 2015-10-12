/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

#include "LinuxLoader.h"

#define ALIGN(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)    ((void *)(ALIGN ((unsigned long)(p), (a))))
#define GET_CELL(p)     (p += 4, *((const UINT32 *)(p-4)))

typedef struct {
  UINTN   Base;
  UINTN   Size;
} FDT_REGION;

STATIC
BOOLEAN
IsLinuxReservedRegion (
  IN EFI_MEMORY_TYPE MemoryType
  )
{
  switch (MemoryType) {
  case EfiRuntimeServicesCode:
  case EfiRuntimeServicesData:
  case EfiUnusableMemory:
  case EfiACPIReclaimMemory:
  case EfiACPIMemoryNVS:
  case EfiReservedMemoryType:
    return TRUE;
  default:
    return FALSE;
  }
}

/**
** Relocate the FDT blob to a more appropriate location for the Linux kernel.
** This function will allocate memory for the relocated FDT blob.
**
** @retval EFI_SUCCESS on success.
** @retval EFI_OUT_OF_RESOURCES or EFI_INVALID_PARAMETER on failure.
*/
STATIC
EFI_STATUS
RelocateFdt (
  EFI_PHYSICAL_ADDRESS   OriginalFdt,
  UINTN                  OriginalFdtSize,
  EFI_PHYSICAL_ADDRESS   *RelocatedFdt,
  UINTN                  *RelocatedFdtSize,
  EFI_PHYSICAL_ADDRESS   *RelocatedFdtAlloc
  )
{
  EFI_STATUS            Status;
  INTN                  Error;

  *RelocatedFdtSize = OriginalFdtSize + FDT_ADDITIONAL_ENTRIES_SIZE;

  // XXXX This needs fixing ! The FDT could be overwriten by the kernel
  // relocating itself. See comments in LinuxStarter.c
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData,
			       EFI_SIZE_TO_PAGES (*RelocatedFdtSize), RelocatedFdt);
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Failed to allocate pages for FDT\n");
    return Status;
  }

  *RelocatedFdtAlloc = *RelocatedFdt;

  // Load the Original FDT tree into the new region
  Error = fdt_open_into ((VOID*)(UINTN) OriginalFdt,
            (VOID*)(UINTN)(*RelocatedFdt), *RelocatedFdtSize);
  if (Error) {
    DEBUG ((EFI_D_ERROR, "fdt_open_into(): %a\n", fdt_strerror (Error)));
    gBS->FreePages (*RelocatedFdtAlloc, EFI_SIZE_TO_PAGES (*RelocatedFdtSize));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrepareFdt (
  IN     CONST CHAR8*         CommandLineArguments,
  IN     EFI_PHYSICAL_ADDRESS InitrdImage,
  IN     UINTN                InitrdImageSize,
  IN OUT EFI_PHYSICAL_ADDRESS *FdtBlobBase,
  IN OUT UINTN                *FdtBlobSize
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  NewFdtBlobBase;
  EFI_PHYSICAL_ADDRESS  NewFdtBlobAllocation;
  UINTN                 NewFdtBlobSize;
  VOID*                 fdt;
  INTN                  err;
  INTN                  node;
  INT32                 lenp;
  CONST VOID*           BootArg;
  EFI_PHYSICAL_ADDRESS  InitrdImageStart;
  EFI_PHYSICAL_ADDRESS  InitrdImageEnd;
  UINTN                 Index;
  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  EFI_MEMORY_DESCRIPTOR *MemoryMapPtr;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINTN                 Pages;
  UINTN                 OriginalFdtSize;

  NewFdtBlobAllocation = 0;

  //
  // Sanity checks on the original FDT blob.
  //
  err = fdt_check_header ((VOID*)(UINTN)(*FdtBlobBase));
  if (err != 0) {
    Print (L"ERROR: Device Tree header not valid (err:%d)\n", err);
    return EFI_INVALID_PARAMETER;
  }

  // The original FDT blob might have been loaded partially.
  // Check that it is not the case.
  OriginalFdtSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(*FdtBlobBase));
  if (OriginalFdtSize > *FdtBlobSize) {
    Print (L"ERROR: Incomplete FDT. Only %d/%d bytes have been loaded.\n",
           *FdtBlobSize, OriginalFdtSize);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Relocate the FDT to its final location.
  //
  Status = RelocateFdt (*FdtBlobBase, OriginalFdtSize,
			&NewFdtBlobBase, &NewFdtBlobSize, &NewFdtBlobAllocation);
  if (EFI_ERROR (Status)) {
    goto FAIL_RELOCATE_FDT;
  }

  fdt = (VOID*)(UINTN)NewFdtBlobBase;

  node = fdt_subnode_offset (fdt, 0, "chosen");
  if (node < 0) {
    // The 'chosen' node does not exist, create it
    node = fdt_add_subnode (fdt, 0, "chosen");
    if (node < 0) {
      DEBUG ((EFI_D_ERROR, "Error on finding 'chosen' node\n"));
      Status = EFI_INVALID_PARAMETER;
      goto FAIL_COMPLETE_FDT;
    }
  }

  DEBUG_CODE_BEGIN ();
    BootArg = fdt_getprop (fdt, node, "bootargs", &lenp);
    if (BootArg != NULL) {
      DEBUG ((EFI_D_ERROR, "BootArg: %a\n", BootArg));
    }
  DEBUG_CODE_END ();

  //
  // Set Linux CmdLine
  //
  if ((CommandLineArguments != NULL) && (AsciiStrLen (CommandLineArguments) > 0)) {
    err = fdt_setprop (fdt, node, "bootargs", CommandLineArguments,
		       AsciiStrSize (CommandLineArguments));
    if (err) {
      DEBUG ((EFI_D_ERROR, "Fail to set new 'bootarg' (err:%d)\n", err));
    }
  }

  //
  // Set Linux Initrd
  //
  if (InitrdImageSize != 0) {
    InitrdImageStart = cpu_to_fdt64 (InitrdImage);
    err = fdt_setprop (fdt, node, "linux,initrd-start",
		       &InitrdImageStart, sizeof (EFI_PHYSICAL_ADDRESS));
    if (err) {
      DEBUG ((EFI_D_ERROR, "Fail to set new 'linux,initrd-start' (err:%d)\n", err));
    }
    InitrdImageEnd = cpu_to_fdt64 (InitrdImage + InitrdImageSize);
    err = fdt_setprop (fdt, node, "linux,initrd-end",
		       &InitrdImageEnd, sizeof (EFI_PHYSICAL_ADDRESS));
    if (err) {
      DEBUG ((EFI_D_ERROR, "Fail to set new 'linux,initrd-start' (err:%d)\n", err));
    }
  }

  //
  // Add the memory regions reserved by the UEFI Firmware
  //

  // Retrieve the UEFI Memory Map
  MemoryMap = NULL;
  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // The UEFI specification advises to allocate more memory for the MemoryMap buffer between successive
    // calls to GetMemoryMap(), since allocation of the new buffer may potentially increase memory map size.
    Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
    MemoryMap = AllocatePages (Pages);
    if (MemoryMap == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FAIL_COMPLETE_FDT;
    }
    Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  }

  // Go through the list and add the reserved region to the Device Tree
  if (!EFI_ERROR (Status)) {
    MemoryMapPtr = MemoryMap;
    for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
      if (IsLinuxReservedRegion ((EFI_MEMORY_TYPE)MemoryMapPtr->Type)) {
        DEBUG ((DEBUG_VERBOSE, "Reserved region of type %d [0x%lX, 0x%lX]\n",
            MemoryMapPtr->Type,
            (UINTN)MemoryMapPtr->PhysicalStart,
            (UINTN)(MemoryMapPtr->PhysicalStart + MemoryMapPtr->NumberOfPages * EFI_PAGE_SIZE)));
        err = fdt_add_mem_rsv (fdt, MemoryMapPtr->PhysicalStart, MemoryMapPtr->NumberOfPages * EFI_PAGE_SIZE);
        if (err != 0) {
          Print (L"Warning: Fail to add 'memreserve' (err:%d)\n", err);
        }
      }
      MemoryMapPtr = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMapPtr + DescriptorSize);
    }
  }

  // Update the real size of the Device Tree
  fdt_pack ((VOID*)(UINTN)(NewFdtBlobBase));

  *FdtBlobBase = NewFdtBlobBase;
  *FdtBlobSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(NewFdtBlobBase));
  return EFI_SUCCESS;

FAIL_COMPLETE_FDT:
  gBS->FreePages (NewFdtBlobAllocation, EFI_SIZE_TO_PAGES (NewFdtBlobSize));

FAIL_RELOCATE_FDT:
  *FdtBlobSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(*FdtBlobBase));

  // Return success even if we failed to update the FDT blob.
  // The original one is still valid.
  return EFI_SUCCESS;
}
