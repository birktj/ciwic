#pragma once 

#include <parselib.h>
#include <ast.h>

ciwic_parser ciwic_parser_new(char *buf, int len);
int ciwic_parser_char(ciwic_parser *parser, char *res);
int ciwic_parser_int(ciwic_parser *parser, int *res);
int ciwic_parser_expr_arg_list(ciwic_parser *parser, ciwic_expr_arg_list *res);
int ciwic_parser_assignment_expr(ciwic_parser *parser, ciwic_expr *res);
int ciwic_parser_expr(ciwic_parser *parser, ciwic_expr *res);
int ciwic_parser_initializer_list(ciwic_parser *parser, ciwic_initializer_list *list);
int ciwic_parser_type_name(ciwic_parser *parser, ciwic_type_name *name);
