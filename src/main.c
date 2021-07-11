#include <stdio.h>
//#include "parser.h"
//#include "expr.h"
#include <ast.h>
#include <parser.h>

char buf[100];

int main() {
    printf("Write a declaration: ");
    fgets(buf, 100, stdin);

    ciwic_parser parser = ciwic_parser_new(buf, 100);
    ciwic_statement stmt;

    if (ciwic_parser_statement(&parser, &stmt)) {
        printf("Error: could not parse\n");
    } else {
        printf("Successfully parsed:\n");
        ciwic_print_statement(&stmt, 0);
        printf("\n");
    }

    return 0;
}
