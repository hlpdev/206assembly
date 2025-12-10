> [!NOTE]
> Prebuilt Windows, macOS, and Linux binaries are available in the **Releases** section of this repository.

# ASM206 - 8-bit Assembly Language + Emulator

ASM206 is a small 8-bit assembly language and emulator implemented in C.
This repository contains both the assembler (`assembler.c`) and
the emulator (`emulator.c`) as independent tools.

## CPU Overview

### Registers

| Register | Type         |
| -------- | ------------ |
| A        | 8-bit signed |
| B        | 8-bit signed |
| C        | 8-bit signed |
| D        | 8-bit signed |

## Instruction Set

### Format

| Instruction  | Opcode Pattern | Description                           |
| ------------ | -------------- | ------------------------------------- |
| `LDI R, IMM` | `1100iiii`     | Load immediate (0-15) into a register |
| `ADD RD, RS` | `0111ddss`     | RD = RD + RS                          |
| `SUB RD, RS` | `0001ddss`     | RD = RD - RS                          |
| `SKIPNZ R`   | `0100rr00`     | Skip next instruction if R != 0       |
| `JMP addr`   | `10aaaaaa`     | PC = addr (0-63)                      |
| `HALT`       | `00000001`     | Stops execution                       |

- `dd` = destination register
- `ss` = source register
- `rr` = register for skipnz
- `iiii` = immediate (0-15)
- `aaaaaa` = absolute address

> Immediate values range from 0-15 (4-bit)

> Jump targets range from 0-63 (6-bit)

## Syntax

### File Names

- `*.asm206` ASM206 source file
- `*.bin206` Assembled ASM206 binary

### Comments

```
; My cool comment
// My other cool comment
```

### Labels

```
my_label:
    ldi A, 6

my_other_label:
    jmp my_label
```

## Usage

### Assembling

```
assembler <input.asm206> <output.bin206>
```

### Emulating

```
emulator <input.bin206>
```

## Example

```asm
; The following program sets registers A & D to
; 1 & 4. It then continues subtracting the value
; in register D by the value in register A until
; the value in register A equals zero.

start:
    ldi A, 1
    ldi D, 4

loop:
    sub D, A
    skipnz D
    jmp done
    jmp loop

done:
    halt
```

## VScode

This repository includes a local vscode extension which
provides syntax highlighting for `*.asm206` source files.
