/** @file
*  Core DXE driver for PowerNV machines
*
*  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are
*  licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/HobLib.h>
#include <Library/PnvOpalLib.h>

#include <libfdt.h>

#include <Guid/Fdt.h>
#include <Guid/FdtHob.h>

//
// Initialization
//
EFI_STATUS
EFIAPI
InitializePnvDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID                           *Hob;
  VOID                           *DeviceTreeBase;
  EFI_STATUS                     Status;
  //
  // Recover the DeviceTree HOB and install it in the configuration table
  //
  Hob = GetFirstGuidHob(&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    DEBUG ((EFI_D_ERROR, "%a: No FDT HOB found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (DeviceTreeBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: DTB Invalid @ 0x%p\n", __FUNCTION__, DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DeviceTreeBase);
  ASSERT_EFI_ERROR (Status);
  
  DEBUG ((EFI_D_INFO, "%a: DTB @ 0x%p\n", __FUNCTION__, DeviceTreeBase));

  return EFI_SUCCESS;
}

