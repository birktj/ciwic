#include <stdio.h>
//#include "parser.h"
//#include "expr.h"
#include <ast.h>
#include <parser.h>

char buf[100];

int main() {
    printf("Write an expression: ");
    fgets(buf, 100, stdin);

    ciwic_parser parser = ciwic_parser_new(buf, 100);
    ciwic_expr expr;
    if (ciwic_parser_expr(&parser, &expr)) {
        printf("Error: could not parse expression\n");
    } else {
        printf("Successfully parsed expression:\n");
        ciwic_print_expr(&expr, 0);
        printf("\n");
    }

    return 0;
}
