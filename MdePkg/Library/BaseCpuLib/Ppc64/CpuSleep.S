#------------------------------------------------------------------------------ 
#
# CpuSleep() for PPC64
#
# Copyright 2015, IBM Corp. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.align 5

#/**
#  Places the CPU in a sleep state until an interrupt is received.
#
#  Places the CPU in a sleep state until an interrupt is received. If interrupts
#  are disabled prior to calling this function, then the CPU will be placed in a
#  sleep state indefinitely.
#
#  Note: Current implementation just uses low priority nop loop, we will add
#  nap support later
#
#**/
#VOID
#EFIAPI
#CpuSleep (
#  VOID
#  );
#
ASM_GLOBAL_EXPORT(CpuSleep)
  b CpuPause
