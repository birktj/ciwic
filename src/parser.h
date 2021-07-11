#pragma once 

#include <parselib.h>
#include <ast.h>

extern const char* ciwic_prim_types_keywords[12];
extern const ciwic_type_prim ciwic_prim_types_list[12];

ciwic_parser ciwic_parser_new(char *buf, int len);
int ciwic_parser_char(ciwic_parser *parser, char *res);
int ciwic_parser_int(ciwic_parser *parser, int *res);
int ciwic_parser_expr_arg_list(ciwic_parser *parser, ciwic_expr_arg_list *res);
int ciwic_parser_assignment_expr(ciwic_parser *parser, ciwic_expr *res);
int ciwic_parser_expr(ciwic_parser *parser, ciwic_expr *res);

int ciwic_parser_declaration(ciwic_parser *parser, ciwic_declaration *decl);
int ciwic_parser_declaration_specifiers(ciwic_parser *parser, ciwic_declaration_specifiers *specifiers);
int ciwic_parser_declarator(ciwic_parser *parser, ciwic_declarator *prev, ciwic_declarator *decl);

int ciwic_parser_initializer_list(ciwic_parser *parser, ciwic_initializer_list *list);
int ciwic_parser_type_name(ciwic_parser *parser, ciwic_type_name *name);

int ciwic_parser_statement(ciwic_parser *parser, ciwic_statement *name);

