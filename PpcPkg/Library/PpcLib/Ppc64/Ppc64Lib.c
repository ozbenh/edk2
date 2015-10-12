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

#include <Base.h>
#include <Library/PpcLib.h>

VOID
EFIAPI
PpcEnableInterrupts (
  VOID
  )
{
  mtmsrd(MSR_EE | MSR_RI, 1);
}

VOID
EFIAPI
PpcDisableInterrupts (
  VOID
  )
{
  mtmsrd(MSR_RI, 1);
}

BOOLEAN
EFIAPI
PpcGetInterruptState (
  VOID
  )
{
  return (mfmsr() & MSR_EE) != 0;
}
