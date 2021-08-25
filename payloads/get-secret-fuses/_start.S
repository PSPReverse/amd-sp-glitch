// Copyright (C) 2021 Niklas Jacob, Robert Buhren
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/* SMN defines */
#define SMN_SLOT_BASE 0x3220000
#define SMN_SLOT_COUNT 0x20

#define SMN_MAP_BASE 0x1000000
#define SMN_MAP_LEN 0x100000

#define SMN_FLASH_ADDR 0x44000000

/* CCP defines */
#define CCP_BASE 0x3001000
#define TAIL_OFF 0x4
#define HEAD_OFF 0x8
#define STATUS_REG_OFF 0x100

/* memory defines */
#define DESC_SCRATCH 0x15000
#define RESULT_SCRATCH 0x10000

_start:

  /* write magic register */
  ldr r0, =0x3006000
  mov r1, #1
  str r1, [r0]

  /* clear smn slots */
  mov r0, #0x0
  ldr r1, =SMN_SLOT_BASE
  ldr r2, =SMN_SLOT_COUNT
_clear_smn_slots:
  str r0, [r1], #4
  subs r2, r2, #2
  bne _clear_smn_slots

  /* map flash */
  ldr r0, =SMN_FLASH_ADDR
  lsr r0, r0, #20 // smn address high part
  ldr r1, =SMN_SLOT_BASE
  str r0, [r1] // map to first smn slot
  ldr r3, =SMN_MAP_BASE

  /* test write */
  ldr r0, =0xdeadbeef
  str r0, [r3]
  
  /* Create descriptor */
  ldr r0, =DESC_SCRATCH

  // engine passthru (no-op)
  ldr r1, =0x00500011
  str r1, [r0], #4

  // length is 6*32 byte (0xc0)
  mov r1, #0xc0
  str r1, [r0], #4

  // source is at address 0
  mov r1, #0
  str r1, [r0], #4

  // no high address, mem_type = lsb
  mov r1, #0x10000
  str r1, [r0], #4

  // destination
  ldr r1, =RESULT_SCRATCH
  str r1, [r0], #4

  // no high address, mem_type = psp_ram
  mov r1, #0x20000
  str r1, [r0], #4

  // no key
  mov r1, #0
  str r1, [r0], #4
  str r1, [r0], #4

  /* Write TAIL reg */
  ldr r1, =CCP_BASE
  str r0, [r1, #TAIL_OFF]

  /* Write head reg */
  sub r0, r0, #0x20
  str r0, [r1, #HEAD_OFF]

  /* Start CCP cmd */
  mov r0, #0x17
  str r0, [r1]

  /* Wait until ccp command completes */
_wait:
  ldr r0, [r1]
  and r0, r0, #3
  cmp r0, #2
  bne _wait

  /* Read error code */
  ldr r0, [r1, #STATUS_REG_OFF]
  and r0, r0, #0x3f
  
  ldr r1, =SMN_MAP_BASE
  str r0, [r1]


  /* Write results */
  mov r0, #0x30
  ldr r1, =SMN_MAP_BASE
  ldr r2, =RESULT_SCRATCH
_write_loop:
  ldr r3, [r2], #4
  str r3, [r1], #4
  subs r0, r0, #1
  bne _write_loop

_endless:
  b _endless

 /* vim: ft=gas :
 */ 
