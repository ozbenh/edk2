/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "LinuxLoader.h"

#include "elf_common.h"
#include "elf64.h"

#define IS_DEVICE_PATH_NODE(node,type,subtype) (((node)->Type == (type)) && ((node)->SubType == (subtype)))

STATIC
EFI_STATUS
ShutdownUefiBootServices (
  VOID
  )
{
  EFI_STATUS              Status;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINTN                   MapKey;
  UINTN                   DescriptorSize;
  UINT32                  DescriptorVersion;
  UINTN                   Pages;

  MemoryMap = NULL;
  MemoryMapSize = 0;
  Pages = 0;

  do {
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {

      Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
      MemoryMap = AllocatePages (Pages);

      //
      // Get System MemoryMap
      //
      Status = gBS->GetMemoryMap (
                      &MemoryMapSize,
                      MemoryMap,
                      &MapKey,
                      &DescriptorSize,
                      &DescriptorVersion
                      );
    }

    // Don't do anything between the GetMemoryMap() and ExitBootServices()
    if (!EFI_ERROR(Status)) {
      Status = gBS->ExitBootServices (gImageHandle, MapKey);
      if (EFI_ERROR(Status)) {
        FreePages (MemoryMap, Pages);
        MemoryMap = NULL;
        MemoryMapSize = 0;
      }
    }
  } while (EFI_ERROR(Status));

  return Status;
}

STATIC
VOID
PrintDevicePath(
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevicePathToTextProtocol;
  EFI_STATUS                        Status;
  CHAR16*                           DevicePathTxt;

  Status = gBS->LocateProtocol (
                     &gEfiDevicePathToTextProtocolGuid,
                     NULL,
                     (VOID **)&DevicePathToTextProtocol
                     );
  ASSERT_EFI_ERROR (Status);

  DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (
                                              DevicePath,
					      TRUE,
					      TRUE
					      );
  Print (L"%s", DevicePathTxt);
}

STATIC
EFI_STATUS
BdsConnectAndUpdateDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT    EFI_HANDLE                *Handle,
  OUT    EFI_DEVICE_PATH_PROTOCOL  **RemainingDevicePath
  )
{
  EFI_DEVICE_PATH*            Remaining;
  EFI_STATUS                  Status;
  EFI_HANDLE                  PreviousHandle;

  if ((DevicePath == NULL) || (*DevicePath == NULL) || (Handle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Print (L"Connect/Update path=");
  PrintDevicePath(*DevicePath);
  Print (L"\n");

  PreviousHandle = NULL;
  do {
    Remaining = *DevicePath;

    // The LocateDevicePath() function locates all devices on DevicePath
    // that support Protocol and returns the handle to the device that is
    // closest to DevicePath. On output, the device path pointer is modified
    // to point to the remaining part of the device path
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Remaining, Handle);

    Print (L"Locate result %r Remaining=", Status);
    PrintDevicePath(Remaining);
    Print (L"\n");

  if (!EFI_ERROR (Status)) {
      if (*Handle == PreviousHandle) {
        //
        // If no forward progress is made try invoking the Dispatcher.
        // A new FV may have been added to the system and new drivers
        // may now be found.
        // Status == EFI_SUCCESS means a driver was dispatched
        // Status == EFI_NOT_FOUND means no new drivers were dispatched
        //
        //Status = gDS->Dispatch ();
	Print (L"No forward progress !\n");
      }

      if (!EFI_ERROR (Status)) {
        PreviousHandle = *Handle;

        // Recursive = FALSE: We do not want to start the whole device tree
        Status = gBS->ConnectController (*Handle, NULL, Remaining, FALSE);
      }
    }
  } while (!EFI_ERROR (Status) && !IsDevicePathEnd (Remaining));

  if (!EFI_ERROR (Status)) {
    // Now, we have got the whole Device Path connected, call again ConnectController
    // to ensure all the supported Driver Binding Protocol are connected (such as DiskIo
    // and SimpleFileSystem)
    Remaining = *DevicePath;
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Remaining, Handle);
    if (!EFI_ERROR (Status)) {
      Status = gBS->ConnectController (*Handle, NULL, Remaining, FALSE);
      if (EFI_ERROR (Status)) {
        // If the last node is a Memory Map Device Path just return EFI_SUCCESS.
        if ((Remaining->Type == HARDWARE_DEVICE_PATH) && (Remaining->SubType == HW_MEMMAP_DP)) {
            Status = EFI_SUCCESS;
        }
      }
    }
  } else {
    // XXX HACK
    //return Status;
    Status = EFI_SUCCESS;
  }

  if (RemainingDevicePath) {
    *RemainingDevicePath = Remaining;
  }

  return Status;
}

