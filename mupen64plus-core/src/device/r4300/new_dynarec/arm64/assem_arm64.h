#ifndef M64P_DEVICE_R4300_NEW_DYNAREC_ARM_ASSEM_ARM64_H
#define M64P_DEVICE_R4300_NEW_DYNAREC_ARM_ASSEM_ARM64_H

#define HOST_REGS 29
#define HOST_CCREG 20 /* callee-save */
#define HOST_BTREG 19 /* callee-save */
/* Host registers 0-28 are x0-x28; x29/fp holds &dynarec_local and already lies
   outside HOST_REGS, the selection loops run hr<HOST_REGS. On Darwin x18 is the
   reserved platform register - the operating system and the runtime may
   overwrite it at any time. The register allocator therefore has to skip it,
   otherwise a guest register (in the failing case ROREG, the RAM offset) holds
   no stable value there. */
#if defined(__APPLE__)
#define EXCLUDE_REG 18 /* x18: reserved by Darwin */
#else
#define EXCLUDE_REG 29 /* FP - without effect, see above */
#endif

#define NATIVE_64 1
#define HOST_IMM8 1
//#define HAVE_CMOV_IMM 1
//#define CORTEX_A8_BRANCH_PREDICTION_HACK 1
//#define REG_PREFETCH 1
//#define HAVE_CONDITIONAL_CALL 1
// The RAM offset is kept in a register and added in the generated code as
// [guest address + offset<<2]. Without RAM_OFFSET "map" stays -1, and in the
// emitter map==-1 means "the address is already a host address" - which only
// holds for the 32-bit x86 dynarec whose guest and host address spaces are
// identical (RDRAM lives at 0x80000000 there). On arm64 that then addresses the
// bare guest address space and crashes (evidenced by ldr w4,[x1] with
// x1=0x80000400 in Mario Kart 64). RAM_OFFSET therefore has to stay on here; the
// register holding the offset just must not be x18 (see EXCLUDE_REG).
#define RAM_OFFSET 1
#define USE_MINI_HT 1

/* ARM calling convention:
   x0-x18: caller-save
   x19-x28: callee-save */

#define ARG1_REG 0
#define ARG2_REG 1
#define ARG3_REG 2
#define ARG4_REG 3

/* GCC register naming convention:
   x16 = ip0 (scratch)
   x17 = ip1 (scratch)
   x29 = fp (frame pointer)
   x30 = lr (link register)
   x31 = sp (stack pointer) */

#define FP 29
#define LR 30
#define WZR 31
#define XZR WZR
#define CALLER_SAVED_REGS 0x7ffff
#define HOST_TEMPREG 30

// Note: FP is set to &dynarec_local when executing generated code.
// Thus the local variables are actually global and not on the stack.

#define TARGET_SIZE_2 25 // 2^25 = 32 megabytes
#define JUMP_TABLE_SIZE (sizeof(jump_table_symbols)*2)

#endif /* M64P_DEVICE_R4300_NEW_DYNAREC_ARM_ASSEM_ARM64_H */
