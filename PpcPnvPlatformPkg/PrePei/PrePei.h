/** @file
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

#ifndef _PREPI_H_
#define _PREPI_H_

/* Debug helper */
#define SerialPrint(txt)  SerialPortWrite (txt, AsciiStrLen(txt)+1);

/*
 * Constructor prototypes
 */

RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  );

RETURN_STATUS
EFIAPI
BaseDebugLibSerialPortConstructor (
  VOID
  );

RETURN_STATUS
EFIAPI
ExtractGuidedSectionLibConstructor (
  VOID
  );

RETURN_STATUS
EFIAPI
LzmaDecompressLibConstructor (
  VOID
  );

/*
 * Internal prototypes
 */

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS               MemoryBase,
  IN UINT64                             MemorySize
  );

// Either implemented by PrePiLib or by MemoryInitPei
VOID
BuildMemoryTypeInformationHob (
  VOID
  );

/*
 * Exported from EntryPoint.S
 */
char boot_stack;
char boot_stack_top;

#endif /* _PREPI_H_ */
