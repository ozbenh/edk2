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

#include "IODAHostBridge.h"

STATIC VOID *mFdtBase;
STATIC EFI_HANDLE mDriverImageHandle;

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mPciRootDevPathTemplate = {
  // XX FIXME !!!
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A03),
    0
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

//
// Internal helpers
//

STATIC
EFI_STATUS
GetFdt( VOID )
{
  VOID                           *Hob;
  
  //
  // Find Device Tree
  //
  Hob = GetFirstGuidHob(&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    DEBUG((EFI_D_ERROR, "No Fdt Hob !\n"));
    return EFI_NOT_FOUND;
  }
  mFdtBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (fdt_check_header (mFdtBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__, mFdtBase));
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

//
// Reset Host Bridge configuration
//
STATIC
EFI_STATUS
ResetHostBridgeConfig(
  IN IODA_HOST_BRIDGE_INSTANCE    *HbInstance
  )
{
  UINT64                                BusStart;
  UINT64                                BusEnd;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     *Desc;

  DEBUG ((EFI_D_INFO, "PHB%ld: ResetHostBridgeConfig\n", HbInstance->OpalId));
  
  //
  // Setup MMIO 32-bit aperture.
  //
  Desc = &HbInstance->ConfigBuffer.SpaceDesc[TypeMem32];
  Desc->Desc = 0x8A;
  Desc->Len  = 0x2B;
  Desc->ResType = 0;
  Desc->GenFlag = 0;
  Desc->SpecificFlag = 0;
  Desc->AddrSpaceGranularity = 32;
  Desc->AddrRangeMin = HbInstance->Mem32PciBase;
  Desc->AddrRangeMax = 0;
  Desc->AddrLen = HbInstance->Mem32Size;
  Desc->AddrTranslationOffset = HbInstance->Mem32CpuBase - HbInstance->Mem32PciBase;

  //
  // Setup MMIO 64-bit aperture.
  //
  Desc = &HbInstance->ConfigBuffer.SpaceDesc[TypeMem64];
  Desc->Desc = 0x8A;
  Desc->Len  = 0x2B;
  Desc->ResType = 0;
  Desc->GenFlag = 0;
  Desc->SpecificFlag = 0;
  Desc->AddrSpaceGranularity = 64;
  Desc->AddrRangeMin = HbInstance->Mem64PciBase;
  Desc->AddrRangeMax = 0;
  Desc->AddrLen = HbInstance->Mem64Size;
  Desc->AddrTranslationOffset = HbInstance->Mem64CpuBase - HbInstance->Mem64PciBase;

  //
  // Setup bus numbers.
  //
  // XXX Use device-tree "bus-range" property
  //
  BusStart = 0x00;
  BusEnd   = 0xff;
  Desc = &HbInstance->ConfigBuffer.SpaceDesc[TypeBus];
  Desc->Desc = 0x8A;
  Desc->Len  = 0x2B;
  Desc->ResType = 2;
  Desc->GenFlag = 0;
  Desc->SpecificFlag = 0;
  Desc->AddrSpaceGranularity = 0;
  Desc->AddrRangeMin = BusStart;
  Desc->AddrRangeMax = 0;
  Desc->AddrTranslationOffset = 0;
  Desc->AddrLen = BusEnd - BusStart + 1;

  //
  // Setup end tag.
  //
  HbInstance->ConfigBuffer.EndDesc.Desc = 0x79;
  HbInstance->ConfigBuffer.EndDesc.Checksum = 0x0;

  return EFI_SUCCESS;
}  

STATIC
EFI_STATUS
ResetHostBridgeHardware(
  IN IODA_HOST_BRIDGE_INSTANCE    *HbInstance
  )
{
  INT64                                 OpalStatus;
  INT32                                 Index;
  CONST UINT32                          *NumPePtr;
  UINT32                                NumPe;
  UINT64                                TblSize;

  DEBUG ((EFI_D_INFO, "PHB%ld: ResetHostBridgeHardware\n", HbInstance->OpalId));

  //
  // Reset all the IODA tables in the bridge
  //
  OpalStatus = opal_pci_reset(HbInstance->OpalId,
                              OPAL_RESET_PCI_IODA_TABLE, OPAL_ASSERT_RESET);
  if (OpalStatus != OPAL_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_reset()\n",
            __FUNCTION__, OpalStatus));
  }

  //
  // Grab the number of PEs (which gives us the number of M32 segments)
  //
  NumPePtr = fdt_getprop(mFdtBase, HbInstance->FdtNode, "ibm,opal-num-pes", NULL);
  if (NumPePtr == NULL ) {
    DEBUG ((EFI_D_ERROR, "%a: No ibm,opal-num-es in device-tree\n", __FUNCTION__));
    NumPe = 1;
  } else {
    NumPe = fdt32_to_cpu(*NumPePtr);
  }

  //
  // Map all M32 segments as PE#1
  //
  for (Index = 0; Index < NumPe; Index++) {
    OpalStatus = opal_pci_map_pe_mmio_window(HbInstance->OpalId,
                                             1, OPAL_M32_WINDOW_TYPE,
                                             0, Index);
    if (OpalStatus != OPAL_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_map_pe_mmio_window()\n",
              __FUNCTION__, OpalStatus));
    }
  }

  //
  // Create a single-PE M64 covering all the of the 64-bit space
  //
  if (HbInstance->Mem64Size) {
    OpalStatus = opal_pci_set_phb_mem_window(HbInstance->OpalId,
                                             OPAL_M64_WINDOW_TYPE,
                                             0, /* M64 idx */
                                             HbInstance->Mem64CpuBase,
                                             0, /* unused */
                                             HbInstance->Mem64Size);
    if (OpalStatus != OPAL_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_set_phb_mem_window() for M64\n",
              __FUNCTION__, OpalStatus));
      // XXX do something
    }
    OpalStatus = opal_pci_map_pe_mmio_window(HbInstance->OpalId,
					     1, /* PE# */
                                             OPAL_M64_WINDOW_TYPE,
                                             0, /* Window# */
                                             0  /* Segment# */);
    if (OpalStatus != OPAL_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_map_pe_mmio_window() for M64\n",
              __FUNCTION__, OpalStatus));
      // XXX do something
    }
    OpalStatus = opal_pci_phb_mmio_enable(HbInstance->OpalId,
                                          OPAL_M64_WINDOW_TYPE,
                                          0, /* Window# */
                                          OPAL_ENABLE_M64_NON_SPLIT);
    if (OpalStatus != OPAL_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_phb_mmio_enable() for M64\n",
              __FUNCTION__, OpalStatus));
      // XXX do something
    }
  }

  //
  // Create a TVT for PE#1 mapping 0...2G to system RAM 1:1
  //
  TblSize = (HbInstance->Dma32Size >> 12) << 3;
  
  DEBUG ((EFI_D_INFO, "PHB%ld Mapping 32-bit DMA:\n", HbInstance->OpalId));
  DEBUG ((EFI_D_INFO, "  TceTable: %08lx\n", (UINT64)HbInstance->TceTable32));
  DEBUG ((EFI_D_INFO, "  TblSize : %08lx\n", TblSize));
     
  OpalStatus = opal_pci_map_pe_dma_window(HbInstance->OpalId,
                                          1, // PE number
                                          2, // TVE number (2 & PE number for TVE0)
                                          1, // Levels
                                          (UINT64)HbInstance->TceTable32, // Table pointer
                                          TblSize, // Table size in bytes,
                                          0x1000); // TCE page size
  if (OpalStatus != OPAL_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_map_pe_dma_window()\n",
            __FUNCTION__, OpalStatus));
  }
  
  return EFI_SUCCESS;
}

//
// Allocate the resources in the system map
//
STATIC
EFI_STATUS
AllocateHostBridgeResources(
  IN IODA_HOST_BRIDGE_INSTANCE    *HbInstance
  )
{
  DEBUG ((EFI_D_INFO, "PHB%ld: Allocate Host Bridge Resouces\n", HbInstance->OpalId));

  return EFI_SUCCESS;
}

//
// Free the resources in the system map
//
STATIC
EFI_STATUS
FreeHostBridgeResources(
  IN IODA_HOST_BRIDGE_INSTANCE    *HbInstance
  )
{
  DEBUG ((EFI_D_INFO, "PHB%ld: Free Host Bridge Resouces\n", HbInstance->OpalId));

  return EFI_SUCCESS;
}

//
// Host Bridge Protocol Callbacks
//

/**
  These are the notifications from the PCI bus driver that it is about to enter
  a certain phase of the PCI enumeration process.

  This member function can be used to notify the host bridge driver to perform
  specific actions, including any chipset-specific initialization, so that the
  chipset is ready to enter the next phase. Eight notification points are
  defined at this time. See belows:

  EfiPciHostBridgeBeginEnumeration       Resets the host bridge PCI apertures
                                         and internal data structures. The PCI
                                         enumerator should issue this
                                         notification before starting a fresh
                                         enumeration process. Enumeration
                                         cannot be restarted after sending any
                                         other notification such as
                                         EfiPciHostBridgeBeginBusAllocation.

  EfiPciHostBridgeBeginBusAllocation     The bus allocation phase is about to
                                         begin. No specific action is required
                                         here. This notification can be used to
                                         perform any chipset-specific
                                         programming.

  EfiPciHostBridgeEndBusAllocation       The bus allocation and bus programming
                                         phase is complete. No specific action
                                         is required here. This notification
                                         can be used to perform any
                                         chipset-specific programming.

  EfiPciHostBridgeBeginResourceAllocation
                                         The resource allocation phase is about
                                         to begin. No specific action is
                                         required here. This notification can
                                         be used to perform any
                                         chipset-specific programming.

  EfiPciHostBridgeAllocateResources      Allocates resources per previously
                                         submitted requests for all the PCI
                                         root bridges. These resource settings
                                         are returned on the next call to
                                         GetProposedResources(). Before calling
                                         NotifyPhase() with a Phase of
                                         EfiPciHostBridgeAllocateResource, the
                                         PCI bus enumerator is responsible for
                                         gathering I/O and memory requests for
                                         all the PCI root bridges and
                                         submitting these requests using
                                         SubmitResources(). This function pads
                                         the resource amount to suit the root
                                         bridge hardware, takes care of
                                         dependencies between the PCI root
                                         bridges, and calls the Global
                                         Coherency Domain (GCD) with the
                                         allocation request. In the case of
                                         padding, the allocated range could be
                                         bigger than what was requested.

  EfiPciHostBridgeSetResources           Programs the host bridge hardware to
                                         decode previously allocated resources
                                         (proposed resources) for all the PCI
                                         root bridges. After the hardware is
                                         programmed, reassigning resources will
                                         not be supported. The bus settings are
                                         not affected.

  EfiPciHostBridgeFreeResources          Deallocates resources that were
                                         previously allocated for all the PCI
                                         root bridges and resets the I/O and
                                         memory apertures to their initial
                                         state. The bus settings are not
                                         affected. If the request to allocate
                                         resources fails, the PCI enumerator
                                         can use this notification to
                                         deallocate previous resources, adjust
                                         the requests, and retry allocation.

  EfiPciHostBridgeEndResourceAllocation  The resource allocation phase is
                                         completed. No specific action is
                                         required here. This notification can
                                         be used to perform any chipsetspecific
                                         programming.

  @param[in] This                The instance pointer of
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL

  @param[in] Phase               The phase during enumeration

  @retval EFI_NOT_READY          This phase cannot be entered at this time. For
                                 example, this error is valid for a Phase of
                                 EfiPciHostBridgeAllocateResources if
                                 SubmitResources() has not been called for one
                                 or more PCI root bridges before this call

  @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.
                                 This error is valid for a Phase of
                                 EfiPciHostBridgeSetResources.

  @retval EFI_INVALID_PARAMETER  Invalid phase parameter

  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources. This error is valid for a
                                 Phase of EfiPciHostBridgeAllocateResources if
                                 the previously submitted resource requests
                                 cannot be fulfilled or were only partially
                                 fulfilled.

  @retval EFI_SUCCESS            The notification was accepted without any
                                 errors.
**/
EFI_STATUS
EFIAPI
NotifyPhase(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  //  EFI_STATUS                            Status;

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  DEBUG ((EFI_D_INFO, "PHB%ld NotifyPhase(%d)\n", HbInstance->OpalId, Phase));

  switch (Phase) {

  case EfiPciHostBridgeBeginEnumeration:
    ResetHostBridgeConfig(HbInstance);
    ResetHostBridgeHardware(HbInstance);
    break;

  case EfiPciHostBridgeEndEnumeration:
    break;

  case EfiPciHostBridgeBeginBusAllocation:
    break;

  case EfiPciHostBridgeEndBusAllocation:
    //
    // No specific action is required here, can perform any chipset specific
    // programing
    //
    break;

  case EfiPciHostBridgeBeginResourceAllocation:
    //
    // No specific action is required here, can perform any chipset specific
    // programing
    //
    break;

  case EfiPciHostBridgeAllocateResources:
    return AllocateHostBridgeResources(HbInstance);

  case EfiPciHostBridgeSetResources:
    break;

  case EfiPciHostBridgeFreeResources:
    return FreeHostBridgeResources(HbInstance);
 
  case EfiPciHostBridgeEndResourceAllocation:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Return the device handle of the next PCI root bridge that is associated with
  this Host Bridge.

  This function is called multiple times to retrieve the device handles of all
  the PCI root bridges that are associated with this PCI host bridge. Each PCI
  host bridge is associated with one or more PCI root bridges. On each call,
  the handle that was returned by the previous call is passed into the
  interface, and on output the interface returns the device handle of the next
  PCI root bridge. The caller can use the handle to obtain the instance of the
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL for that root bridge. When there are no more
  PCI root bridges to report, the interface returns EFI_NOT_FOUND. A PCI
  enumerator must enumerate the PCI root bridges in the order that they are
  returned by this function.

  For D945 implementation, there is only one root bridge in PCI host bridge.

  @param[in]       This              The instance pointer of
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL

  @param[in, out]  RootBridgeHandle  Returns the device handle of the next PCI
                                     root bridge.

  @retval EFI_SUCCESS            If parameter RootBridgeHandle = NULL, then
                                 return the first Rootbridge handle of the
                                 specific Host bridge and return EFI_SUCCESS.

  @retval EFI_NOT_FOUND          Can not find the any more root bridge in
                                 specific host bridge.

  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not an EFI_HANDLE that was
                                 returned on a previous call to
                                 GetNextRootBridge().
**/
EFI_STATUS
EFIAPI
GetNextRootBridge(
  IN       EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT   EFI_HANDLE                                       *RootBridgeHandle
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  if (*RootBridgeHandle == NULL) {
    *RootBridgeHandle = HbInstance->RootBridgeHandle;
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Returns the allocation attributes of a PCI root bridge.

  The function returns the allocation attributes of a specific PCI root bridge.
  The attributes can vary from one PCI root bridge to another. These attributes
  are different from the decode-related attributes that are returned by the
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.GetAttributes() member function. The
  RootBridgeHandle parameter is used to specify the instance of the PCI root
  bridge. The device handles of all the root bridges that are associated with
  this host bridge must be obtained by calling GetNextRootBridge(). The
  attributes are static in the sense that they do not change during or after
  the enumeration process. The hardware may provide mechanisms to change the
  attributes on the fly, but such changes must be completed before
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL is installed. The permitted
  values of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ATTRIBUTES are defined in
  "Related Definitions" below. The caller uses these attributes to combine
  multiple resource requests.

  For example, if the flag EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM is set, the PCI
  bus enumerator needs to include requests for the prefetchable memory in the
  nonprefetchable memory pool and not request any prefetchable memory.

  Attribute                             Description
  ------------------------------------  ---------------------------------------
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM  If this bit is set, then the PCI root
                                        bridge does not support separate
                                        windows for nonprefetchable and
                                        prefetchable memory. A PCI bus driver
                                        needs to include requests for
                                        prefetchable memory in the
                                        nonprefetchable memory pool.

  EFI_PCI_HOST_BRIDGE_MEM64_DECODE      If this bit is set, then the PCI root
                                        bridge supports 64-bit memory windows.
                                        If this bit is not set, the PCI bus
                                        driver needs to include requests for a
                                        64-bit memory address in the
                                        corresponding 32-bit memory pool.

  @param[in]   This               The instance pointer of
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL

  @param[in]   RootBridgeHandle   The device handle of the PCI root bridge in
                                  which the caller is interested. Type
                                  EFI_HANDLE is defined in
                                  InstallProtocolInterface() in the UEFI 2.0
                                  Specification.

  @param[out]  Attributes         The pointer to attribte of root bridge, it is
                                  output parameter

  @retval EFI_INVALID_PARAMETER   Attribute pointer is NULL

  @retval EFI_INVALID_PARAMETER   RootBridgehandle is invalid.

  @retval EFI_SUCCESS             Success to get attribute of interested root
                                  bridge.
**/
EFI_STATUS
EFIAPI
GetAttributes(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;

  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  *Attributes = HbInstance->Attributes;

  return EFI_SUCCESS;
}

/**
  Sets up the specified PCI root bridge for the bus enumeration process.

  This member function sets up the root bridge for bus enumeration and returns
  the PCI bus range over which the search should be performed in ACPI 2.0
  resource descriptor format.

  @param[in]   This              The
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                                 instance.

  @param[in]   RootBridgeHandle  The PCI Root Bridge to be set up.

  @param[out]  Configuration     Pointer to the pointer to the PCI bus resource
                                 descriptor.

  @retval EFI_INVALID_PARAMETER Invalid Root bridge's handle

  @retval EFI_OUT_OF_RESOURCES  Fail to allocate ACPI resource descriptor tag.

  @retval EFI_SUCCESS           Sucess to allocate ACPI resource descriptor.
**/
EFI_STATUS
EFIAPI
StartBusEnumeration(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  VOID                                  *Buffer;
  UINT8                                 *Temp;

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  if (RootBridgeHandle != HbInstance->RootBridgeHandle) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "PHB%ld StartBusEnumeration()\n", HbInstance->OpalId));

  //
  // Set up the Root Bridge for Bus Enumeration
  //
  Buffer = AllocatePool (
             sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) +
             sizeof(EFI_ACPI_END_TAG_DESCRIPTOR)
             );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem(Buffer, &HbInstance->ConfigBuffer.SpaceDesc[TypeBus],
          sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));

  Temp = (UINT8 *)Buffer;
  Temp = Temp + sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Desc = 0x79;
  ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Checksum = 0x0;

  *Configuration = Buffer;
  return EFI_SUCCESS;
}

/**
  Programs the PCI root bridge hardware so that it decodes the specified PCI
  bus range.

  This member function programs the specified PCI root bridge to decode the bus
  range that is specified by the input parameter Configuration.
  The bus range information is specified in terms of the ACPI 2.0 resource
  descriptor format.

  @param[in] This              The
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                               instance

  @param[in] RootBridgeHandle  The PCI Root Bridge whose bus range is to be
                               programmed

  @param[in] Configuration     The pointer to the PCI bus resource descriptor

  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge
                                 handle.

  @retval EFI_INVALID_PARAMETER  Configuration is NULL.

  @retval EFI_INVALID_PARAMETER  Configuration does not point to a valid ACPI
                                 2.0 resource descriptor.

  @retval EFI_INVALID_PARAMETER  Configuration does not include a valid ACPI
                                 2.0 bus resource descriptor.

  @retval EFI_INVALID_PARAMETER  Configuration includes valid ACPI 2.0 resource
                                 descriptors other than bus descriptors.

  @retval EFI_INVALID_PARAMETER  Configuration contains one or more invalid
                                 ACPI resource descriptors.

  @retval EFI_INVALID_PARAMETER  "Address Range Minimum" is invalid for this
                                 root bridge.

  @retval EFI_INVALID_PARAMETER  "Address Range Length" is invalid for this
                                 root bridge.

  @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.

  @retval EFI_SUCCESS            The bus range for the PCI root bridge was
                                 programmed.
**/
EFI_STATUS
EFIAPI
SetBusNumbers(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  UINT8                                 *Ptr;

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr = Configuration;

  DEBUG ((EFI_D_INFO, "PHB%ld SetBusNumbers()\n", HbInstance->OpalId));

  //
  // Check the Configuration is valid
  //
  if(*Ptr != ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }

  if (((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->ResType != 2) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "  Base=0x%x Len=0x%x\n",
          ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->AddrRangeMin,
          ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Ptr)->AddrLen));

  Ptr += sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  if (*Ptr != ACPI_END_TAG_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  Submits the I/O and memory resource requirements for the specified PCI root
  bridge.

  This function is used to submit all the I/O and memory resources that are
  required by the specified PCI root bridge. The input parameter Configuration
  is used to specify the following:
  - The various types of resources that are required
  - The associated lengths in terms of ACPI 2.0 resource descriptor format

  @param[in] This              Pointer to the
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                               instance.

  @param[in] RootBridgeHandle  The PCI root bridge whose I/O and memory
                               resource requirements are being submitted.

  @param[in] Configuration     The pointer to the PCI I/O and PCI memory
                               resource descriptor.

  @retval EFI_SUCCESS            The I/O and memory resource requests for a PCI
                                 root bridge were accepted.

  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge
                                 handle.

  @retval EFI_INVALID_PARAMETER  Configuration is NULL.

  @retval EFI_INVALID_PARAMETER  Configuration does not point to a valid ACPI
                                 2.0 resource descriptor.

  @retval EFI_INVALID_PARAMETER  Configuration includes requests for one or
                                 more resource types that are not supported by
                                 this PCI root bridge. This error will happen
                                 if the caller did not combine resources
                                 according to Attributes that were returned by
                                 GetAllocAttributes().

  @retval EFI_INVALID_PARAMETER  Address Range Maximum" is invalid.

  @retval EFI_INVALID_PARAMETER  "Address Range Length" is invalid for this PCI
                                 root bridge.

  @retval EFI_INVALID_PARAMETER  "Address Space Granularity" is invalid for
                                 this PCI root bridge.
**/
EFI_STATUS
EFIAPI
SubmitResources(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  UINT8                                 *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     *Ptr;
  //UINT64                                AddrLen;
  //UINT64                                Alignment;

  //
  // Check the input parameter: Configuration
  //
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  if (RootBridgeHandle != HbInstance->RootBridgeHandle) {
    return EFI_INVALID_PARAMETER;
  }
  
  DEBUG ((EFI_D_INFO, "PHB%ld SubmitResouces()\n", HbInstance->OpalId));

  //
  // Validate that we have a number of 0x8A descriptors followed by a 0x79
  //
  Temp = (UINT8 *)Configuration;
  while ( *Temp == 0x8A) {
    Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) ;
  }
  if (*Temp != 0x79) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Iterate the 0x8A descriptors
  //
  Temp = (UINT8 *)Configuration;
  while ( *Temp == 0x8A) {
    Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp ;

    DEBUG ((EFI_D_INFO, "  [%d] F:%02x_%02x\n",
            Ptr->ResType, Ptr->GenFlag, Ptr->SpecificFlag));
    DEBUG ((EFI_D_INFO, "       G=%016lx\n", Ptr->AddrSpaceGranularity));
    DEBUG ((EFI_D_INFO, "       r=%016lx\n", Ptr->AddrRangeMin));
    DEBUG ((EFI_D_INFO, "       R=%016lx\n", Ptr->AddrRangeMax));
    DEBUG ((EFI_D_INFO, "       T=%016lx\n", Ptr->AddrTranslationOffset));
    DEBUG ((EFI_D_INFO, "       L=%016lx\n", Ptr->AddrLen));

    //
    // Check address range alignment
    //
    if (Ptr->AddrRangeMax != (GetPowerOfTwo64 (Ptr->AddrRangeMax + 1) - 1)) {
      DEBUG((EFI_D_ERROR, "PHB%ld: Unaligned range !\n", HbInstance->OpalId));
      return EFI_INVALID_PARAMETER;
    }

    //
    // We don't do much here since we always give out the entire aperture to
    // the bridge, so we just do some sanity checking
    //
    switch (Ptr->ResType) {
    case 0:

      //
      // Check invalid Address Sapce Granularity
      //
      if (Ptr->AddrSpaceGranularity != 32 && Ptr->AddrSpaceGranularity != 64 &&
	  Ptr->AddrLen != 0) {
	DEBUG((EFI_D_ERROR, "PHB%ld: Invalid granularity !\n", HbInstance->OpalId));
        return EFI_INVALID_PARAMETER;
      }

      //
      // check the memory resource request is supported by PCI root bridge
      //
      if (HbInstance->Attributes == EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM &&
          Ptr->SpecificFlag == 0x06) {
        DEBUG((EFI_D_ERROR, "PHB%ld: Unexpected PMEM resource !\n", HbInstance->OpalId));
        return EFI_INVALID_PARAMETER;
      }
      break;

    default:
      break;
    };

    Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) ;
  }

  return EFI_SUCCESS;
}