EFI_STATUS
BdsFileSystemLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            Handle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS  *Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS                       Status;
  FILEPATH_DEVICE_PATH             *FilePathDevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FsProtocol;
  EFI_FILE_PROTOCOL                *Fs;
  EFI_FILE_INFO                    *FileInfo;
  EFI_FILE_PROTOCOL                *File;
  UINTN                            Size;

  ASSERT (IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP));

  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)RemainingDevicePath;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&FsProtocol,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Try to Open the volume and get root directory
  Status = FsProtocol->OpenVolume (FsProtocol, &Fs);
  if (EFI_ERROR (Status)) {
    goto CLOSE_PROTOCOL;
  }

  Status = Fs->Open (Fs, &File, FilePathDevicePath->PathName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    goto CLOSE_PROTOCOL;
  }

  Size = 0;
  File->GetInfo (File, &gEfiFileInfoGuid, &Size, NULL);
  FileInfo = AllocatePool (Size);
  Status = File->GetInfo (File, &gEfiFileInfoGuid, &Size, FileInfo);
  if (EFI_ERROR (Status)) {
    goto CLOSE_FILE;
  }

  // Get the file size
  Size = FileInfo->FileSize;
  if (ImageSize) {
    *ImageSize = Size;
  }
  FreePool (FileInfo);

  Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  // Try to allocate in any pages if failed to allocate memory at the defined location
  if ((Status == EFI_OUT_OF_RESOURCES) && (Type != AllocateAnyPages)) {
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  }
  if (!EFI_ERROR (Status)) {
    Status = File->Read (File, &Size, (VOID*)(UINTN)(*Image));
  }

CLOSE_FILE:
  File->Close (File);

CLOSE_PROTOCOL:
  gBS->CloseProtocol (
         Handle,
         &gEfiSimpleFileSystemProtocolGuid,
         gImageHandle,
         Handle);

  return Status;
}

EFI_STATUS
BdsLoadImage (
  IN OUT EFI_DEVICE_PATH       *DevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *FileSize
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      Handle;
  EFI_DEVICE_PATH *RemainingDevicePath;

  Status = BdsConnectAndUpdateDevicePath (&DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return BdsFileSystemLoadImage(&DevicePath, Handle, RemainingDevicePath, Type, Image, FileSize);
}

STATIC
VOID
PreparePlatformHardware (
  VOID
  )
{
  PpcDisableInterrupts();
}

STATIC
EFI_STATUS
StartLinux (
  IN  EFI_PHYSICAL_ADDRESS  LinuxImageEntry,
  IN  EFI_PHYSICAL_ADDRESS  FdtBlobBase
  )
{
  typedef  VOID (*LINUX_ENTRY)(VOID *Fdt, UINTN r4, UINTN r5, UINTN r6, UINTN r7);

  EFI_STATUS            Status;
  LINUX_ENTRY		Entry = (LINUX_ENTRY)LinuxImageEntry;

  //
  // Shut down UEFI boot services.
  //
  // ExitBootServices() will notify every driver that created an event on
  // ExitBootServices event. Example the Interrupt DXE driver will disable
  // the interrupts on this event.
  //
  DEBUG((EFI_D_INFO, "Shutting down services...\n"));
  Status = ShutdownUefiBootServices ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ERROR: Can not shutdown UEFI boot services. Status=0x%X\n", Status));
    return Status;
  }
  
  //
  // Switch off interrupts, caches, mmu, etc
  //
  DEBUG((EFI_D_INFO, "Prepare...\n"));
  PreparePlatformHardware ();

  // Register and print out performance information
  PERF_END (NULL, "BDS", NULL, 0);
  //  if (PerformanceMeasurementEnabled ()) {
  //    PrintPerformance ();
  //  }

  //
  // Start the Linux Kernel
  //
  DEBUG((EFI_D_INFO, "Calling %p...\n", Entry));
  Entry((VOID *)FdtBlobBase, 0, 0, 0x65504150, 0x30000000); // FIXME

  // Kernel should never exit
  // After Life services are not provided
  ASSERT (FALSE);

  // We cannot recover the execution at this stage
  while (1);
}

EFI_STATUS
CheckLinuxElf (
  IN EFI_PHYSICAL_ADDRESS  LinuxImage,
  IN UINTN                 LinuxImageSize,
  OUT UINTN                *LinuxMemorySize,
  OUT EFI_PHYSICAL_ADDRESS *LinuxEntryPoint)
{
    Elf64_Ehdr *kh = (Elf64_Ehdr *)LinuxImage;
    Elf64_Phdr *ph;
    UINTN i;

    /* Check it's a ppc64 LE ELF */
    if (!IS_ELF(*kh)) {
      Print (L"ERROR: Not an ELF image !\n");
      return EFI_UNSUPPORTED;
    }
    if (kh->e_machine != EM_PPC64) {
      Print (L"ERROR: Not a PPC64 image !\n");
      return EFI_UNSUPPORTED;
    }    
    if (kh->e_ident[EI_DATA] != ELFDATA2LSB) {
      Print (L"ERROR: Big-endian images not (yet) supported !\n");
      return EFI_UNSUPPORTED;
    }

    // XXX FIXME
    *LinuxMemorySize = 0;
    *LinuxEntryPoint = 0;

    /* Look for a loadable program header that has our entry in it
     *
     * Note that we execute the kernel in-place, we don't actually
     * obey the load informations in the headers. This is expected
     * to work for the Linux Kernel because it's a fairly dumb ELF
     * but it will not work for any ELF binary.
     */
    ph = (Elf64_Phdr *)(LinuxImage + kh->e_phoff);
    for (i = 0; i < kh->e_phnum; i++, ph++) {
      if (ph->p_type != PT_LOAD)
	    continue;

      if ((ph->p_vaddr > kh->e_entry) ||
	  (ph->p_vaddr + ph->p_memsz) < kh->e_entry)
        continue;

      /* Get our entry */
      *LinuxEntryPoint = LinuxImage + kh->e_entry - ph->p_vaddr + ph->p_offset;
    }

    if (!LinuxEntryPoint) {
      Print (L"ERROR: Failed to find kernel entry !\n");
      return EFI_UNSUPPORTED;
    }
    return EFI_SUCCESS;
}

