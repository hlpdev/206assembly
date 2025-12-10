#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char name[64];
    int address;
} Label;

typedef struct
{
    Label items[256];
    int count;
} LabelTable;

typedef struct
{
    char lines[1024][256];
    int count;
} Source;

static void to_upper_string(char* str)
{
    while (*str)
    {
        *str = (char)toupper(*str);
        str++;
    }
}

static char* trim_left(char* text)
{
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
    {
        text++;
    }

    return text;
}

static void label_add(LabelTable* table, const char* name, int address)
{
    sscanf(name, "%63s", table->items[table->count].name);

    table->items[table->count].address = address;
    table->count++;
}

static int label_find(LabelTable* table, const char* name)
{
    for (int i = 0; i < table->count; i++)
    {
        if (strcmp(table->items[i].name, name) == 0)
        {
            return table->items[i].address;
        }
    }

    fprintf(stderr, "Unknown label: %s\n", name);
    exit(1);
}

static uint8_t reg_encode(const char* name)
{
    if (strcmp(name, "A") == 0)
    {
        return 0;
    }

    if (strcmp(name, "B") == 0)
    {
        return 1;
    }

    if (strcmp(name, "C") == 0)
    {
        return 2;
    }

    if (strcmp(name, "D") == 0)
    {
        return 3;
    }

    fprintf(stderr, "Unknown register %s\n", name);
    exit(1);
}

static uint8_t op_ldi(const char* rd, int imm)
{
    if (imm < 0 || imm > 15)
    {
        fprintf(stderr, "LDI immediate out of range: %d\n", imm);
        exit(1);
    }

    uint8_t r = reg_encode(rd);
    uint8_t hi = (uint8_t)((imm >> 2) & 0x3);
    uint8_t lo = (uint8_t)(imm & 0x3);

    return (uint8_t)(0b11000000 | (hi << 4) | (r << 2) | lo);
}

static uint8_t op_add(const char* a, const char* b)
{
    return (uint8_t)(0b01110000 | (reg_encode(a) << 2) | reg_encode(b));
}

static uint8_t op_sub(const char* a, const char* b)
{
    return (uint8_t)(0b00010000 | (reg_encode(a) << 2) | reg_encode(b));
}

static uint8_t op_skipnz(const char* a) { return (uint8_t)(0b01000000 | (reg_encode(a) << 2)); }

static uint8_t op_jmp(int addr)
{
    if (addr < 0 || addr > 63)
    {
        fprintf(stderr, "JMP address out of range: %d\n", addr);
        exit(1);
    }

    return (uint8_t)(0b10000000 | addr);
}

static uint8_t op_halt(void) { return 0x01; }

static void load_source(Source* src, const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, "Cannot open %s\n", path);
        exit(1);
    }

    while (fgets(src->lines[src->count], 256, f))
    {
        src->count++;
    }

    fclose(f);
}

static void first_pass(Source* src, LabelTable* table)
{
    int pc = 0;

    for (int line = 0; line < src->count; ++line)
    {
        char buf[256] = {0};
        strncpy(buf, src->lines[line], sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        char* p = trim_left(buf);

        char* comment = strchr(p, ';');
        if (comment)
        {
            *comment = '\0';
        }

        char* slash = strstr(p, "//");
        if (slash)
        {
            *slash = '\0';
        }

        to_upper_string(p);

        if (*p == '\0')
        {
            continue;
        }

        char* colon = strchr(p, ':');
        if (colon != NULL)
        {
            *colon = '\0';

            char label_name[64] = {0};
            sscanf(p, "%63s", label_name);
            label_add(table, label_name, pc);

            continue;
        }

        pc++;
    }
}

static void second_pass(Source* src, LabelTable* table, const char* outpath)
{
    FILE* out = fopen(outpath, "wb");
    if (!out)
    {
        fprintf(stderr, "Cannot write %s\n", outpath);
        exit(1);
    }

    for (int line = 0; line < src->count; ++line)
    {
        char* p = trim_left(src->lines[line]);

        to_upper_string(p);

        char* comment = strchr(p, ';');
        if (comment) *comment = '\0';

        char* slash = strstr(p, "//");
        if (slash)
        {
            *slash = '\0';
        }

        if (*p == '\0' || strchr(p, ':'))
        {
            continue;
        }

        for (char* q = p; *q; q++)
        {
            if (*q == ',')
            {
                *q = ' ';
            }
        }

        char op[32];
        char a[32];
        char b[32];
        char temp[32];
        int value;

        if (sscanf(p, "%31s", op) < 1)
        {
            continue;
        }

        if (strcmp(op, "LDI") == 0)
        {
            sscanf(p, "%*s %31s %d", a, &value);
            uint8_t byte = op_ldi(a, value);
            fwrite(&byte, 1, 1, out);
            continue;
        }

        if (strcmp(op, "ADD") == 0)
        {
            sscanf(p, "%*s %31s %31s", a, b);
            uint8_t byte = op_add(a, b);
            fwrite(&byte, 1, 1, out);
            continue;
        }

        if (strcmp(op, "SUB") == 0)
        {
            sscanf(p, "%*s %31s %31s", a, b);
            uint8_t byte = op_sub(a, b);
            fwrite(&byte, 1, 1, out);
            continue;
        }

        if (strcmp(op, "SKIPNZ") == 0)
        {
            sscanf(p, "%*s %31s", a);
            uint8_t byte = op_skipnz(a);
            fwrite(&byte, 1, 1, out);
            continue;
        }

        if (strcmp(op, "JMP") == 0)
        {
            sscanf(p, "%*s %31s", temp);

            if (isdigit((unsigned char)temp[0]))
            {
                value = atoi(temp);
            }
            else
            {
                value = label_find(table, temp);
            }

            uint8_t byte = op_jmp(value);
            fwrite(&byte, 1, 1, out);
            continue;
        }

        if (strcmp(op, "HALT") == 0)
        {
            uint8_t byte = op_halt();
            fwrite(&byte, 1, 1, out);
            continue;
        }

        fprintf(stderr, "Unknown opcode: %s\n", op);
        exit(1);
    }

    fclose(out);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: assembler <input.asm206> <output.bin206>\n");
        return 1;
    }

    Source source = {0};
    LabelTable labels = {0};

    load_source(&source, argv[1]);
    first_pass(&source, &labels);
    second_pass(&source, &labels, argv[2]);

    return 0;
}
