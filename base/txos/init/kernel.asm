;++
;
; Free95 20x Assembly
;
; You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
; If you do not agree to the terms, do not use the code.
;
;
; Module Name:
;
;    kernel.asm
;
; Abstract:
;
;    This module implements the assembly code necessary to jump
;    to C for 20x/TX.
;
;--

[BITS 32]

global _start
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; Enable the A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Remap the master PIC
    mov al, 00010001b
    out 0x20, al ; Tell master PIC
    out 0xA0, al ; Tell slave PIC

    mov al, 0x20 ; Interrupt 0x20 is where master ISR should start
    out 0x21, al
    mov al, 0x28 ; Slave ISR vector offset
    out 0xA1, al
    mov al, 00000100b ; Tell master PIC that there is a slave at IRQ2 (00000100b = 4)
    out 0x21, al
    mov al, 00000010b ; Tell slave PIC its cascade identity (00000010b = 2)
    out 0xA1, al

    mov al, 00000001b
    out 0x21, al
    out 0xA1, al
    ; End remap

    call kernel_main

    jmp $

times 512-($ - $$) db 0
