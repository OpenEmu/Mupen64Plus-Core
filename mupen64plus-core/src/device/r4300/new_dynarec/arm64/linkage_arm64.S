/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - linkage_arm64.S                                         *
 *   Copyright (C) 2009-2018 Gillou68310                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(__APPLE__)
/* Mach-O knows no .type, .size or .hidden, and C identifiers carry a leading
 * underscore. The definitions use assembler macros rather than preprocessor
 * ones because the label has to start a line of its own, which a #define
 * cannot produce. */
.macro GLOBAL_FUNCTION name
    .globl _\name
_\name:
.endm

.macro LOCAL_FUNCTION name
_\name:
.endm

.macro GLOBAL_VARIABLE name, size_
    .globl _\name
.endm

#define EXTERN_SYMBOL(name) _##name
/* A page-relative reference is spelled differently: Mach-O names the page and
 * the offset explicitly, ELF uses :lo12: on the add. */
#define PAGE_EXTERN(name) _##name@PAGE
#define PAGEOFF_EXTERN(name) _##name@PAGEOFF
#else
.macro GLOBAL_FUNCTION name
    .globl \name
    .hidden \name
    .type \name, %function
\name:
.endm

.macro LOCAL_FUNCTION name
    .hidden \name
    .type \name, %function
\name:
.endm

.macro GLOBAL_VARIABLE name, size_
    .globl \name
    .hidden \name
    .type \name, %object
    .size \name, \size_
.endm

#define EXTERN_SYMBOL(name) name
#define PAGE_EXTERN(name) name
#define PAGEOFF_EXTERN(name) :lo12:name
#endif

.macro movl Wn, imm
    movz    \Wn, (\imm >> 16) & 0xFFFF, lsl 16
    movk    \Wn,  \imm & 0xFFFF
.endm

#define TEXT_SECTION .text
#define END_SECTION

#include "asm_defines_gas.h"

device_r4300_new_dynarec_hot_state_dynarec_local = (offsetof_struct_device_r4300 + offsetof_struct_r4300_core_new_dynarec_hot_state + offsetof_struct_new_dynarec_hot_state_dynarec_local)
saved_context = 160

/* Defines offsets for fp addressed variables */
fp_cycle_count         = offsetof_struct_new_dynarec_hot_state_cycle_count
fp_pending_exception   = offsetof_struct_new_dynarec_hot_state_pending_exception
fp_pcaddr              = offsetof_struct_new_dynarec_hot_state_pcaddr
fp_stop                = offsetof_struct_new_dynarec_hot_state_stop

TEXT_SECTION

GLOBAL_FUNCTION jump_vaddr_x0
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x1
    mov    w0, w1
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x2
    mov    w0, w2
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x3
    mov    w0, w3
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x4
    mov    w0, w4
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x5
    mov    w0, w5
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x6
    mov    w0, w6
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x8
    mov    w0, w8
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x9
    mov    w0, w9
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x10
    mov    w0, w10
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x11
    mov    w0, w11
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x12
    mov    w0, w12
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x13
    mov    w0, w13
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x14
    mov    w0, w14
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x15
    mov    w0, w15
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x16
    mov    w0, w16
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x17
    mov    w0, w17
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x19
    mov    w0, w19
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x21
    mov    w0, w21
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x22
    mov    w0, w22
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x23
    mov    w0, w23
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x24
    mov    w0, w24
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x25
    mov    w0, w25
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x26
    mov    w0, w26
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x27
    mov    w0, w27
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x28
    mov    w0, w28
    b      EXTERN_SYMBOL(jump_vaddr)

GLOBAL_FUNCTION jump_vaddr_x7
    mov    w0, w7

GLOBAL_FUNCTION jump_vaddr
    bl     EXTERN_SYMBOL(get_addr_ht)
    br     x0

