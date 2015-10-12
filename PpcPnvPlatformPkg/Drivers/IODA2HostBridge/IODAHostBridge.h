/** @file

  Top level PowerPC IODA Host Bridge support

  Copyright 2015, IBM Corp. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IODA_HOST_BRIDGE_H_
#define _IODA_HOST_BRIDGE_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/Metronome.h>
#include <Protocol/DevicePath.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/PnvOpalLib.h>

#include <libfdt.h>

//
// PCI Root Bridge Device Path (ACPI Device Node + End Node)
//
// XXX Change this... ?
//
typedef struct {
  ACPI_HID_DEVICE_PATH          Acpi;
  EFI_DEVICE_PATH_PROTOCOL      End;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

//
// Resource configuration array
//
typedef enum {
  TypeMem32,
  TypeMem64,
  TypeBus,
  TypeMax
} PCI_RESOURCE_TYPE;

#pragma pack(1)
typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR SpaceDesc[TypeMax];
  EFI_ACPI_END_TAG_DESCRIPTOR       EndDesc;
} RESOURCE_CONFIGURATION;
#pragma pack()
//
// IODA has a fairly fixed 1:1 relationship between a Host Bridge and
// a Root Bridge. Too keep things simpler, we thus create a single
// instance structure to cover both
//

#define IODA_HOST_BRIDGE_SIGNATURE  SIGNATURE_32('I', 'O', 'D', 'A')
typedef struct {
  UINTN                                             Signature;  
  EFI_HANDLE                                        HostBridgeHandle;
  BOOLEAN                                           ResourceSubmited;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  ResAlloc;
  EFI_HANDLE                                        RootBridgeHandle;
  UINT64                                            Attributes;
  UINT64                                            RbAttributesSupports;
  UINT64                                            RbAttributes;
  UINT32                                            OpalId;
  INT32                                             FdtNode;	

  UINT64                                            Mem32CpuBase;
  UINT32                                            Mem32PciBase;
  UINT32                                            Mem32Size;
  UINT64                                            Mem64CpuBase;
  UINT64                                            Mem64PciBase;
  UINT64                                            Mem64Size;

  UINT64                                            *TceTable32;
  UINT32                                            Dma32Base;
  UINT32                                            Dma32Size;
  
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH                   DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                   Io;

  RESOURCE_CONFIGURATION                            ConfigBuffer;
} IODA_HOST_BRIDGE_INSTANCE;

#define INSTANCE_FROM_RESOURCE_ALLOCATION_THIS(a) \
  CR(a, IODA_HOST_BRIDGE_INSTANCE, ResAlloc, IODA_HOST_BRIDGE_SIGNATURE)

#define INSTANCE_FROM_ROOT_BRIDGE_IO_THIS(a) \
  CR(a, IODA_HOST_BRIDGE_INSTANCE, Io, IODA_HOST_BRIDGE_SIGNATURE)

//
// Used internally for DMA bounce buffering
//
typedef struct {
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation;
  UINTN                                      NumberOfBytes;
  UINTN                                      NumberOfPages;
  EFI_PHYSICAL_ADDRESS                       HostAddress;
  EFI_PHYSICAL_ADDRESS                       MappedHostAddress;
} MAP_INFO;


//
// Root bridge IO protocol exports
//

EFI_STATUS
EFIAPI
RootBridgeIoPollMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );


EFI_STATUS
EFIAPI
RootBridgeIoPollIo (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );

EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN     VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  OUT    VOID                                   *UserBuffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN     VOID                                   *UserBuffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoCopyMem (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 DestAddress,
  IN     UINT64                                 SrcAddress,
  IN     UINTN                                  Count
  );

EFI_STATUS
EFIAPI
RootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN     VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
RootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

EFI_STATUS
EFIAPI
RootBridgeIoUnmap (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  VOID                             *Mapping
  );

EFI_STATUS
EFIAPI
RootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  );

EFI_STATUS
EFIAPI
RootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  );

EFI_STATUS
EFIAPI
RootBridgeIoFlush (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
RootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  );

EFI_STATUS
EFIAPI
RootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength
  );

EFI_STATUS
EFIAPI
RootBridgeIoConfiguration (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT    VOID                             **Resources
  );


//
// Others
//
EFI_STATUS
RootBridgeIoInit (
  IN   IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  );

#endif /* _IODA_HOST_BRIDGE_H_ */