/**
   Returns the proposed resource settings for the specified PCI root bridge.

   This member function returns the proposed resource settings for the
   specified PCI root bridge. The proposed resource settings are prepared when
   NotifyPhase() is called with a Phase of EfiPciHostBridgeAllocateResources.
   The output parameter Configuration specifies the following:
   - The various types of resources, excluding bus resources, that are
     allocated
   - The associated lengths in terms of ACPI 2.0 resource descriptor format

   @param[in]  This              Pointer to the
                               EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                                 instance.

   @param[in]  RootBridgeHandle  The PCI root bridge handle. Type EFI_HANDLE is
                                 defined in InstallProtocolInterface() in the
                                 UEFI 2.0 Specification.

   @param[out] Configuration     The pointer to the pointer to the PCI I/O and
                                 memory resource descriptor.

   @retval EFI_SUCCESS            The requested parameters were returned.

   @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge
                                  handle.

   @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.

   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                  lack of resources.
**/
EFI_STATUS
EFIAPI
GetProposedResources(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  VOID                                  *Buffer;
  UINT8                                 *Temp;

  Buffer = NULL;

  //
  // Get the Host Bridge Instance from the resource allocation protocol
  //
  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  if (RootBridgeHandle != HbInstance->RootBridgeHandle) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "PHB%ld GetProposedResources()\n", HbInstance->OpalId));

  Buffer = AllocateZeroPool (
               2 * sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof(EFI_ACPI_END_TAG_DESCRIPTOR)
           );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Temp = Buffer;

  CopyMem(Temp, &HbInstance->ConfigBuffer.SpaceDesc[TypeMem32],
    sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
  Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);

  CopyMem(Temp, &HbInstance->ConfigBuffer.SpaceDesc[TypeMem64],
    sizeof(EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
  Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);

  ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Desc = 0x79;
  ((EFI_ACPI_END_TAG_DESCRIPTOR *)Temp)->Checksum = 0x0;

  *Configuration = Buffer;

  return EFI_SUCCESS;
}

