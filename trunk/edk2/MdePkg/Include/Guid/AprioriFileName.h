/** @file

  The GUID PEI_APRIORI_FILE_NAME_GUID definition is the file
  name of the PEI a priori file that is stored in a firmware
  volume.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference: PI
  Version 1.00.

**/

#ifndef __PEI_APRIORI_FILE_NAME_H__
#define __PEI_APRIORI_FILE_NAME_H__

#define PEI_APRIORI_FILE_NAME_GUID \
  { 0x1b45cc0a, 0x156a, 0x428a, { 0x62, 0XAF, 0x49, 0x86, 0x4d, 0xa0, 0xe6, 0xe6 } }


/**
   
  This file must be of type EFI_FV_FILETYPE_FREEFORM and must
  contain a single section of type EFI_SECTION_RAW. For details on
  firmware volumes, firmware file types, and firmware file section
  types.

  @param FileNamesWithinVolume[]  An array of zero or more
                                  EFI_GUID type entries that
                                  match the file names of PEIM
                                  modules in the same Firmware
                                  Volume. The maximum number of
                                  entries.

  @param NumberOfModulesInVolume  Determined by the number of
                                  modules in the FV.
  
**/
typedef struct {
  EFI_GUID  FileNamesWithinVolume[1];
// Optional list of file-names
} PEI_APRIORI_FILE_CONTENTS;

extern EFI_GUID gPeiAprioriFileNameGuid;

#endif

