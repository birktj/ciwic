#include <ast.h>
#include <parser.h>
#include <stdio.h>

const char* ciwic_expr_unary_op_table[] = {
    "post dec", "post inc", "pre dec", "pre inc", "ref", "deref", "pos", "neg",
    "bit inv", "log inv" };

const char* ciwic_expr_binary_op_table[] = { "mul", "div", "mod", "add",
    "sub", "shift right", "shift left", "less than", "greater than",
    "less or eq", "greater or eq", "eq", "neq", "and", "xor", "or", "land",
    "lor", "comma" };

int ciwic_declarator_is_abstract(ciwic_declarator *declarator) {
    if (declarator->type == ciwic_declarator_identifier) {
        return 0;
    }

    if (declarator->inner != NULL)
        return ciwic_declarator_is_abstract(declarator->inner);

    return 1;
}

void ciwic_print_constant(ciwic_constant *constant, int indent) {
    printf("%*cconstant:\n", indent, ' ');

    printf("%*ctype: ", indent+4, ' ');
    switch (constant->type) {
        case ciwic_constant_integer:
            printf("integer\n");
            break;
        case ciwic_constant_float:
            printf("float\n");
            break;
        case ciwic_constant_char:
            printf("char\n");
            break;
    }
    printf("%*ctext: %s\n", indent+4, ' ', constant->raw_text);
}

void ciwic_print_arg_list(ciwic_expr_arg_list *list, int indent) {
    printf("%*carg list:\n", indent, ' ');
    ciwic_print_expr(&list->head, indent+4);
    if (list->rest != NULL) {
        ciwic_print_arg_list(list->rest, indent+4);
    }
}

void ciwic_print_type_name(ciwic_type_name *name, int indent) {
    printf("%*ctype name: \n", indent, ' ');
    ciwic_print_declaration_specifiers(&name->specifiers, indent+4);
    if (name->declarator != NULL) 
        ciwic_print_declarator(name->declarator, indent+4);
}

void ciwic_print_designator_list(ciwic_designator_list *list, int indent) {
    printf("%*cdesignator list: ", indent, ' ');

    switch (list->type) {
        case ciwic_designator_expr:
            printf("expr\n");
            ciwic_print_expr(&list->expr, indent+4);
            break;
        case ciwic_designator_ident:
            printf("ident\n");
            printf("%*cidentifier: %.*s\n", indent+4, ' ', list->ident.len, list->ident.text);
            break;
    }

    if (list->rest != NULL) {
        ciwic_print_designator_list(list->rest, indent+4);
    }
}

void ciwic_print_initializer_list(ciwic_initializer_list *list, int indent) {
    printf("%*cinitializer list: \n", indent, ' ');

    if (list->designation != NULL) {
        ciwic_print_designator_list(list->designation, indent+4);
    }

    ciwic_print_initializer(list->initializer, indent+4);

    if (list->rest != NULL) {
        ciwic_print_initializer_list(list->rest, indent+4);
    }
}

