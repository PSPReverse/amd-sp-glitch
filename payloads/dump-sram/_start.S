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

.text
.arm

_start:

  /* write magic register */
  ldr r0, =0x3006000
  mov r1, #1
  str r1, [r0]

  /* clear smn slots */
  mov r0, #0x0
  ldr r1, =SMN_SLOT_BASE
  ldr r2, =SMN_SLOT_COUNT

clear_smn_slots:
  str r0, [r1], #4
  subs r2, r2, #2
  bne clear_smn_slots

  /* map flash */
  ldr r0, =SMN_FLASH_ADDR
  lsr r0, r0, #20 // smn address high part
  ldr r1, =SMN_SLOT_BASE
  str r0, [r1] // map to first smn slot
  ldr r1, =SMN_MAP_BASE

  /* dump sram contents */
  mov r0, #0
  mov r3, #0x50000
sram_write_loop:
  ldr r2, [r0], #4
  str r2, [r1], #4
  cmp r0, r3
  bne sram_write_loop

  /* busy loop */
end_loop:
  b end_loop

