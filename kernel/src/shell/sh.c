#include "sh.h"
#include "../printk.h"
#include "../port.h"
#include <stdint.h>
#include <stdbool.h>

// ------------------- KEYBOARD -------------------

#define KBD_DATA 0x60
#define KBD_STATUS 0x64

bool shift = false;

char keymap[128] = {
    0, 27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
};

char keymap_shift[128] = {
    0, 27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
};

char kbd_getchar() {
    while (!(inb(KBD_STATUS) & 1));

    uint8_t sc = inb(KBD_DATA);

    // release
    if (sc & 0x80) {
        sc &= 0x7F;
        if (sc == 42 || sc == 54) shift = false;
        return 0;
    }

    // press
    if (sc == 42 || sc == 54) {
        shift = true;
        return 0;
    }

    if (sc >= 128) return 0;

    return shift ? keymap_shift[sc] : keymap[sc];
}

// ------------------- SHELL CORE -------------------

char buffer[SHELL_BUFFER_SIZE];
int index = 0;

void shell_getline() {
    index = 0;

    while (1) {
        char c = kbd_getchar();
        if (!c) continue;

        if (c == '\n') {
            printk("\n");
            buffer[index] = 0;
            return;
        }

        if (c == '\b') {
            if (index > 0) {
                index--;
                printk("\b \b");
            }
            continue;
        }

        if (index < SHELL_BUFFER_SIZE - 1) {
            buffer[index++] = c;
            printk("%c", c);
        }
    }
}

// ------------------- PARSER -------------------

int parse(char* input, char** argv) {
    int argc = 0;

    while (*input && argc < SHELL_MAX_ARGS) {

        while (*input == ' ') input++;
        if (!*input) break;

        argv[argc++] = input;

        while (*input && *input != ' ') input++;

        if (*input) {
            *input = 0;
            input++;
        }
    }

    return argc;
}

// ------------------- COMMANDS -------------------

void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s ", argv[i]);
    }
    printk("\n");
}

void cmd_clear() {
    kclear(0x000000);
}

void cmd_exit() {
    printk("\n[LKSH] Exited.\n");
    printk("[LKSH] System halted!\n");
    kabort();
}

void cmd_help() {
    printk("commands:\n");
    printk("  help - show this help\n");
    printk("  clear - clear screen\n");
    printk("  echo - echo an text\n");
    printk("  exit - exit and halt system\n");
}


// ------------------- EXEC -------------------

void execute(int argc, char** argv) {
    if (argc == 0) return;

    if (!argv[0]) return;

    if (!__builtin_strcmp(argv[0], "help")) cmd_help();
    else if (!__builtin_strcmp(argv[0], "clear")) cmd_clear();
    else if (!__builtin_strcmp(argv[0], "echo")) cmd_echo(argc, argv);
    else if (!__builtin_strcmp(argv[0], "exit")) cmd_exit();
    else printk("unknown: %s\n", argv[0]);
}

// ------------------- MAIN LOOP -------------------

void kernel_shell_run(void) {
    char* argv[SHELL_MAX_ARGS];

    printk("Welcome to LKSH! Lavaos Kernel SHell.\n");

    while (1) {
        printk("> ");
        shell_getline();
        int argc = parse(buffer, argv);
        execute(argc, argv);
    }
}