void ciwic_print_expr(ciwic_expr *expr, int indent) {
    const char *op;
    switch (expr->type) {
        case ciwic_expr_type_identifier:
            printf("%*cidentifier: ", indent, ' ');
            printf("%.*s\n", expr->identifier.len, expr->identifier.text);
            break;
        case ciwic_expr_type_constant:
            //printf("%*cconstant:\n", indent, ' ');
            ciwic_print_constant(&expr->constant, indent);
            break;
        case ciwic_expr_type_unary_op:
            op = ciwic_expr_unary_op_table[expr->unary_op.op];
            printf("%*cunary_op: %s\n", indent, ' ', op);
            ciwic_print_expr(expr->unary_op.inner, indent+4);
            break;
        case ciwic_expr_type_binary_op:
            op = ciwic_expr_binary_op_table[expr->binary_op.op];
            printf("%*cbinary_op: %s\n", indent, ' ', op);
            ciwic_print_expr(expr->binary_op.fst, indent+4);
            ciwic_print_expr(expr->binary_op.snd, indent+4);
            break;
        case ciwic_expr_type_call:
            printf("%*ccall: \n", indent, ' ');
            ciwic_print_expr(expr->call.fun, indent+4);
            if (expr->call.args != NULL) {
                ciwic_print_arg_list(expr->call.args, indent+4);
            }
            break;
        case ciwic_expr_type_initialize:
            printf("%*cinitialize: \n", indent, ' ');
            // TODO: print initializer list
            break;
        case ciwic_expr_type_subscript:
            printf("%*csubscript: \n", indent, ' ');
            ciwic_print_expr(expr->subscript.val, indent+4);
            ciwic_print_expr(expr->subscript.pos, indent+4);
            break;
        case ciwic_expr_type_member:
            printf("%*cmember: ", indent, ' ');
            printf("%*s\n", expr->member.identifier.len, expr->member.identifier.text);
            ciwic_print_expr(expr->member.expr, indent+4);
            break;
        case ciwic_expr_type_member_deref:
            printf("%*cmember deref: ", indent, ' ');
            printf("%*s\n", expr->member.identifier.len, expr->member.identifier.text);
            ciwic_print_expr(expr->member.expr, indent+4);
            break;
        case ciwic_expr_type_sizeof_expr:
            printf("%*csizeof expr: \n", indent, ' ');
            ciwic_print_expr(expr->sizeof_expr, indent+4);
            break;
        case ciwic_expr_type_sizeof_type:
            printf("%*csizeof type: \n", indent, ' ');
            ciwic_print_type_name(&expr->sizeof_type, indent+4);
            break;
        case ciwic_expr_type_cast:
            printf("%*ccast: \n", indent, ' ');
            ciwic_print_type_name(&expr->cast.type_name, indent+4);
            ciwic_print_expr(expr->cast.expr, indent+4);
            break;
        case ciwic_expr_type_conditional:
            printf("%*cconditional: \n", indent, ' ');
            ciwic_print_expr(expr->conditional.cond, indent+4);
            ciwic_print_expr(expr->conditional.left, indent+4);
            ciwic_print_expr(expr->conditional.right, indent+4);
            break;
        case ciwic_expr_type_assignment:
            op = ciwic_expr_binary_op_table[expr->binary_op.op];
            printf("%*cassignment: %s\n", indent, ' ', op);
            ciwic_print_expr(expr->assignment.left, indent+4);
            ciwic_print_expr(expr->assignment.right, indent+4);
            break;
    }
}

void ciwic_print_type_qualifiers(int type_qualifiers, int indent) {
    if (type_qualifiers & ciwic_type_qualifier_const)
        printf("%*ctype qualifier: const\n", indent, ' ');

    if (type_qualifiers & ciwic_type_qualifier_restrict)
        printf("%*ctype qualifier: restrict\n", indent, ' ');

    if (type_qualifiers & ciwic_type_qualifier_volatile)
        printf("%*ctype qualifier: volatile\n", indent, ' ');
}

void ciwic_print_declaration_specifiers(ciwic_declaration_specifiers *specs, int indent) {
    printf("%*cdeclaration specifiers: \n", indent, ' ');
    
    if (specs->storage_class & ciwic_specifier_typedef)
        printf("%*cstorage class: typedef\n", indent+4, ' ');

    if (specs->storage_class & ciwic_specifier_extern)
        printf("%*cstorage class: extern\n", indent+4, ' ');

    if (specs->storage_class & ciwic_specifier_static)
        printf("%*cstorage class: static\n", indent+4, ' ');

    if (specs->storage_class & ciwic_specifier_auto)
        printf("%*cstorage class: auto\n", indent+4, ' ');

    if (specs->storage_class & ciwic_specifier_register)
        printf("%*cstorage class: register\n", indent+4, ' ');

    if (specs->func_specifiers & ciwic_function_specifier_inline)
        printf("%*cfunction specifier: inline\n", indent+4, ' ');

    ciwic_print_type_qualifiers(specs->type_qualifiers, indent+4);

    if (specs->type_spec == ciwic_type_spec_prim) {
        for (int i = 0; i < 12; i++) {
            if (specs->prim_type & ciwic_prim_types_list[i])
                printf("%*ctype specifier: %s\n", indent+4, ' ', ciwic_prim_types_keywords[i]);
        }
    }

    if (specs->type_spec == ciwic_type_spec_enum) {
        printf("%*ctype specifier: enum\n", indent+4, ' ');
        if (specs->enum_.identifier != NULL) {
            printf("%*cident: ", indent+8, ' ');
            printf("%.*s\n", specs->enum_.identifier->len, specs->enum_.identifier->text);
        }
        if (specs->enum_.decl != NULL) {
            int di = 8;
            ciwic_enum_list *decl = specs->enum_.decl;

            while (decl != NULL) {
                printf("%*cenum list: ", indent+di, ' ');
                printf("%.*s\n", decl->name.len, decl->name.text);
                if (decl->expr != NULL) {
                    ciwic_print_expr(decl->expr, indent+di+4);
                }
                di += 4;
                decl = decl->rest;
            }
        }
    }

    if (specs->type_spec == ciwic_type_spec_struct || specs->type_spec == ciwic_type_spec_union) {
        printf("%*ctype specifier: ", indent+4, ' ');
        if (specs->type_spec == ciwic_type_spec_struct)
            printf("struct\n");
        else
            printf("union\n");
        if (specs->struct_or_union.identifier != NULL) {
            printf("%*cident: ", indent+8, ' ');
            printf("%.*s\n", specs->struct_or_union.identifier->len, specs->struct_or_union.identifier->text);
        }
        if (specs->struct_or_union.decl != NULL) {
            int di = 8;
            ciwic_struct_list *decl = specs->struct_or_union.decl;

            while (decl != NULL) {
                printf("%*cstruct list: \n", indent+di, ' ');
                ciwic_print_declaration_specifiers(&decl->specifiers, indent+di+4);
                ciwic_struct_declarator_list *list = &decl->declarator_list;
                int dj = di+4;

                while (list != NULL) {
                    printf("%*cstruct declarator list: \n", indent+dj, ' ');
                    if (list->declarator != NULL)
                        ciwic_print_declarator(list->declarator, indent+dj+4);
                    if (list->expr != NULL)
                        ciwic_print_expr(list->expr, indent+dj+4);

                    dj += 4;
                    list = list->rest;
                }

                di += 4;
                decl = decl->rest;
            }
        }
    }

    if (specs->type_spec == ciwic_type_spec_typedef_name) {
        printf("%*ctypedef name: ", indent+4, ' ');
        printf("%.*s\n", specs->typedef_name.len, specs->typedef_name.text);
    }
}

