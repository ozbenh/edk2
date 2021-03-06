/** @file
  Processor or Compiler specific defines and types for Ppc64.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
  Portions copyright (c) 2015, IBM Corp. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PROCESSOR_BIND_H__
#define __PROCESSOR_BIND_H__

///
/// Define the processor type so other code can make processor based choices
///
#define MDE_CPU_PPC64

//
// Make sure we are using the correct packing rules per EFI specification
//
#ifndef __GNUC__
#pragma pack()
#endif

#if _MSC_EXTENSIONS
  //
  // use Microsoft* C complier dependent integer width types 
  //
  typedef unsigned __int64    UINT64;
  typedef __int64             INT64;
  typedef unsigned __int32    UINT32;
  typedef __int32             INT32;
  typedef unsigned short      UINT16;
  typedef unsigned short      CHAR16;
  typedef short               INT16;
  typedef unsigned char       BOOLEAN;
  typedef unsigned char       UINT8;
  typedef char                CHAR8;
  typedef signed char         INT8;
#else
  //
  // Assume standard PPC64 alignment.
  //
  typedef unsigned long long  UINT64;
  typedef long long           INT64;
  typedef unsigned int        UINT32;
  typedef int                 INT32;
  typedef unsigned short      UINT16;
  typedef unsigned short      CHAR16;
  typedef short               INT16;
  typedef unsigned char       BOOLEAN;
  typedef unsigned char       UINT8;
  typedef char                CHAR8;
  typedef signed char         INT8;

  #define UINT8_MAX 0xff
#endif

///
/// Unsigned value of native width.  (4 bytes on supported 32-bit processor instructions,
/// 8 bytes on supported 64-bit processor instructions)
///
typedef UINT64  UINTN;

///
/// Signed value of native width.  (4 bytes on supported 32-bit processor instructions,
/// 8 bytes on supported 64-bit processor instructions)
///
typedef INT64   INTN;

//
// Processor specific defines
//

///
/// A value of native width with the highest bit set.
///
#define MAX_BIT      0x8000000000000000

///
/// A value of native width with the two highest bits set.
///
#define MAX_2_BITS   0xC000000000000000

///
/// Maximum legal PPC64  address (XXX)
///
#define MAX_ADDRESS  0xFFFFFFFFFFFFFFFF

///
/// The stack alignment required for PPC64 (XXX)
///
#define CPU_STACK_ALIGNMENT  16

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify their member functions with EFIAPI.
//
#define EFIAPI

#if defined(__GNUC__)
    // Global exported functions need an external entry with ELF v2
    #define ASM_GLOBAL(func__)                 \
             .align 2;                         \
             .type ASM_PFX(func__), @function; \
             .global ASM_PFX(func__);          \
         ASM_PFX(func__):

    #define ASM_GLOBAL_EXPORT(func__)	       \
             ASM_GLOBAL(func__)                \
0:	     addis %r2,%r12,(.TOC.-0b)@ha;       \
	     addi %r2,%r2,(.TOC.-0b)@l;          \
            .localentry ASM_PFX(func__),.-ASM_PFX(func__)
#endif

#endif

