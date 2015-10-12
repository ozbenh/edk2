//  Implementation of synchronization functions for ARM architecture (AArch64)
//
//  Copyright (c) 2015, IBM Corp. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

.text
.align 4

//GCC_ASM_EXPORT(InternalSyncCompareExchange16)
//GCC_ASM_EXPORT(InternalSyncCompareExchange32)
//GCC_ASM_EXPORT(InternalSyncCompareExchange64)
//GCC_ASM_EXPORT(InternalSyncIncrement)
//GCC_ASM_EXPORT(InternalSyncDecrement)

/**
  Performs an atomic compare exchange operation on a 16-bit unsigned integer.

  Performs an atomic compare exchange operation on the 16-bit unsigned integer
  specified by Value.  If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned.  If Value is not equal to CompareValue,
  then Value is returned.  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param  Value         A pointer to the 16-bit value for the compare exchange
                        operation.
  @param  CompareValue  16-bit value used in compare operation.
  @param  ExchangeValue 16-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
//UINT16
//EFIAPI
//InternalSyncCompareExchange16 (
//  IN      volatile UINT16           *Value,
//  IN      UINT16                    CompareValue,
//  IN      UINT16                    ExchangeValue
//  )
ASM_GLOBAL_EXPORT(InternalSyncCompareExchange16)
	// I don't fully know what primitives this is used for so for safety, I
	// put a full memory barrier both before and after the atomic operation
	// We assume r4 and r5 are already zero extended
	sync
0:	lharx %r0,0,%r3
	cmpw %r0,%r4
	bne 1f
	sthcx. %r5,0,%r3
	bne 0b
	sync
1:	mr %r3,%r0
	blr

/**
  Performs an atomic compare exchange operation on a 32-bit unsigned integer.

  Performs an atomic compare exchange operation on the 32-bit unsigned integer
  specified by Value.  If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned.  If Value is not equal to CompareValue,
  then Value is returned.  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param  Value         A pointer to the 32-bit value for the compare exchange
                        operation.
  @param  CompareValue  32-bit value used in compare operation.
  @param  ExchangeValue 32-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
//UINT32
//EFIAPI
//InternalSyncCompareExchange32 (
//  IN      volatile UINT32           *Value,
//  IN      UINT32                    CompareValue,
//  IN      UINT32                    ExchangeValue
//  )
ASM_GLOBAL_EXPORT(InternalSyncCompareExchange32)
	// I don't fully know what primitives this is used for so for safety, I
	// put a full memory barrier both before and after the atomic operation
	// We assume r4 and r5 are already zero extended
	sync
0:	lwarx %r0,0,%r3
	cmpw %r0,%r4
	bne 1f
	stwcx. %r5,0,%r3
	bne 0b
	sync
1:	mr %r3,%r0
	blr

/**
  Performs an atomic compare exchange operation on a 64-bit unsigned integer.

  Performs an atomic compare exchange operation on the 64-bit unsigned integer specified
  by Value.  If Value is equal to CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to CompareValue, then Value is returned.
  The compare exchange operation must be performed using MP safe mechanisms.

  @param  Value         A pointer to the 64-bit value for the compare exchange
                        operation.
  @param  CompareValue  64-bit value used in compare operation.
  @param  ExchangeValue 64-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
//UINT64
//EFIAPI
//InternalSyncCompareExchange64 (
//  IN      volatile UINT64           *Value,
//  IN      UINT64                    CompareValue,
//  IN      UINT64                    ExchangeValue
//  )
ASM_GLOBAL_EXPORT(InternalSyncCompareExchange64)
	// I don't fully know what primitives this is used for so for safety, I
	// put a full memory barrier both before and after the atomic operation
	sync
0:	ldarx %r0,0,%r3
	cmpw %r0,%r4
	bne 1f
	stdcx. %r5,0,%r3
	bne 0b
	sync
1:	mr %r3,%r0
	blr

/**
  Performs an atomic increment of an 32-bit unsigned integer.

  Performs an atomic increment of the 32-bit unsigned integer specified by
  Value and returns the incremented value. The increment operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to increment.

  @return The incremented value.

**/
//UINT32
//EFIAPI
//InternalSyncIncrement (
//  IN      volatile UINT32           *Value
//  )
ASM_GLOBAL_EXPORT(InternalSyncIncrement)
	// I don't fully know what primitives this is used for so for safety, I
	// put a full memory barrier both before and after the atomic operation
	sync
0:	lwarx %r0,0,%r3
	addi %r0,%r0,1
	stwcx. %r0,0,%r3
	bne 0b
	sync
	mr %r3,%r0
	blr

/**
  Performs an atomic decrement of an 32-bit unsigned integer.

  Performs an atomic decrement of the 32-bit unsigned integer specified by
  Value and returns the decrement value. The decrement operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to decrement.

  @return The decrement value.

**/
//UINT32
//EFIAPI
//InternalSyncDecrement (
//  IN      volatile UINT32           *Value
//  )
ASM_GLOBAL_EXPORT(InternalSyncDecrement)
	// I don't fully know what primitives this is used for so for safety, I
	// put a full memory barrier both before and after the atomic operation
	sync
0:	lwarx %r0,0,%r3
	subi %r0,%r0,1
	stwcx. %r0,0,%r3
	bne 0b
	sync
	mr %r3,%r0
	blr