/**
  Provides the hooks from the PCI bus driver to every PCI controller
  (device/function) at various stages of the PCI enumeration process that allow
  the host bridge driver to preinitialize individual PCI controllers before
  enumeration.

  This function is called during the PCI enumeration process. No specific
  action is expected from this member function. It allows the host bridge
  driver to preinitialize individual PCI controllers before enumeration.

  @param This              Pointer to the
                           EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                           instance.

  @param RootBridgeHandle  The associated PCI root bridge handle. Type
                           EFI_HANDLE is defined in InstallProtocolInterface()
                           in the UEFI 2.0 Specification.

  @param PciAddress        The address of the PCI device on the PCI bus. This
                           address can be passed to the
                           EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL member functions to
                           access the PCI configuration space of the device.
                           See Table 12-1 in the UEFI 2.0 Specification for the
                           definition of
                           EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS.

  @param Phase             The phase of the PCI device enumeration.

  @retval EFI_SUCCESS              The requested parameters were returned.

  @retval EFI_INVALID_PARAMETER    RootBridgeHandle is not a valid root bridge
                                   handle.

  @retval EFI_INVALID_PARAMETER    Phase is not a valid phase that is defined
                                   in
                                  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE.

  @retval EFI_DEVICE_ERROR         Programming failed due to a hardware error.
                                   The PCI enumerator should not enumerate this
                                   device, including its child devices if it is
                                   a PCI-to-PCI bridge.
**/
EFI_STATUS
EFIAPI
PreprocessController (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL        *This,
  IN  EFI_HANDLE                                              RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS             PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE            Phase
  )
{
  IODA_HOST_BRIDGE_INSTANCE             *HbInstance;
  UINT32                                BDFn;
  INT64                                 OpalStatus;

  HbInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  if (RootBridgeHandle != HbInstance->RootBridgeHandle) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "PHB%ld PreprocessController(Phase=%d, PCIAddress=0x%08x)\n",
    HbInstance->OpalId, Phase, PciAddress));

  if ((UINT32)Phase > EfiPciBeforeResourceCollection) {
    return EFI_INVALID_PARAMETER;
  }

  if (Phase ==  EfiPciBeforeResourceCollection) {
    //
    // Setup all functions as PE#1
    //
    BDFn = (PciAddress.Bus << 8) | (PciAddress.Device << 3) | PciAddress.Function;
    OpalStatus = opal_pci_set_pe(HbInstance->OpalId, 1, BDFn,
                                 OpalPciBusAll,
                                 OPAL_COMPARE_RID_DEVICE_NUMBER,
                                 OPAL_COMPARE_RID_FUNCTION_NUMBER,
                                 OPAL_MAP_PE);
    if (OpalStatus != OPAL_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "%a: Opal error %ld in opal_pci_set_pe()\n",
              __FUNCTION__, OpalStatus));
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

