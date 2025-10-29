#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// 🔹 تعریف متغیرها
typedef struct {
    const char *name;
    double value;
} Var;

static Var vars[] = {
    {"!pi", 3.141592653589793},
    {"!e",  2.718281828459045},
    {"!test", 1.0},
    {NULL, 0}
};

// 🔹 گرفتن مقدار متغیر (مثل !pi یا !test)
double get_var_value(const char *token) {
    for (int i = 0; vars[i].name != NULL; i++) {
        if (strcmp(vars[i].name, token) == 0)
            return vars[i].value;
    }
    fprintf(stderr, "Unknown variable: %s\n", token);
    exit(1);
}

// 🔹 حذف فاصله‌ها
void remove_spaces(char *str) {
    char *dst = str;
    while (*str) {
        if (!isspace((unsigned char)*str))
            *dst++ = *str;
        str++;
    }
    *dst = '\0';
}

// 🔹 گرفتن عدد یا متغیر
const char *parse_number_or_var(const char *expr, double *out) {
    char buf[64];
    int i = 0;

    // ✅ متغیر
    if (*expr == '!') {
        buf[i++] = *expr++;  // ! رو بریز تو بافر
        while (*expr && (isalpha((unsigned char)*expr) || *expr == '_' || isdigit((unsigned char)*expr)))
            buf[i++] = *expr++;
        buf[i] = '\0';
        *out = get_var_value(buf);
        return expr;
    }

    // ✅ عدد
    while (*expr && (isdigit((unsigned char)*expr) || *expr == '.'))
        buf[i++] = *expr++;
    buf[i] = '\0';
    *out = atof(buf);
    return expr;
}

// 🔹 ارزیابی با تقدم عملگر
const char *parse_factor(const char *expr, double *out);
const char *parse_term(const char *expr, double *out);
const char *parse_expr(const char *expr, double *out);

const char *parse_factor(const char *expr, double *out) {
    if (*expr == '(') {
        expr = parse_expr(expr + 1, out);
        if (*expr != ')') {
            fprintf(stderr, "Expected ')'\n");
            exit(1);
        }
        return expr + 1;
    }
    return parse_number_or_var(expr, out);
}

const char *parse_term(const char *expr, double *out) {
    expr = parse_factor(expr, out);
    while (*expr == '*' || *expr == '/') {
        char op = *expr++;
        double rhs;
        expr = parse_factor(expr, &rhs);
        if (op == '*') *out *= rhs;
        else {
            if (rhs == 0) {
                fprintf(stderr, "Division by zero!\n");
                exit(1);
            }
            *out /= rhs;
        }
    }
    return expr;
}

const char *parse_expr(const char *expr, double *out) {
    expr = parse_term(expr, out);
    while (*expr == '+' || *expr == '-') {
        char op = *expr++;
        double rhs;
        expr = parse_term(expr, &rhs);
        if (op == '+') *out += rhs;
        else *out -= rhs;
    }
    return expr;
}

// 🔹 main
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: math \"expression\"\n");
        return 1;
    }

    char expr[256];
    strncpy(expr, argv[1], sizeof(expr) - 1);
    expr[sizeof(expr) - 1] = '\0';
    remove_spaces(expr);

    double result;
    parse_expr(expr, &result);

    printf("= %.15f\n", result); // 👈 دقت کامل double
    return 0;
}
