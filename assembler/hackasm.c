// This is an assembler for the Hack computer.
/* TODO:
-Create symbol struct (name, value)
-Create SymbolTable struct (dynamic array of symbol structs)
-SymbolTable keeps count of unique variables found
-Convert variable to symbol by check if num after @
-Convert label by check 0th char if ( and get in between
*/

#define MAX_LINE_LEN 128
#define LINE_CHUNK 128
#define PROGRAM_BUF_CHUNK (MAX_LINE_LEN * LINE_CHUNK)
#define INSTR_BITS 16
#define SYMBOL_MAX_LEN 64
#define VAR_START_ADDR 16
#define MAX_SYMBOLS 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

// Represents a symbol as a name:value pair.
typedef struct Symbol
{
    char name[SYMBOL_MAX_LEN];
    char value[SYMBOL_MAX_LEN];
} Symbol;

// Sets the values of a symbol.
void symbol_set_value(Symbol *symbol, char *name, char *value)
{
    strncpy(symbol->name, name, SYMBOL_MAX_LEN);
    strncpy(symbol->value, value, SYMBOL_MAX_LEN);
}

/* Represents the list of symbols found in the program.
Although a hash table might make more sense, a simple array is much quicker
to implement and may even be faster since the size of the table should be
relatively small. */
typedef struct SymbolTable
{
    Symbol symbols[MAX_SYMBOLS];
    int count;
    int variables;
} SymbolTable;

// Adds a symbol to the table.
void symboltable_add(SymbolTable *table, Symbol symbol, bool is_var)
{
    table->symbols[table->count] = symbol;
    table->count++;

    if (is_var)
    {
        table->variables++;
    }
}

// Retrieves the value of a symbol if it exists.
void symboltable_get(SymbolTable *table, char *name, char *dest)
{
    for (int i = 0; i < table->count; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            strncpy(dest, table->symbols[i].value, SYMBOL_MAX_LEN);
            break;
        }
    }
}

// Initializes the symbol table.
void symboltable_init(SymbolTable *table)
{
    table->count = 0;
    table->variables = 0;
    Symbol symbol;

    // Add predefined symbols
    symbol_set_value(&symbol, "SP", "0");
    symboltable_add(table, symbol, false);

    symbol_set_value(&symbol, "LCL", "1");
    symboltable_add(table, symbol, false);

    symbol_set_value(&symbol, "ARG", "2");
    symboltable_add(table, symbol, false);

    symbol_set_value(&symbol, "THIS", "3");
    symboltable_add(table, symbol, false);

    symbol_set_value(&symbol, "THAT", "4");
    symboltable_add(table, symbol, false);

    // Add symbols R0-R15
    for (int i = 0; i < 16; i++)
    {
        char label[4] = "R";
        char num[3];
        sprintf(num, "%d", i);
        strncat(label, num, 3);

        symbol_set_value(&symbol, label, num);
        symboltable_add(table, symbol, false);
    }

    symbol_set_value(&symbol, "SCREEN", "16384");
    symboltable_add(table, symbol, false);

    symbol_set_value(&symbol, "KBD", "24576");
    symboltable_add(table, symbol, false);
}

// Contains information about the loaded program such as contents and size.
typedef struct Program
{
    char *binary;
    char *assembly;
    size_t size;
    int lines;
} Program;

// Initialize the program.
void program_init(Program *program)
{
    program->assembly = calloc(PROGRAM_BUF_CHUNK, sizeof(char));
    program->size = PROGRAM_BUF_CHUNK * sizeof(char);
    program->lines = 0;
}

// Add a line to the program.
void program_add_line(Program *program, char *line, size_t line_len)
{
    strncpy(program->assembly + (program->lines * MAX_LINE_LEN), line, line_len + 1);

    // Expand the program buffer every LINE_CHUNK number of lines.
    program->lines++;
    if (program->lines % LINE_CHUNK == 0)
    {
        program->size += PROGRAM_BUF_CHUNK;
        program->assembly = realloc(program->assembly, program->size);
    }
}

// Frees up memory before exiting.
void clean_exit(Program *program, int status)
{
    if (program->assembly != NULL)
    {
        free(program->assembly);
    }
    if (program->binary != NULL)
    {
        free(program->binary);
    }

    exit(status);
}

// Remove all whitespace from the line.
void trim_ws(char *str)
{
    char buf[MAX_LINE_LEN] = "";
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (!isspace(str[i]))
        {
            buf[strlen(buf)] = str[i];
        }
    }
    strncpy(str, buf, strlen(buf) + 1);
}

// Remove all comments from the line.
void trim_comments(char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (str[i] == '/' && str[i + 1] == '/')
        {
            str[i] = '\0';
            break;
        }
    }
}

