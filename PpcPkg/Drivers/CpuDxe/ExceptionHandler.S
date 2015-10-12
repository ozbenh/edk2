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

#include "ExceptionHandler.h"

.globl	__ExceptionStubStart
.globl	__ExceptionStubPatchVNum
.globl	__ExceptionStubPatchBranch
.globl	__ExceptionStubEnd
.globl	__ExceptionTrampolineStart
.globl	__ExceptionTrampolineEnd

__ExceptionStubStart:
  mtsprg0    %r3
  mtsprg1    %r4
  mfspr      %r3,SPR_CFAR
__ExceptionStubPatchVNum:
  li         %r4,0
__ExceptionStubPatchBranch:
  b         .
__ExceptionStubEnd:

// XXX TODO: Handle state loss on 0x100 and 0x200
__ExceptionTrampolineStart:
  stdu      %r1,-EXC_SIZE(%r1)
  std       %r3,EXC_CFAR(%r1)
  mfcr      %r3
  stw       %r4,EXC_EXCNUM(%r1)
  mfsprg1   %r4
  stw       %r3,EXC_CR(%r1)
  mfsprg0   %r3
  std       %r0,EXC_Rn(0)(%r1)
  addi      %r0,%r1,EXC_SIZE
  std       %r0,EXC_Rn(1)(%r1)
  std       %r2,EXC_Rn(2)(%r1)
  std       %r3,EXC_Rn(3)(%r1)
  std       %r4,EXC_Rn(4)(%r1)
  std       %r5,EXC_Rn(5)(%r1)
  std       %r6,EXC_Rn(6)(%r1)
  std       %r7,EXC_Rn(7)(%r1)
  std       %r8,EXC_Rn(8)(%r1)
  std       %r9,EXC_Rn(9)(%r1)
  std       %r10,EXC_Rn(10)(%r1)
  std       %r11,EXC_Rn(11)(%r1)
  std       %r12,EXC_Rn(12)(%r1)
  std       %r13,EXC_Rn(13)(%r1)
  std       %r14,EXC_Rn(14)(%r1)
  std       %r15,EXC_Rn(15)(%r1)
  std       %r16,EXC_Rn(16)(%r1)
  std       %r17,EXC_Rn(17)(%r1)
  std       %r18,EXC_Rn(18)(%r1)
  std       %r19,EXC_Rn(19)(%r1)
  std       %r20,EXC_Rn(20)(%r1)
  std       %r21,EXC_Rn(21)(%r1)
  std       %r22,EXC_Rn(22)(%r1)
  std       %r23,EXC_Rn(23)(%r1)
  std       %r24,EXC_Rn(24)(%r1)
  std       %r25,EXC_Rn(25)(%r1)
  std       %r26,EXC_Rn(26)(%r1)
  std       %r27,EXC_Rn(27)(%r1)
  std       %r28,EXC_Rn(28)(%r1)
  std       %r29,EXC_Rn(29)(%r1)
  std       %r30,EXC_Rn(30)(%r1)
  std       %r31,EXC_Rn(31)(%r1)
  mflr      %r3
  mfctr     %r4
  mfxer     %r5
  std       %r3,EXC_LR(%r1)
  std       %r4,EXC_CTR(%r1)
  std       %r5,EXC_XER(%r1)
  mfmsr     %r3
  ori       %r3,%r3,MSR_FP
  mtmsrd    %r3
  isync
  stfd      %f0,EXC_Fn(0)(%r1)
  stfd      %f1,EXC_Fn(1)(%r1)
  stfd      %f2,EXC_Fn(2)(%r1)
  stfd      %f3,EXC_Fn(3)(%r1)
  stfd      %f4,EXC_Fn(4)(%r1)
  stfd      %f5,EXC_Fn(5)(%r1)
  stfd      %f6,EXC_Fn(6)(%r1)
  stfd      %f7,EXC_Fn(7)(%r1)
  stfd      %f8,EXC_Fn(8)(%r1)
  stfd      %f9,EXC_Fn(9)(%r1)
  stfd      %f10,EXC_Fn(10)(%r1)
  stfd      %f11,EXC_Fn(11)(%r1)
  stfd      %f12,EXC_Fn(12)(%r1)
  stfd      %f13,EXC_Fn(13)(%r1)
  stfd      %f14,EXC_Fn(14)(%r1)
  stfd      %f15,EXC_Fn(15)(%r1)
  stfd      %f16,EXC_Fn(16)(%r1)
  stfd      %f17,EXC_Fn(17)(%r1)
  stfd      %f18,EXC_Fn(18)(%r1)
  stfd      %f19,EXC_Fn(19)(%r1)
  stfd      %f20,EXC_Fn(20)(%r1)
  stfd      %f21,EXC_Fn(21)(%r1)
  stfd      %f22,EXC_Fn(22)(%r1)
  stfd      %f23,EXC_Fn(23)(%r1)
  stfd      %f24,EXC_Fn(24)(%r1)
  stfd      %f25,EXC_Fn(25)(%r1)
  stfd      %f26,EXC_Fn(26)(%r1)
  stfd      %f27,EXC_Fn(27)(%r1)
  stfd      %f28,EXC_Fn(28)(%r1)
  stfd      %f29,EXC_Fn(29)(%r1)
  stfd      %f30,EXC_Fn(30)(%r1)
  stfd      %f31,EXC_Fn(31)(%r1)
  mr        %r3,%r1
  mfsprg2   %r12
  mtctr     %r12
  bctrl     // Set LR for nicer backtraces
  b         .    
