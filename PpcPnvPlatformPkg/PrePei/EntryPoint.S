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

#define __ASSEMBLY__ // Make the build tools do this !

#include <Base.h>
#include <Library/PpcLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

	.text
	.align 5

#define LOAD_REG32(r,v)	\
	lis	r,(v)@ha	;\
	addi	r,r,(v)@l

#define R_PPC64_RELATIVE  22

/* Main entry point
 *
 * r3  : FDT base
 * r6  : EPAPR magic
 * r7  : IMA size
 * r8  : OPAL base
 * r9  : OPAL entry
 * lr  : Image start address + 4
 */
ASM_GLOBAL(_ModuleEntryPoint)
	/* Grab image starting address. We know that LR will point to
	 * this + 4 and stash it in r30 for later
	 */
	mflr	%r10
	subi	%r30,%r10,4

	/* Fixup TOC */
	bcl     20,31,+4
0:	mflr    %r12
	addis   %r2,%r12,(.TOC.-0b)@ha;
	addi    %r2,%r2,(.TOC.-0b)@l;

	/* Get relocs address */
	LOAD_REG32(%r15, __reloc_start - 0b)
	LOAD_REG32(%r16, __reloc_end - 0b)
	add %r15,%r15,%r12
	add %r16,%r16,%r12
	/* Runtime address of __reloc_base in r14 */
	LOAD_REG32(%r14, __reloc_base - 0b)
	add %r14,%r14,%r12

	/* Perform relocations */
1:	cmpd	%cr0,%r15,%r16
	bge	2f
	ld	%r18,0(%r15)	/* r_offset */
	ld	%r19,8(%r15)	/* r_info */
	ld	%r20,16(%r15)	/* r_addend */
	addi	%r15,%r15,24
	cmpdi	%r19,R_PPC64_RELATIVE
	bne	bad_reloc
	add	%r0,%r20,%r14
	stdx	%r0,%r18,%r14
	b	1b
2:
	/* Clear BSS */
	LOAD_REG32(%r15, __bss_start - 0b)
	LOAD_REG32(%r16, __bss_end - 0b)
	subf	%r16,%r15,%r16
	srdi	%r16,%r16,3
	mtctr	%r16
	li	%r0,0
1:	std	%r0,0(%r15)
	bdnz	1b

	/* Patch Fd/FvBaseAddress PCDs */
	ld	%r10,PcdGet64 (PcdFvBaseAddress)@got(%r2)
	ld	%r11,0(%r10)
	subf	%r11,%r11,%r30
	std	%r30,0(%r10)
	ld	%r10,PcdGet64 (PcdFdBaseAddress)@got(%r2)
	ld	%r29,0(%r10)
	add	%r29,%r11,%r29
	std	%r29,0(%r10)

	/* Enable MSR:FP, might be used by generated code */
	mfmsr	%r10
	ori	%r10,%r10,MSR_FP
	mtmsr	%r10
	isync

	/* Setup a local stack for now */
 	LOAD_REG32(%r1, boot_stack_top - 0b)
	add	%r1,%r1,%r12
	subi	%r1,%r1,32
	li	%r0,0
	std	%r0,0(%r1)

	/* Move arguments */
	mr	%r4,%r7
	mr	%r5,%r8
	mr	%r6,%r9

	/* Call C */
	b	CEntryPoint

bad_reloc:
	/* Not much we can do here */
	attn
	b .

	.section ".bss"
	.balign 16
.globl boot_stack
boot_stack:
	.space	0x4000
.globl boot_stack_top
boot_stack_top:
