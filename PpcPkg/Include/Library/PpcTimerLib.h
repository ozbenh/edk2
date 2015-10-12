/** @file
  PPC implementation of TimerLib.h and more utilities

  Copyright 2015, IBM Corp. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __PPC_TIMER_LIB_H__
#define __PPC_TIMER_LIB_H__

// This should be a Pcd or device-tree ... for now, hard code
#define PPC_TB_FREQ	512000000ul

//
// Conversion To/From uSecs.
//
// Works fine with TB values round multiples of 1Mhz
//
STATIC inline
UINT64
uSecsToTB(
  UINT64 uSecs
  )
{
  return uSecs * (PPC_TB_FREQ / 1000000);
}

STATIC inline
UINT64
TBTouSecs(
  UINT64 Tb
  )
{
  return Tb / (PPC_TB_FREQ / 1000000);
}

//
// Conversion To/From 100nS intervals which are commonly used
// by DXE.
//
STATIC inline
UINT64
nS100ToTB(
  UINT64 nS100
  )
{
  return (nS100 * (PPC_TB_FREQ / 1000000)) / 10;
}

STATIC inline
UINT64
TBTonS100(
  UINT64 Tb
  )
{
  return (Tb * 10) / (PPC_TB_FREQ / 1000000);
}

typedef enum {
  TB_ABEFOREB = -1,
  TB_AEQUALB  = 0,
  TB_AAFTERB  = 1
} TB_CMPVAL;

STATIC inline
TB_CMPVAL
TBCompare(
  UINT64 A,
  UINT64 B
  )
{
  if (A == B) {
    return TB_AEQUALB;
  }
  return ((INT64)(B - A)) > 0 ? TB_ABEFOREB : TB_AAFTERB;
}

#endif /* __PPC_TIMER_LIB_H__ */