/* Performs the first pass which includes removing whitespace and comments,
and building the symbol table. */
bool first_pass(char *filename, Program *program)
{
    // Open and read .asm file line-by-line
    char line[MAX_LINE_LEN];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Unable to open assembly file.\n");
        return false;
    }

    while (fgets(line, MAX_LINE_LEN, fp) != NULL)
    {
        // Strip whitespace and comments from line
        trim_ws(line);
        trim_comments(line);

        // Disregard blank lines
        size_t line_len = strlen(line);
        if (line_len > 0)
        {
            // Check for symbol in line, add to symbol table
            // If label symbol, value is next line number.
            // If variable symbol, value is 16 + variables found so far

            // Add line to buffer.
            program_add_line(program, line, line_len);
        }
    }

    fclose(fp);

    // Create space to store the binary conversion of program
    program->binary = calloc(program->size, sizeof(char));

    return true;
}

/* Performs the second pass which includes converting symbols into numbers and
instructions into binary. */
void second_pass(Program *program)
{
    for (int i = 0; i < program->lines; i++)
    {
        // Replace each symbol in line with value in table
        // TODO

        // Get the line of assembly code
        char line[MAX_LINE_LEN];
        strncpy(line, program->assembly + (i * MAX_LINE_LEN), MAX_LINE_LEN);

        char instruction[INSTR_BITS + 2] = "0000000000000000\n";

        // Handle A instruction (which all begin with '@')
        if (line[0] == '@')
        {
            uint16_t addr = atoi(line + 1);
            for (int b = 0; b < (INSTR_BITS - 1); b++)
            {
                /* Get the least significant bit of address, convert it to
                ASCII, and store it in respective instruction bit. */
                instruction[(INSTR_BITS - 1) - b] = ((addr >> b) & 0x0001) + '0';
            }
        }

        // Handle C instruction
        else
        {
            // Set first 3 bits to 111
            memcpy(instruction, "111", 3);

            // Store the three parts of a Hack assembly instruction
            char dest[4] = "";
            char comp[4] = "";
            char jump[4] = "";

            char *dest_pntr = strchr(line, '=');
            char *jump_pntr = strchr(line, ';');

            int comp_bits = 7;
            int dest_bits = 3;
            int jump_bits = 3;

            char *comp_start = instruction + 3;
            char *dest_start = instruction + 3 + comp_bits;
            char *jump_start = instruction + 3 + comp_bits + dest_bits;

            // Get the destination
            if (dest_pntr != NULL)
            {
                strncpy(dest, line, dest_pntr - line);
            }

            // Get the jump
            if (jump_pntr != NULL)
            {
                strncpy(jump, jump_pntr + 1, 3);
            }

            // Get the comp
            if (dest_pntr == NULL && jump_pntr == NULL)
            {
                strncpy(comp, line, strlen(line));
            }
            else if (dest_pntr == NULL)
            {
                strncpy(comp, line, jump_pntr - line);
            }
            else
            {
                strcpy(comp, dest_pntr + 1);
            }

            // Convert comp into binary
            if (strcmp(comp, "0") == 0)
            {
                memcpy(comp_start, "0101010", comp_bits);
            }
            else if (strcmp(comp, "1") == 0)
            {
                memcpy(comp_start, "0111111", comp_bits);
            }
            else if (strcmp(comp, "-1") == 0)
            {
                memcpy(comp_start, "0111010", comp_bits);
            }
            else if (strcmp(comp, "D") == 0)
            {
                memcpy(comp_start, "0001100", comp_bits);
            }
            else if (strcmp(comp, "A") == 0)
            {
                memcpy(comp_start, "0110000", comp_bits);
            }
            else if (strcmp(comp, "!D") == 0)
            {
                memcpy(comp_start, "0001101", comp_bits);
            }
            else if (strcmp(comp, "!A") == 0)
            {
                memcpy(comp_start, "0110011", comp_bits);
            }
            else if (strcmp(comp, "-D") == 0)
            {
                memcpy(comp_start, "0001111", comp_bits);
            }
            else if (strcmp(comp, "-A") == 0)
            {
                memcpy(comp_start, "0110011", comp_bits);
            }
            else if (strcmp(comp, "D+1") == 0)
            {
                memcpy(comp_start, "0011111", comp_bits);
            }
            else if (strcmp(comp, "A+1") == 0)
            {
                memcpy(comp_start, "0110111", comp_bits);
            }
            else if (strcmp(comp, "D-1") == 0)
            {
                memcpy(comp_start, "0001110", comp_bits);
            }
            else if (strcmp(comp, "A-1") == 0)
            {
                memcpy(comp_start, "0110010", comp_bits);
            }
            else if (strcmp(comp, "D+A") == 0)
            {
                memcpy(comp_start, "0000010", comp_bits);
            }
            else if (strcmp(comp, "D-A") == 0)
            {
                memcpy(comp_start, "0010011", comp_bits);
            }
            else if (strcmp(comp, "A-D") == 0)
            {
                memcpy(comp_start, "0000111", comp_bits);
            }
            else if (strcmp(comp, "D&A") == 0)
            {
                memcpy(comp_start, "0000000", comp_bits);
            }
            else if (strcmp(comp, "D|A") == 0)
            {
                memcpy(comp_start, "0010101", comp_bits);
            }
            else if (strcmp(comp, "M") == 0)
            {
                memcpy(comp_start, "1110000", comp_bits);
            }
            else if (strcmp(comp, "!M") == 0)
            {
                memcpy(comp_start, "1110001", comp_bits);
            }
            else if (strcmp(comp, "-M") == 0)
            {
                memcpy(comp_start, "1110011", comp_bits);
            }
            else if (strcmp(comp, "M+1") == 0)
            {
                memcpy(comp_start, "1110111", comp_bits);
            }
            else if (strcmp(comp, "M-1") == 0)
            {
                memcpy(comp_start, "1110010", comp_bits);
            }
            else if (strcmp(comp, "D+M") == 0)
            {
                memcpy(comp_start, "1000010", comp_bits);
            }
            else if (strcmp(comp, "D-M") == 0)
            {
                memcpy(comp_start, "1010011", comp_bits);
            }
            else if (strcmp(comp, "M-D") == 0)
            {
                memcpy(comp_start, "1000111", comp_bits);
            }
            else if (strcmp(comp, "D&M") == 0)
            {
                memcpy(comp_start, "1000000", comp_bits);
            }
            else if (strcmp(comp, "D|M") == 0)
            {
                memcpy(comp_start, "1010101", comp_bits);
            }
            else
            {
                fprintf(stderr, "Invalid compute on line %d.\n", i);
                clean_exit(program, 1);
            }

            // Convert dest into binary
            if (strcmp(dest, "M") == 0)
            {
                memcpy(dest_start, "001", dest_bits);
            }
            else if (strcmp(dest, "D") == 0)
            {
                memcpy(dest_start, "010", dest_bits);
            }
            else if (strcmp(dest, "MD") == 0)
            {
                memcpy(dest_start, "011", dest_bits);
            }
            else if (strcmp(dest, "A") == 0)
            {
                memcpy(dest_start, "100", dest_bits);
            }
            else if (strcmp(dest, "AM") == 0)
            {
                memcpy(dest_start, "101", dest_bits);
            }
            else if (strcmp(dest, "AD") == 0)
            {
                memcpy(dest_start, "110", dest_bits);
            }
            else if (strcmp(dest, "AMD") == 0)
            {
                memcpy(dest_start, "111", dest_bits);
            }
            else if (strcmp(dest, "") == 0)
            {
                memcpy(dest_start, "000", dest_bits);
            }
            else
            {
                fprintf(stderr, "Invalid destination on line %d.\n", i);
                clean_exit(program, 1);
            }

            // Convert jump into binary
            if (strcmp(jump, "JGT") == 0)
            {
                memcpy(jump_start, "001", jump_bits);
            }
            else if (strcmp(jump, "JEQ") == 0)
            {
                memcpy(jump_start, "010", jump_bits);
            }
            else if (strcmp(jump, "JGE") == 0)
            {
                memcpy(jump_start, "011", jump_bits);
            }
            else if (strcmp(jump, "JLT") == 0)
            {
                memcpy(jump_start, "100", jump_bits);
            }
            else if (strcmp(jump, "JNE") == 0)
            {
                memcpy(jump_start, "101", jump_bits);
            }
            else if (strcmp(jump, "JLE") == 0)
            {
                memcpy(jump_start, "110", jump_bits);
            }
            else if (strcmp(jump, "JMP") == 0)
            {
                memcpy(jump_start, "111", jump_bits);
            }
            else if (strcmp(jump, "") == 0)
            {
                memcpy(jump_start, "000", jump_bits);
            }
            else
            {
                fprintf(stderr, "Invalid jump on line %d.\n", i);
                clean_exit(program, 1);
            }
        }

        // Add the binary instruction to the overall binary program.
        strncat(program->binary, instruction, strlen(instruction) + 1);
    }
}

/* Generates a .hack file containing the raw binary of converted assembly
program. */
bool gen_hack(char *filename, Program *program)
{
    FILE *fp = fopen(filename, "w");
    if (fp != NULL)
    {
        fwrite(program->binary, strlen(program->binary), 1, fp);
    }
    else
    {
        fprintf(stderr, "Unable to generate %s\n", filename);
        return false;
    }

    fclose(fp);
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./hackasm <path-to-file>\n");
        return 1;
    }

    // Initialize container which holds the contents of the assembly file.
    Program program;
    program_init(&program);

    // Initialize symbol table
    SymbolTable symtbl;
    symboltable_init(&symtbl);

    // Perform first pass
    if (!first_pass(argv[1], &program))
    {
        clean_exit(&program, 1);
    }

    // Perform second pass
    second_pass(&program);

    // Write binary to .hack file
    if (!gen_hack("out.hack", &program))
    {
        clean_exit(&program, 1);
    }

    clean_exit(&program, 0);
}