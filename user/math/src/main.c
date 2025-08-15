#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: math number1 [+, -, *, /] number2\n");
        return 1;
    }

    long a = strtol(argv[1], NULL, 10);
    char *op_str = argv[2];
    long b = strtol(argv[3], NULL, 10);
    long result;

    if (op_str[1] != '\0') {
        printf("Operator must be a single character: +, -, *, /\n");
        return 1;
    }

    char op = op_str[0];

    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b == 0) {
                printf("Error: Division by zero!\n");
                return 1;
            }
            result = a / b; 
            break;
        default:
            printf("Unknown operator: %c\n", op);
            return 1;
    }

    printf("%ld\n", result);
    return 0;
}