void ciwic_print_declarator(ciwic_declarator *decl, int indent) {
    switch (decl->type) {
        case ciwic_declarator_pointer:
            printf("%*cdeclarator: pointer\n", indent, ' ');
            ciwic_print_type_qualifiers(decl->pointer_qualifiers, indent+4);
            break;
        case ciwic_declarator_identifier:
            printf("%*cdeclarator: identifier\n", indent, ' ');
            printf("%*cident: %.*s\n", indent+4, ' ', decl->ident.len, decl->ident.text);
            break;
        case ciwic_declarator_array:
            printf("%*cdeclarator: array\n", indent, ' ');
            if (decl->array.is_static) {
                printf("%*cstorage class: static\n", indent+4, ' ');
            }
            ciwic_print_type_qualifiers(decl->array.type_qualifiers, indent+4);
            if (decl->array.is_var_len) {
                printf("%*cvar len\n", indent+4, ' ');
            }
            if (decl->array.expr != NULL)
                ciwic_print_expr(decl->array.expr, indent+4);
            break;
        case ciwic_declarator_func:
            printf("%*cdeclarator: func\n", indent, ' ');
            ciwic_param_list *list = decl->func.param_list;
            int di = 4;

            while (list != NULL) {
                printf("%*cparam list: \n", indent+di, ' ');

                ciwic_print_declaration_specifiers(&list->specifiers, indent+di+4);

                if (list->declarator != NULL) {
                    ciwic_print_declarator(list->declarator, indent+di+4);
                }

                di += 4;
                list = list->rest;
            }

            if (decl->func.has_ellipsis) {
                printf("%*cellipsis\n", indent, ' ');
            }
            break;
    }
    
    if (decl->inner != NULL) {
        ciwic_print_declarator(decl->inner, indent+4);
    }
}

void ciwic_print_initializer(ciwic_initializer *init, int indent) {
    printf("%*cinitializer: ", indent, ' ');
    switch (init->type) {
        case ciwic_initializer_init_expr:
            printf("expr\n");
            ciwic_print_expr(&init->expr, indent + 4);
            break;
        case ciwic_initializer_init_list:
            printf("list\n");
            ciwic_print_initializer_list(&init->list, indent + 4);
            break;
    }
}

void ciwic_print_declaration(ciwic_declaration *decl, int indent) {
    printf("%*cdeclaration: \n", indent, ' ');
    ciwic_print_declaration_specifiers(&decl->specifiers, indent+4);

    ciwic_init_declarator_list *list = &decl->list;
    int nindent = indent + 4;

    while (list != NULL) {
        printf("%*cinit declarator list: \n", nindent, ' ');
        ciwic_print_declarator(&list->declarator, nindent+4);
        if (list->initializer != NULL) {
            ciwic_print_initializer(list->initializer, nindent+4);
        }

        nindent = nindent + 4;
        list = list->rest;
    }
}

