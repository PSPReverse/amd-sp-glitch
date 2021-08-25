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

/* First request:

This decrypts the wrapped ikek.

CCP Request 0x0003f900:
    u32Dw0:             0x00000013 (Engine: AES, AES Type: AES128, Mode: ECB, Encrypt: 0, Size: 0)
    cbSrc:              16
    u32AddrSrcLow:      0x0003fa00
    u16AddrSrcHigh:     0x00000000
    u16SrcMemType:      0x00000102 (MemType: 2, LsbCtxId: 64, Fixed: 0)
    u32AddrDstLow:      0x0003f890
    u16AddrDstHigh:     0x00000000
    u16DstMemType:      0x00000002 (MemType: 2, Fixed: 0)
    u32AddrKeyLow:      0x00000080
    u16AddrKeyHigh:     0x00000000
    u16KeyMemType:      0x00000001
*/

/* Second request:
CCP Request 0x0003f900:
    u32Dw0:             0x00002013 (Engine: AES, AES Type: AES128, Mode: CBC, Encrypt: 0, Size: 0)
    cbSrc:              52544
    u32AddrSrcLow:      0x02077800
    u16AddrSrcHigh:     0x00000000
    u16SrcMemType:      0x00000102 (MemType: 2, LsbCtxId: 64, Fixed: 0)
    u32AddrDstLow:      0x00000100
    u16AddrDstHigh:     0x00000000
    u16DstMemType:      0x00000002 (MemType: 2, Fixed: 0)
    u32AddrKeyLow:      0x00000820
    u16AddrKeyHigh:     0x00000000
    u16KeyMemType:      0x00000001
*/

/* Address defines */
#define CCP_BASE 0x3001000
#define CCP_MAX 0x6000

#define SMN_SLOT_BASE 0x3220000
#define SMN_SLOT_COUNT 0x20
#define SMN_MAP_BASE 0x1000000
#define SMN_MAP_LEN 0x100000

#define SMN_FLASH_ADDR 0x44000000
#define IKEK_FLASH_ADDR 0x9f000

#define DESC_SCRATCH 0x15000
#define RESULT_SCRATCH 0x10000

/* Pre-defined dwords */
#define DW0 0x00000013 /* AES Decrypt */
#define DW1 0x10 /* Size */
//#define DW2 0x02072500 /* Location of wrapped IKEK in selfread flash image */
#define DW3 0x01020000
//#define DW4 0x02000000 /* Address 0 of flash image */
#define DW5 0x00020000
#define DW6 0x00000080
#define DW7 0x00010000

/* Dword offsets */
#define DW1_OFF 4
#define DW2_OFF 8
#define DW3_OFF 0xc
#define DW4_OFF 0x10
#define DW5_OFF 0x14
#define DW6_OFF 0x18
#define DW7_OFF 0x1c

/* SB related defines */
#define SB_SHIFT 5
#define LSB_MAX 128

/* CCP related defines */
#define QUEUE_LEN 0x1000
#define QUEUE_COUNT 6
#define TAIL_OFF 0x4
#define HEAD_OFF 0x8
#define STATUS_REG_OFF 0x100

_start:

  /* clear smn slots */
  mov r0, #0x0
  ldr r1, =SMN_SLOT_BASE
  ldr r2, =SMN_SLOT_COUNT
_clear_smn_slots:
  str r0, [r1], #4
  subs r2, r2, #2
  bne _clear_smn_slots

  /* map wrapped ikek */
  ldr r0, =SMN_FLASH_ADDR
  ldr r1, =IKEK_FLASH_ADDR
  add r0, r0, r1 // final smn address
  ldr r1, =0xfffff 
  and r1, r1, r0 // smn slot offset
  lsr r0, r0, #20 // smn address high part
  ldr r2, =SMN_SLOT_BASE
  str r0, [r2] // map to first smn slot
  ldr r3, =SMN_MAP_BASE
  add r2, r3, r1 // mapped ikek address

  /* test write */
  ldr r0, =0xdeadbeef
  str r0, [r3]
  
  /* Create descriptor */
  ldr r0, =DESC_SCRATCH
  ldr r1, =DW0
  str r1, [r0], #4

  ldr r1, =DW1
  str r1, [r0], #4

  //ldr r1, =DW2
  str r2, [r0], #4

  ldr r1, =DW3
  str r1, [r0], #4

  //ldr r1, =DW4
  ldr r1, =RESULT_SCRATCH
  str r1, [r0], #4

  ldr r1, =DW5
  str r1, [r0], #4

  ldr r1, =DW6
  str r1, [r0], #4

  ldr r1, =DW7
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
  mov r0, #0x10
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
