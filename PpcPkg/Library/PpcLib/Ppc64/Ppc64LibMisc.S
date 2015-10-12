//
// Copyright 2015, IBM Corp. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

#define __ASSEMBLY__
#include <Library/PpcLib.h>
#include <Base.h>

ASM_GLOBAL_EXPORT(PpcSetHid0)
  sync
  mtspr	SPR_HID0,%r3
  mfspr	%r3,SPR_HID0
  mfspr	%r3,SPR_HID0
  mfspr	%r3,SPR_HID0
  mfspr	%r3,SPR_HID0
  mfspr	%r3,SPR_HID0
  mfspr	%r3,SPR_HID0
  isync
  blr

ASM_GLOBAL_EXPORT(PpcCpuRelax)
  /* Relax a bit to give sibling threads some breathing space */
  smt_low
  smt_very_low
  nop
  nop
  nop
  nop
  smt_medium
  blr
	
