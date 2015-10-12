/** @file
*
*  OPAL Access methods
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

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/PrePiLib.h>
#include <Library/DebugLib.h>
#include <Library/PnvOpalLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>

#include <Guid/FdtHob.h>

struct OPAL_INFO {
	VOID *Base;
	VOID *Entry;	
} gOpal = { NULL , NULL };

STATIC INT32 gOpalNode = -1;

// Manual Library init (for PrePi)
VOID
EFIAPI
PnvOpalLibInit(
  IN VOID *OpalBase,
  IN VOID *OpalEntry
  )
{
  gOpal.Base = OpalBase;
  gOpal.Entry = OpalEntry;
}

// The constructor requires a functional HobLib
RETURN_STATUS
EFIAPI
PnvOpalLibConstructor(
  VOID
  )
{
  VOID                           *Hob;
  VOID                           *DTBase;
  const UINT64                   *PVal;

  DEBUG((DEBUG_INIT, "PnvOpalLibConstructor..\n"));
  Hob = GetFirstGuidHob(&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    DEBUG((DEBUG_INIT, "No Fdt Hob !\n"));
    return EFI_NOT_FOUND;
  }
  DTBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (fdt_check_header (DTBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__, DTBase));
    return EFI_NOT_FOUND;
  }

  gOpalNode = fdt_subnode_offset (DTBase, 0, "ibm,opal");
  if (gOpalNode < 0) {
    DEBUG ((EFI_D_ERROR, "%a: No ibm,opal node in device_tree\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  PVal = fdt_getprop(DTBase, gOpalNode, "opal-entry-address", NULL);
  if (PVal == NULL ) {
    DEBUG ((EFI_D_ERROR, "%a: No opal-entry-address in ibm,opal node\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  gOpal.Entry = (VOID *)fdt64_to_cpu(*PVal);
	
  PVal = fdt_getprop(DTBase, gOpalNode, "opal-base-address", NULL);
  if (PVal == NULL ) {
    DEBUG ((EFI_D_ERROR, "%a: No opal-base-address in ibm,opal node\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  gOpal.Base = (VOID *)fdt64_to_cpu(*PVal);

  return EFI_SUCCESS;
}

EFIAPI
INT32
OpalLibGetOpalNode(
  VOID
  )
{
  return gOpalNode;
}
