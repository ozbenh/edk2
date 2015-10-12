#------------------------------------------------------------------------------ 
#
# CpuFlushTlb() for PPC64
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
#  Flushes all the Translation Lookaside Buffers(TLB) entries in a CPU.
#
#**/
#VOID
#EFIAPI
#CpuFlushTlb (
#  VOID
#  )#
#
ASM_GLOBAL_EXPORT(CpuFlushTlb)
  # Not Yet Implemented... shouldn't be needed until we have MMU support	
  blr
