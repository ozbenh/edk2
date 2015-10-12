#ifndef __EXCEPTION_HANDLER_H__
#define __EXCEPTION_HANDLER_H__

// Size of redzone
#define EXC_REDZONE_SIZE    288

#define EXC_Rn(__n)   (32 + 8 * (__n))
#define EXC_Fn(__n)   ((EXC_Rn(32) + 8 * (__n)))
#define EXC_EXCNUM    (EXC_Fn(32) + 0x00)
#define EXC_CR        (EXC_Fn(32) + 0x04)
#define EXC_LR        (EXC_Fn(32) + 0x08)
#define EXC_CTR       (EXC_Fn(32) + 0x10)
#define EXC_XER       (EXC_Fn(32) + 0x18)
#define EXC_CFAR      (EXC_Fn(32) + 0x20)
#define EXC_PC        (EXC_Fn(32) + 0x28)
#define EXC_MSR       (EXC_Fn(32) + 0x30)
#define EXC_DAR       (EXC_Fn(32) + 0x38)
#define EXC_DSISR     (EXC_Fn(32) + 0x40)
#define EXC_PAD       (EXC_Fn(32) + 0x48)
#define EXC_Vn(__n)   (EXC_PAD + 8 + 16 * (__n))
#define EXC_Xn(__n)   (EXC_Vn(32) + 8 * (__n))
#define EXC_SIZE      (EXC_Xn(32) + EXC_REDZONE_SIZE)

#ifndef __ASSEMBLY__

#include <Protocol/DebugSupport.h>

VOID
ExceptionReturn(
  IN CONST EFI_SYSTEM_CONTEXT SystemContext
  )  __attribute__((noreturn));

// Markers in the ASM code
extern CHAR8 __ExceptionStubStart;
extern CHAR8 __ExceptionStubPatchVNum;
extern CHAR8 __ExceptionStubPatchBranch;
extern CHAR8 __ExceptionStubEnd;
extern CHAR8 __ExceptionTrampolineStart;
extern CHAR8 __ExceptionTrampolineEnd;

#endif /* __ASSEMBLY__ */

#endif /* __EXCEPTION_HANDLER_H__ */