GLOBAL_FUNCTION verify_code
    /* x0 = head */
    mov    x21, x30 /* Save link register */
    bl     EXTERN_SYMBOL(verify_dirty)
    tst    x0, x0
    b.ne   .D1
    mov    x30, x21 /* Restore link register */
    ret
.D1:
    bl     EXTERN_SYMBOL(get_addr)
    br     x0

GLOBAL_FUNCTION cc_interrupt
    str    w20, [x29, #fp_cycle_count]
    str    wzr, [x29, #fp_pending_exception]
    mov    x20, x30 /* Save link register */
    bl     EXTERN_SYMBOL(dynarec_gen_interrupt)
    mov    x30, x20 /* Restore link register */
    ldr    w20, [x29, #fp_cycle_count]
    ldr    w1, [x29, #fp_pending_exception]
    ldr    w2, [x29, #fp_stop]
    tst    w2, w2
    b.ne   EXTERN_SYMBOL(new_dyna_stop)
    tst    w1, w1
    b.ne   .E1
    ret
.E1:
    ldr    w0, [x29, #fp_pcaddr]
    bl     EXTERN_SYMBOL(get_addr_ht)
    br     x0

LOCAL_FUNCTION new_dyna_stop
    add    x16, x29, #saved_context
    ldp    x19,x20,[x16,#0]
    ldp    x21,x22,[x16,#16]
    ldp    x23,x24,[x16,#32]
    ldp    x25,x26,[x16,#48]
    ldp    x27,x28,[x16,#64]
    ldp    x29,x30,[x16,#80]
    ret

GLOBAL_FUNCTION do_interrupt
    ldr    w2, [x29, #fp_stop]
    tst    w2, w2
    b.ne   EXTERN_SYMBOL(new_dyna_stop)
    ldr    w0, [x29, #fp_pcaddr]
    bl     EXTERN_SYMBOL(get_addr_ht)
    ldr    w20, [x29, #fp_cycle_count]
    br     x0

GLOBAL_FUNCTION fp_exception
    str    w0, [x29, #fp_pcaddr]
    bl     EXTERN_SYMBOL(cop1_unusable)
    br     x0

GLOBAL_FUNCTION jump_syscall
    str    w0, [x29, #fp_pcaddr]
    bl     EXTERN_SYMBOL(SYSCALL_new)
    br     x0

GLOBAL_FUNCTION jump_eret
    str    w20, [x29, #fp_cycle_count]
    bl     EXTERN_SYMBOL(ERET_new)
    ldr    w20, [x29, #fp_cycle_count]
    tst    w0, w0
    b.eq   EXTERN_SYMBOL(new_dyna_stop)
    br     x0

GLOBAL_FUNCTION dyna_linker
    bl     EXTERN_SYMBOL(dynamic_linker)
    br     x0

GLOBAL_FUNCTION dyna_linker_ds
    bl     EXTERN_SYMBOL(dynamic_linker_ds)
    br     x0

GLOBAL_FUNCTION new_dyna_start
    adrp   x16, PAGE_EXTERN(g_dev)
    add    x16, x16, PAGEOFF_EXTERN(g_dev)
    movl   x1, (device_r4300_new_dynarec_hot_state_dynarec_local + saved_context)
    add    x16, x16, x1
    adrp   x1, PAGE_EXTERN(base_addr_rx)
    add    x1, x1, PAGEOFF_EXTERN(base_addr_rx)
    mov    w0, #0xa4000000
    stp    x19,x20,[x16,#0]
    stp    x21,x22,[x16,#16]
    stp    x23,x24,[x16,#32]
    stp    x25,x26,[x16,#48]
    stp    x27,x28,[x16,#64]
    stp    x29,x30,[x16,#80]
    sub    x29, x16, #saved_context
    ldr    x19, [x1]
    add    w0, w0, #0x40
    bl     EXTERN_SYMBOL(new_recompile_block)
    ldr    w20, [x29, #fp_cycle_count]
    br     x19

GLOBAL_FUNCTION breakpoint
    brk 0
    ret

END_SECTION