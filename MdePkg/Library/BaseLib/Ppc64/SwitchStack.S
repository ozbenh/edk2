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
//
//  This allows the caller to switch the stack and goes to the new entry point
//
// @param      EntryPoint   The pointer to the location to enter
// @param      Context      Parameter to pass in
// @param      Context2     Parameter2 to pass in
// @param      NewStack     New Location of the stack
//
// @return     Nothing. Goes to the Entry Point passing in the new parameters
//
VOID
EFIAPI
InternalSwitchStackAsm (
  SWITCH_STACK_ENTRY_POINT EntryPoint,
  VOID  *Context,
  VOID  *Context2,
  VOID  *NewStack
  );
**/
ASM_GLOBAL_EXPORT(InternalSwitchStack)
    mr    %r12,%r3
    mr	  %r3,%r4 
    mr    %r4,%r5
    subi  %r6,%r6,32	
    std   %r1,0(%r6)
    mr    %r1,%r6	
    mtctr %r12
    bctr

/**
//
//  Requests CPU to pause for a short period of time.
//
//  Requests CPU to pause for a short period of time. Typically used in MP
//  systems to prevent memory starvation while waiting for a spin lock.
//
VOID
EFIAPI
CpuPause (
  VOID
  )
**/
ASM_GLOBAL_EXPORT(CpuPause)
	or 1,1,1
	or 31,31,31
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	or 2,2,2
	blr