void ciwic_print_statement(ciwic_statement *stmt, int indent) {
    printf("%*cstatement: ", indent, ' ');

    switch (stmt->type) {
        case ciwic_statement_label:
            printf("label\n");
            printf("%*cident: %.*s\n", indent+4, ' ', stmt->labeled.label_ident.len, stmt->labeled.label_ident.text);
            ciwic_print_statement(stmt->labeled.stmt, indent+4);
            break;
        case ciwic_statement_case:
            printf("case\n");
            ciwic_print_expr(&stmt->labeled.case_expr, indent+4);
            ciwic_print_statement(stmt->labeled.stmt, indent+4);
            break;
        case ciwic_statement_default:
            printf("default\n");
            ciwic_print_statement(stmt->labeled.stmt, indent+4);
            break;
        case ciwic_statement_block:
            printf("block\n");
            ciwic_print_statement(stmt->block.head, indent+4);
            if (stmt->block.rest != NULL)
                ciwic_print_statement(stmt->block.rest, indent+4);
            break;
        case ciwic_statement_expr:
            printf("expr\n");
            ciwic_print_expr(&stmt->expr, indent+4);
            break;
        case ciwic_statement_if:
            printf("if\n");
            ciwic_print_expr(&stmt->if_stmt.expr, indent+4);
            ciwic_print_statement(stmt->if_stmt.if_then, indent+4);
            if (stmt->if_stmt.if_else != NULL)
                ciwic_print_statement(stmt->if_stmt.if_else, indent+4);
            break;
        case ciwic_statement_switch:
            printf("switch\n");
            ciwic_print_expr(&stmt->switch_stmt.expr, indent+4);
            ciwic_print_statement(stmt->switch_stmt.stmt, indent+4);
            break;
        case ciwic_statement_while:
            printf("while\n");
            ciwic_print_expr(&stmt->while_stmt.expr, indent+4);
            ciwic_print_statement(stmt->while_stmt.stmt, indent+4);
            break;
        case ciwic_statement_do_while:
            printf("do while\n");
            ciwic_print_statement(stmt->while_stmt.stmt, indent+4);
            ciwic_print_expr(&stmt->while_stmt.expr, indent+4);
            break;
        case ciwic_statement_for:
            printf("for\n");
            printf("%*cpre decl:\n", indent+4, ' ');
            if (stmt->for_stmt.pre_decl != NULL)
                ciwic_print_declaration(stmt->for_stmt.pre_decl, indent+8);

            printf("%*cpre expr:\n", indent+4, ' ');
            if (stmt->for_stmt.pre_expr != NULL)
                ciwic_print_expr(stmt->for_stmt.pre_expr, indent+8);

            printf("%*ctest expr:\n", indent+4, ' ');
            if (stmt->for_stmt.test_expr != NULL)
                ciwic_print_expr(stmt->for_stmt.test_expr, indent+8);

            printf("%*cpost expr:\n", indent+4, ' ');
            if (stmt->for_stmt.post_expr != NULL)
                ciwic_print_expr(stmt->for_stmt.post_expr, indent+8);

            ciwic_print_statement(stmt->for_stmt.stmt, indent+4);
            break;
        case ciwic_statement_goto:
            printf("goto\n");
            printf("%*cident: %.*s\n", indent+4, ' ', stmt->goto_ident.len, stmt->goto_ident.text);
            break;
        case ciwic_statement_continue:
            printf("continue\n");
            break;
        case ciwic_statement_break:
            printf("break\n");
            break;
        case ciwic_statement_return:
            printf("return\n");
            if (stmt->return_expr != NULL)
                ciwic_print_expr(stmt->return_expr, indent+4);
            break;
        case ciwic_statement_null:
            printf("null\n");
            break;
    }
}

void ciwic_print_declaration_list(ciwic_declaration_list *decl_list, int indent) {
    printf("%*cdeclaration list: \n", indent, ' ');
    ciwic_print_declaration(&decl_list->head, indent+4);
    if (decl_list->rest != NULL)
        ciwic_print_declaration_list(decl_list->rest, indent+4);
}

void ciwic_print_func_definition(ciwic_func_definition *func_def, int indent) {
    printf("%*cfunction definition: \n", indent, ' ');
    ciwic_print_declaration_specifiers(&func_def->specifiers, indent+4);
    ciwic_print_declarator(&func_def->declarator, indent+4);
    if (func_def->decl_list != NULL)
        ciwic_print_declaration_list(func_def->decl_list, indent+4);
    ciwic_print_statement(&func_def->statement, indent+4);
}

void ciwic_print_translation_unit(ciwic_translation_unit *translation_unit, int indent) {
    printf("%*ctranslation unit: \n", indent, ' ');
    switch (translation_unit->def_type) {
        case ciwic_definition_func:
            ciwic_print_func_definition(&translation_unit->func, indent+4);
            break;
        case ciwic_definition_decl:
            ciwic_print_declaration(&translation_unit->decl, indent+4);
            break;
    }

    if (translation_unit->rest != NULL)
        ciwic_print_translation_unit(translation_unit->rest, indent+4);
}
