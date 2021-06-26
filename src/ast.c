#include <ast.h>
#include <stdio.h>

const char* ciwic_expr_unary_op_table[] = {
    "post dec", "post inc", "pre dec", "pre inc", "ref", "deref", "pos", "neg",
    "bit inv", "log inv" };

const char* ciwic_expr_binary_op_table[] = { "mul", "div", "mod", "add",
    "sub", "shift right", "shift left", "less than", "greater than",
    "less or eq", "greater or eq", "eq", "neq", "and", "xor", "or", "land",
    "lor", "comma" };

void ciwic_print_arg_list(ciwic_expr_arg_list *list, int indent) {
    printf("%*carg list:\n", indent, ' ');
    ciwic_print_expr(&list->head, indent+4);
    if (list->rest != NULL) {
        ciwic_print_arg_list(list->rest, indent+4);
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
            printf("%*cconstant:\n", indent, ' ');
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
            // TODO: print type name
            break;
        case ciwic_expr_type_cast:
            printf("%*ccast: \n", indent, ' ');
            // TODO: print type name
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