__ExceptionTrampolineEnd:

RestoreMostRegs:
  lfd       %f0,EXC_Fn(0)(%r1)
  lfd       %f1,EXC_Fn(1)(%r1)
  lfd       %f2,EXC_Fn(2)(%r1)
  lfd       %f3,EXC_Fn(3)(%r1)
  lfd       %f4,EXC_Fn(4)(%r1)
  lfd       %f5,EXC_Fn(5)(%r1)
  lfd       %f6,EXC_Fn(6)(%r1)
  lfd       %f7,EXC_Fn(7)(%r1)
  lfd       %f8,EXC_Fn(8)(%r1)
  lfd       %f9,EXC_Fn(9)(%r1)
  lfd       %f10,EXC_Fn(10)(%r1)
  lfd       %f11,EXC_Fn(11)(%r1)
  lfd       %f12,EXC_Fn(12)(%r1)
  lfd       %f13,EXC_Fn(13)(%r1)
  lfd       %f14,EXC_Fn(14)(%r1)
  lfd       %f15,EXC_Fn(15)(%r1)
  lfd       %f16,EXC_Fn(16)(%r1)
  lfd       %f17,EXC_Fn(17)(%r1)
  lfd       %f18,EXC_Fn(18)(%r1)
  lfd       %f19,EXC_Fn(19)(%r1)
  lfd       %f20,EXC_Fn(20)(%r1)
  lfd       %f21,EXC_Fn(21)(%r1)
  lfd       %f22,EXC_Fn(22)(%r1)
  lfd       %f23,EXC_Fn(23)(%r1)
  lfd       %f24,EXC_Fn(24)(%r1)
  lfd       %f25,EXC_Fn(25)(%r1)
  lfd       %f26,EXC_Fn(26)(%r1)
  lfd       %f27,EXC_Fn(27)(%r1)
  lfd       %f28,EXC_Fn(28)(%r1)
  lfd       %f29,EXC_Fn(29)(%r1)
  lfd       %f30,EXC_Fn(30)(%r1)
  lfd       %f31,EXC_Fn(31)(%r1)
  ld        %r3,EXC_CTR(%r1)
  ld        %r4,EXC_XER(%r1)
  mtctr     %r3
  mtxer     %r4
  ld        %r2,EXC_Rn(2)(%r1)
  ld        %r3,EXC_Rn(3)(%r1)
  ld        %r4,EXC_Rn(4)(%r1)
  ld        %r5,EXC_Rn(5)(%r1)
  ld        %r6,EXC_Rn(6)(%r1)
  ld        %r7,EXC_Rn(7)(%r1)
  ld        %r8,EXC_Rn(8)(%r1)
  ld        %r9,EXC_Rn(9)(%r1)
  ld        %r10,EXC_Rn(10)(%r1)
  ld        %r11,EXC_Rn(11)(%r1)
  ld        %r12,EXC_Rn(12)(%r1)
  ld        %r13,EXC_Rn(13)(%r1)
  ld        %r14,EXC_Rn(14)(%r1)
  ld        %r15,EXC_Rn(15)(%r1)
  ld        %r16,EXC_Rn(16)(%r1)
  ld        %r17,EXC_Rn(17)(%r1)
  ld        %r18,EXC_Rn(18)(%r1)
  ld        %r19,EXC_Rn(19)(%r1)
  ld        %r20,EXC_Rn(20)(%r1)
  ld        %r21,EXC_Rn(21)(%r1)
  ld        %r22,EXC_Rn(22)(%r1)
  ld        %r23,EXC_Rn(23)(%r1)
  ld        %r24,EXC_Rn(24)(%r1)
  ld        %r25,EXC_Rn(25)(%r1)
  ld        %r26,EXC_Rn(26)(%r1)
  ld        %r27,EXC_Rn(27)(%r1)
  ld        %r28,EXC_Rn(28)(%r1)
  ld        %r29,EXC_Rn(29)(%r1)
  ld        %r30,EXC_Rn(30)(%r1)
  ld        %r31,EXC_Rn(31)(%r1)
  lwz       %r0,EXC_CR(%r1)
  mtcr      %r0
  blr

ASM_GLOBAL_EXPORT(ExceptionReturn)
  li        %r0,0
  mtmsrd    %r0,1	
  mr        %r1,%r3
  ld        %r3,EXC_PC(%r1)
  ld        %r4,EXC_MSR(%r1)
  mtspr     SPR_SRR0,%r3
  mtspr     SPR_SRR1,%r4
  bl        RestoreMostRegs
  ld        %r0,EXC_LR(%r1)
  mtlr      %r0
  ld        %r0,EXC_Rn(0)(%r1)
  ld        %r1,EXC_Rn(1)(%r1)
  rfid
