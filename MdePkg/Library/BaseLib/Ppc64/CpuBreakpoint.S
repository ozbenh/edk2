#------------------------------------------------------------------------------
#
# CpuBreakpoint() for PPC64
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
#  Generates a breakpoint on the CPU.
#
#  Generates a breakpoint on the CPU. The breakpoint must be implemented such
#  that code can resume normal execution after the breakpoint.
#
#**/
#VOID
#EFIAPI
#CpuBreakpoint (
#  VOID
#  );
#
ASM_GLOBAL_EXPORT(CpuBreakpoint)
  // Use ATTN ?
  trap
  blr