STATIC CONST IODA_HOST_BRIDGE_INSTANCE mHostBridgeInstanceTemplate = {
  .Signature = IODA_HOST_BRIDGE_SIGNATURE,
  .Attributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
		EFI_PCI_HOST_BRIDGE_MEM64_DECODE |
                EFI_PCI_HOST_BRIDGE_NO_IO_DECODE,
  .RbAttributesSupports = EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
  .ResAlloc = {
    .NotifyPhase                = NotifyPhase,
    .GetNextRootBridge          = GetNextRootBridge,
    .GetAllocAttributes         = GetAttributes,
    .StartBusEnumeration        = StartBusEnumeration,
    .SetBusNumbers              = SetBusNumbers,
    .SubmitResources            = SubmitResources,
    .GetProposedResources       = GetProposedResources,
    .PreprocessController       = PreprocessController
  },  
  .Io = {
    .PollMem        = RootBridgeIoPollMem,
    .PollIo         = RootBridgeIoPollIo,
    .Mem.Read       = RootBridgeIoMemRead,
    .Mem.Write      = RootBridgeIoMemWrite,
    .Io.Read        = RootBridgeIoIoRead,
    .Io.Write       = RootBridgeIoIoWrite,
    .CopyMem        = RootBridgeIoCopyMem,
    .Pci.Read       = RootBridgeIoPciRead,
    .Pci.Write      = RootBridgeIoPciWrite,
    .Map            = RootBridgeIoMap,
    .Unmap          = RootBridgeIoUnmap,
    .AllocateBuffer = RootBridgeIoAllocateBuffer,
    .FreeBuffer     = RootBridgeIoFreeBuffer,
    .Flush          = RootBridgeIoFlush,
    .GetAttributes  = RootBridgeIoGetAttributes,
    .SetAttributes  = RootBridgeIoSetAttributes,
    .Configuration  = RootBridgeIoConfiguration,
    .SegmentNumber  = 0,
  },
};

