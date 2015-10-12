/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
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

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>

#include "PrePei.h"

static
VOID
InitMmu (
  VOID
  )
{
  // TODO, for now stick to real mode
}

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS               MemoryBase,
  IN UINT64                             MemorySize
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttributes;
  UINT64 Base,Size;

  //
  // For now we simply declare the IMA memory given to us, we will
  // do things a bit more smartly when I understand UEFI memory
  // management a bit better.
  //
  // Now, the permanent memory has been installed, we can call AllocatePages()
  //
  ResourceAttributes = (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
  );

  // TODO, use device-tree
  DEBUG((DEBUG_INIT, "System Memory Hob: %lx, %lx\n", MemoryBase, MemorySize));
  BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      ResourceAttributes,
      MemoryBase, MemorySize);

  // Reserve ourselves (TODO: reserve map in DT ?)
  Base = PcdGet64(PcdFdBaseAddress) & ~(EFI_PAGE_SIZE - 1);
  Size = ((PcdGet64(PcdFdBaseAddress) + PcdGet64(PcdFdSize)) + (EFI_PAGE_SIZE - 1)) & ~(EFI_PAGE_SIZE - 1);
  DEBUG((DEBUG_INIT, "Reserve Hob: %lx, %lx\n", Base, Size));
  BuildMemoryAllocationHob (Base, Size, EfiBootServicesData);

  // Initialize MMU
  InitMmu ();

  if (FeaturePcdGet (PcdPrePiProduceMemoryTypeInformationHob)) {
    // Optional feature that helps prevent EFI memory map fragmentation.
    BuildMemoryTypeInformationHob ();
  }

  return EFI_SUCCESS;
}
