#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Cross-platform secure wrappers
#if defined(_WIN32) || defined(_WIN64)
#include <stdio.h>
#else
#define fopen_s(fp, filename, mode) ((*(fp) = fopen((filename), (mode))) == NULL)
#define sscanf_s sscanf
#define fprintf_s fprintf
#define strncpy_s(dest, destsz, src, count) strncpy((dest), (src), (destsz))
#define _TRUNCATE 0  // ignored on non-Windows
#endif

#define TRUE 1
#define FALSE 0
typedef uint8_t bool;

enum
{
    OPCODE_ADD = 0b01110000,
    OPCODE_SUB = 0b00010000,
    OPCODE_SKIPNZ = 0b01000000,
    OPCODE_JMP = 0b10000000,
    OPCODE_LDI = 0b11000000,
    OPCODE_HALT = 0b00000001
};

typedef enum
{
    REG_A = 0,
    REG_B = 1,
    REG_C = 2,
    REG_D = 3
} Register;

struct CPU
{
    int8_t A;
    int8_t B;
    int8_t C;
    int8_t D;
    size_t PC;
    bool halted;
};

struct Memory
{
    uint8_t* buffer;
    size_t length;
};

// Gets RD from an instruction
static inline Register get_rd(uint8_t op) { return (Register)((op >> 2) & 0x03); }

// Gets RS from an instruction
static inline Register get_rs(uint8_t op) { return (Register)(op & 0x03); }

// Returns a pointer to the specified register
static inline int8_t* register_ptr(struct CPU* cpu, Register reg)
{
    switch (reg)
    {
        case REG_A:
            return &cpu->A;
        case REG_B:
            return &cpu->B;
        case REG_C:
            return &cpu->C;
        case REG_D:
            return &cpu->D;
    }

    return NULL;
}

static void execute_add(struct CPU* cpu, uint8_t op)
{
    *register_ptr(cpu, get_rd(op)) += *register_ptr(cpu, get_rs(op));
}

static void execute_sub(struct CPU* cpu, uint8_t op)
{
    *register_ptr(cpu, get_rd(op)) -= *register_ptr(cpu, get_rs(op));
}

static void execute_skipnz(struct CPU* cpu, uint8_t op)
{
    if (*register_ptr(cpu, get_rd(op)) != 0)
    {
        cpu->PC++;
    }
}

static void execute_jmp(struct CPU* cpu, uint8_t op)
{
    uint8_t addr = op & 0b00111111;
    cpu->PC = addr;
}

static void execute_ldi(struct CPU* cpu, uint8_t op)
{
    Register rd = (Register)((op >> 2) & 0x3);

    uint8_t imm_hi = (op >> 4) & 0x3;
    uint8_t imm_lo = op & 0x3;
    uint8_t imm = (imm_hi << 2) | imm_lo;

    *register_ptr(cpu, rd) = (int8_t)imm;
}

void run_program(struct CPU* cpu, struct Memory* mem)
{
    while (!cpu->halted && cpu->PC < mem->length)
    {
        // Get the next instruction
        uint8_t op = mem->buffer[cpu->PC++];

        // If it's HALT, then exit.
        if (op == OPCODE_HALT)
        {
            cpu->halted = TRUE;
            break;
        }

        // Get the first 2 & 4 bits of the instruction via masking
        uint8_t top2 = op & 0b11000000;
        uint8_t top4 = op & 0b11110000;

        if (top2 == OPCODE_JMP)
        {
            execute_jmp(cpu, op);
            continue;
        }

        if (top2 == OPCODE_LDI)
        {
            execute_ldi(cpu, op);
            continue;
        }

        switch (top4)
        {
            case OPCODE_ADD:
                execute_add(cpu, op);
                break;
            case OPCODE_SUB:
                execute_sub(cpu, op);
                break;
            case OPCODE_SKIPNZ:
                execute_skipnz(cpu, op);
                break;
            default:
                fprintf_s(stderr, "Unknown opcode %02X at PC=%zu\n", op, cpu->PC - 1);
                cpu->halted = TRUE;
                return;
        }
    }
}

// Loads the input file into the Memory struct
struct Memory load_file(const char* path)
{
    FILE* f = NULL;
    fopen_s(&f, path, "rb");

    if (!f)
    {
        fprintf_s(stderr, "Could not open file %s\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);

    struct Memory mem;
    mem.length = len;
    mem.buffer = malloc(len);
    if (!mem.buffer)
    {
        fprintf_s(stderr, "Out of memory!\n");
        fclose(f);
        exit(1);
    }

    if (fread(mem.buffer, 1, len, f) != len)
    {
        fprintf_s(stderr, "File read error!\n");
        fclose(f);
        exit(1);
    }
    fclose(f);

    return mem;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: asm206 my_program.bin206\n");
        return 1;
    }

    struct Memory mem = load_file(argv[1]);

    struct CPU cpu;
    cpu.A = 0;
    cpu.B = 0;
    cpu.C = 0;
    cpu.D = 0;
    cpu.PC = 0;
    cpu.halted = FALSE;

    run_program(&cpu, &mem);

    free(mem.buffer);

    printf("A = %d\tB = %d\tC = %d\tD = %d\tPC = %zu\n", cpu.A, cpu.B, cpu.C, cpu.D, cpu.PC);
    return 0;
}