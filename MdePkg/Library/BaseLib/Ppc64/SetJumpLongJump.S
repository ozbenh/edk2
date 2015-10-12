//------------------------------------------------------------------------------
//
// Copyright 2015, IBM Corp. All rights reserved.<BR>
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php.
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

.text
.align 5
	
/**
 * Saves the current CPU context that can be restored with a call to LongJump()
 * and returns 0.
 *
 * The inital call to SetJump() must always return 0. Subsequent calls to LongJump()
 * cause a non-zero value to be returned by SetJump().
 *
 * @param  JumpBuffer    A pointer to CPU context buffer.
 *
**/

ASM_GLOBAL_EXPORT(SetJump)
	mflr	%r0
	std	%r0,0x000(%r3)
	std	%r1,0x008(%r3)
	std	%r2,0x010(%r3)
	mfcr	%r0
	std	%r0,0x018(%r3)
	std	%r13,0x020(%r3)
	std	%r14,0x028(%r3)
	std	%r15,0x030(%r3)
	std	%r16,0x038(%r3)
	std	%r17,0x040(%r3)
	std	%r18,0x048(%r3)
	std	%r19,0x050(%r3)
	std	%r20,0x058(%r3)
	std	%r21,0x060(%r3)
	std	%r22,0x068(%r3)
	std	%r23,0x070(%r3)
	std	%r24,0x078(%r3)
	std	%r25,0x080(%r3)
	std	%r26,0x088(%r3)
	std	%r27,0x090(%r3)
	std	%r28,0x098(%r3)
	std	%r29,0x0a0(%r3)
	std	%r30,0x0a8(%r3)
	std	%r31,0x0b0(%r3)
	stfd	%f18,0x0b8(%r3)
	stfd	%f19,0x0c0(%r3)
	stfd	%f20,0x0c8(%r3)
	stfd	%f21,0x0d0(%r3)
	stfd	%f22,0x0d8(%r3)
	stfd	%f23,0x0e0(%r3)
	stfd	%f24,0x0e8(%r3)
	stfd	%f25,0x0f0(%r3)
	stfd	%f26,0x0f8(%r3)
	stfd	%f27,0x100(%r3)
	stfd	%f28,0x108(%r3)
	stfd	%f29,0x110(%r3)
	stfd	%f30,0x118(%r3)
	stfd	%f31,0x120(%r3)
	// XXX TODO: VRs
	li	%r3,0
	blr

/**
 *  Restores the CPU context that was saved with SetJump().#
 *
 *  Restores the CPU context from the buffer specified by JumpBuffer.
 *  This function never returns to the caller.
 *  Instead is resumes execution based on the state of JumpBuffer.
 *
 *  @param  JumpBuffer    A pointer to CPU context buffer.
 *  @param  Value         The value to return when the SetJump() context is restored.
 *
 **/

ASM_GLOBAL_EXPORT(InternalLongJump)
	ld	%r0,0x000(%r3)
	mtlr	%r0
	ld	%r1,0x008(%r3)
	ld	%r2,0x010(%r3)
	ld	%r0,0x018(%r3)
	mtcr	%r0
	ld	%r13,0x020(%r3)
	ld	%r14,0x028(%r3)
	ld	%r15,0x030(%r3)
	ld	%r16,0x038(%r3)
	ld	%r17,0x040(%r3)
	ld	%r18,0x048(%r3)
	ld	%r19,0x050(%r3)
	ld	%r20,0x058(%r3)
	ld	%r21,0x060(%r3)
	ld	%r22,0x068(%r3)
	ld	%r23,0x070(%r3)
	ld	%r24,0x078(%r3)
	ld	%r25,0x080(%r3)
	ld	%r26,0x088(%r3)
	ld	%r27,0x090(%r3)
	ld	%r28,0x098(%r3)
	ld	%r29,0x0a0(%r3)
	ld	%r30,0x0a8(%r3)
	ld	%r31,0x0b0(%r3)
	lfd	%f18,0x0b8(%r3)
	lfd	%f19,0x0c0(%r3)
	lfd	%f20,0x0c8(%r3)
	lfd	%f21,0x0d0(%r3)
	lfd	%f22,0x0d8(%r3)
	lfd	%f23,0x0e0(%r3)
	lfd	%f24,0x0e8(%r3)
	lfd	%f25,0x0f0(%r3)
	lfd	%f26,0x0f8(%r3)
	lfd	%f27,0x100(%r3)
	lfd	%f28,0x108(%r3)
	lfd	%f29,0x110(%r3)
	lfd	%f30,0x118(%r3)
	lfd	%f31,0x120(%r3)
	mr	%r3,%r4
	blr