/**
  Start a Linux kernel from a Device Path

  @param[in]  LinuxKernelDevicePath  Device Path to the Linux Kernel
  @param[in]  InitrdDevicePath       Device Path to the Initrd
  @param[in]  Arguments              Linux kernel arguments

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BootLinuxFdt (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments
  )
{
  EFI_STATUS               Status;
  UINTN                    LinuxImageSize;
  UINTN                    LinuxMemorySize;
  UINTN                    LinuxEntryPoint;
  UINTN                    InitrdImageSize;
  VOID                     *InstalledFdtBase;
  UINTN                    FdtBlobSize;
  EFI_PHYSICAL_ADDRESS     FdtBlobBase;
  EFI_PHYSICAL_ADDRESS     LinuxImage;
  EFI_PHYSICAL_ADDRESS     InitrdImage;

  InitrdImage = 0;
  InitrdImageSize = 0;

  PERF_START (NULL, "BDS", NULL, 0);

  //
  // Load the Linux kernel from a device path
  //

  // Try to put the kernel at the start of RAM so as to give it access to all memory.
  // If that fails fall back to try loading it within LINUX_KERNEL_MAX_OFFSET of memory start.
  LinuxImage = 0;
  Status = BdsLoadImage (LinuxKernelDevicePath, AllocateAnyPages, &LinuxImage, &LinuxImageSize);
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Did not find Linux kernel (%r).\n", Status);
    return Status;
  }
  Print (L"Linux loaded at 0x%lx\n", LinuxImage);

  Status = CheckLinuxElf (LinuxImage, LinuxImageSize, &LinuxMemorySize, &LinuxEntryPoint);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print (L"Linux entry point at 0x%lx\n", LinuxEntryPoint);
  
  if (InitrdDevicePath) {
    InitrdImage = 0;
    Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImage, &InitrdImageSize);
    if (EFI_ERROR (Status)) {
      Print (L"ERROR: Did not find initrd image (%r).\n", Status);
      goto EXIT_FREE_LINUX;
    }
    Print (L"Initrd loaded at 0x%lx\n", InitrdImage);
  }

  // XXX WARNING !!! BUG !!!
  //
  // At the moment we just allocate pages without constraints for the kernel,
  // ramdisk and device-tree. However that means that the ramdisk and device
  // tree can be overwritten by either the kernel BSS or the kernel relocating
  // itself !
  //
  // We need to fix that by effectively stop doing allocations all over the
  // place and instead either load the ELF headers first to figure out the
  // total memory footprint, and allocate a big chunk for the kernel, the
  // ramdisk and the DT all together laid out in that order, or somewhat
  // reserve a big chunk of memory down near 0 for the kernel to move itself
  //
  // Things will be easier when this implementaiton of UEFI knows about RAM
  // above 768M, so we can load things high and let them relocate down.
  //

  //
  // Get the FDT from the Configuration Table.
  // The FDT will be reloaded in PrepareFdt() to a more appropriate
  // location for the Linux Kernel and adjusted
  //
  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &InstalledFdtBase);
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Did not get the Device Tree blob (%r).\n", Status);
    goto EXIT_FREE_INITRD;
  }
  FdtBlobBase = (EFI_PHYSICAL_ADDRESS)InstalledFdtBase;
  FdtBlobSize = fdt_totalsize (InstalledFdtBase);

  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareFdt (Arguments, InitrdImage, InitrdImageSize, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Can not load Linux kernel with Device Tree. Status=0x%X\n", Status);
    goto EXIT_FREE_FDT;
  }

  return StartLinux (LinuxEntryPoint, FdtBlobBase);

EXIT_FREE_FDT:
  gBS->FreePages (FdtBlobBase, EFI_SIZE_TO_PAGES (FdtBlobSize));

EXIT_FREE_INITRD:
  if (InitrdDevicePath) {
    gBS->FreePages (InitrdImage, EFI_SIZE_TO_PAGES (InitrdImageSize));
  }

EXIT_FREE_LINUX:
  gBS->FreePages (LinuxImage, EFI_SIZE_TO_PAGES (LinuxImageSize));

  return Status;
}
