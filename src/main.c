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
    ciwic_declaration decl;
    //if (ciwic_parser_expr(&parser, &expr)) {
    if (ciwic_parser_declaration(&parser, &decl)) {
        printf("Error: could not parse\n");
    } else {
        printf("Successfully parsed:\n");
        ciwic_print_declaration(&decl, 0);
        printf("\n");
    }

    return 0;
}