STATIC
VOID
AddHostBridge(
  IN INT32 Node
  )
{
  IODA_HOST_BRIDGE_INSTANCE *Hb;
  UINT64 OpalId;
  CONST UINT64 *OpalIdPtr;
  CONST UINT32 *RangesPtr;
  INT32 RangesLen;
  EFI_STATUS Status;
  UINT64 TceCount, Index;

  OpalIdPtr = fdt_getprop(mFdtBase, Node, "ibm,opal-phbid", NULL);
  if (OpalIdPtr == NULL ) {
    DEBUG ((EFI_D_ERROR, "%a: No ibm,opal-phbid in device-tree\n", __FUNCTION__));
    return;
  }
  OpalId = fdt64_to_cpu(*OpalIdPtr);
  
  Hb = AllocateCopyPool (sizeof(IODA_HOST_BRIDGE_INSTANCE), &mHostBridgeInstanceTemplate);
  if (Hb == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: Memory allocation failed\n", __FUNCTION__));
    return;
  }
  Hb->OpalId = OpalId;
  Hb->FdtNode = Node;

  //
  // Grab the 32-bit memory region
  //
  RangesPtr = fdt_getprop(mFdtBase, Node, "ranges", &RangesLen);
  if (RangesPtr == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: No ranges in device-tree\n", __FUNCTION__));
    FreePool(Hb);
    return;
  }
  RangesLen /= 4;
  while (RangesLen >= 7) {
    if ((fdt32_to_cpu(RangesPtr[0]) & 0x0f000000u) == 0x02000000) {
      Hb->Mem32PciBase = fdt64_to_cpu(*(UINT64 *)&RangesPtr[1]);
      Hb->Mem32CpuBase = fdt64_to_cpu(*(UINT64 *)&RangesPtr[3]);
      Hb->Mem32Size    = fdt64_to_cpu(*(UINT64 *)&RangesPtr[5]);      
    }
    RangesPtr += 7;
    RangesLen -= 7;
  }
  if (Hb->Mem32Size == 0) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to find M32 range\n", __FUNCTION__));
    FreePool(Hb);
    return;
  }
  if ((Hb->Mem32PciBase | Hb->Mem32Size) > 0xffffffffu) {
    DEBUG ((EFI_D_ERROR, "%a: M32 out of 32-bit range\n", __FUNCTION__));
    FreePool(Hb);
    return;
  }

  //
  // Grab the 64-bit memory region
  //
  RangesPtr = fdt_getprop(mFdtBase, Node, "ibm,opal-m64-window", &RangesLen);
  if (RangesPtr != NULL) {
    Hb->Mem64PciBase = fdt64_to_cpu(*(UINT64 *)&RangesPtr[0]);
    Hb->Mem64CpuBase = fdt64_to_cpu(*(UINT64 *)&RangesPtr[2]);
    Hb->Mem64Size    = fdt64_to_cpu(*(UINT64 *)&RangesPtr[4]);
  }

  DEBUG ((EFI_D_INFO, "IODA PHB%ld Found\n", Hb->OpalId));
  DEBUG ((EFI_D_INFO, "  M32 PCI Base:         %08x\n", Hb->Mem32PciBase));
  DEBUG ((EFI_D_INFO, "  M32 CPU Base: %016lx\n", Hb->Mem32CpuBase));
  DEBUG ((EFI_D_INFO, "  M32 Size    :         %08x\n", Hb->Mem32Size));
  DEBUG ((EFI_D_INFO, "  M64 PCI Base: %016lx\n", Hb->Mem64PciBase));
  DEBUG ((EFI_D_INFO, "  M64 CPU Base: %016lx\n", Hb->Mem64CpuBase));
  DEBUG ((EFI_D_INFO, "  M64 Size    : %016lx\n", Hb->Mem64Size));

  //
  // Add them to the system memory map
  //
  Status = gDS->AddMemorySpace(
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Hb->Mem32CpuBase,
                  Hb->Mem32Size,
                  EFI_MEMORY_UC
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: AddMemorySpace (M32): %r\n", __FUNCTION__, Status));
    FreePool(Hb);
    return;
  }

  if (Hb->Mem64Size) {
    Status = gDS->AddMemorySpace(
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Hb->Mem64CpuBase,
                    Hb->Mem64Size,
                    EFI_MEMORY_UC
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%a: AddMemorySpace (M64): %r\n", __FUNCTION__, Status));
      FreePool(Hb);
      return;
    }
  }

  //
  // Prepare for a simple 32-bit DMA map
  //
  // Notes: Our DMA window always starts at 0. We allocate the TCE table as
  // EfiBootServicesData, assuming we won't need it for runtime services,
  // this might have to be revisited...
  //
  Hb->Dma32Base = 0;
  Hb->Dma32Size = Hb->Mem32PciBase;
  TceCount = Hb->Dma32Size >> 12;

  Hb->TceTable32 = (UINT64 *)AllocateAlignedPages(EFI_SIZE_TO_PAGES(TceCount << 3),
                                                  TceCount << 3);
  if (Hb->TceTable32 == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to allocate TCE table\n", __FUNCTION__));
    FreePool(Hb);
    return;
  }
  for (Index = 0; Index < TceCount; Index++) {
    Hb->TceTable32[Index] = SwapBytes64((Index << 12) | 3);
  }

  //
  // Install protocols
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Hb->HostBridgeHandle,
                  &gEfiPciHostBridgeResourceAllocationProtocolGuid,
                  &Hb->ResAlloc,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to install HB protocol interfaces\n", __FUNCTION__));
    FreePool (Hb);
    return;
  }

  CopyMem (&Hb->DevicePath, &mPciRootDevPathTemplate,
    sizeof mPciRootDevPathTemplate);
  Hb->DevicePath.Acpi.UID = Hb->OpalId;
  
  RootBridgeIoInit (Hb);

  Status = gBS->InstallMultipleProtocolInterfaces(
                      &Hb->RootBridgeHandle,
                      &gEfiDevicePathProtocolGuid, &Hb->DevicePath,
                      &gEfiPciRootBridgeIoProtocolGuid, &Hb->Io,
                      NULL
                      );
  if (EFI_ERROR (Status)) {
    FreePool(Hb);
    return;
  }
}

/**
  Entry point of this driver

  @param ImageHandle     Handle of driver image
  @param SystemTable     Point to EFI_SYSTEM_TABLE

  @retval EFI_ABORTED           PCI host bridge not present
  @retval EFI_OUT_OF_RESOURCES  Can not allocate memory resource
  @retval EFI_DEVICE_ERROR      Can not install the protocol instance
  @retval EFI_SUCCESS           Success to initialize the Pci host bridge.
**/
EFI_STATUS
EFIAPI
InitializePciHostBridge (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  INT32                          Node, Prev;
  EFI_STATUS                     Status;

  mDriverImageHandle = ImageHandle;

  //
  // Find Device Tree
  //
  Status = GetFdt();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: GetFdt: %r\n", __FUNCTION__, Status));
    return Status;
  }
  
  //
  // Iterate it to find IODA2 host bridges
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (mFdtBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    if (fdt_node_check_compatible(mFdtBase, Node, "ibm,ioda2-phb") != 0) {
      continue;
    }

    AddHostBridge(Node);
  }
  return EFI_SUCCESS;
}

  
