#pragma once

#include <parselib.h>

typedef enum {
    ciwic_expr_type_unary_op,
    ciwic_expr_type_binary_op,
    ciwic_expr_type_call,
    ciwic_expr_type_identifier,
    ciwic_expr_type_constant,
    ciwic_expr_type_initialize,
    ciwic_expr_type_subscript,
    ciwic_expr_type_member,
    ciwic_expr_type_member_deref,
    ciwic_expr_type_sizeof_expr,
    ciwic_expr_type_sizeof_type,
    ciwic_expr_type_cast,
    ciwic_expr_type_conditional,
    ciwic_expr_type_assignment,
} ciwic_expr_type;

typedef enum {
    ciwic_expr_op_post_dec,
    ciwic_expr_op_post_inc,
    ciwic_expr_op_pre_dec,
    ciwic_expr_op_pre_inc,
    ciwic_expr_op_ref,
    ciwic_expr_op_deref,
    ciwic_expr_op_pos,
    ciwic_expr_op_neg,
    ciwic_expr_op_bitneg,
    ciwic_expr_op_boolneg,
} ciwic_expr_unary_op;

typedef enum {
    ciwic_expr_op_mul,
    ciwic_expr_op_div,
    ciwic_expr_op_mod,
    ciwic_expr_op_add,
    ciwic_expr_op_sub,
    ciwic_expr_op_sr,
    ciwic_expr_op_sl,
    ciwic_expr_op_lt,
    ciwic_expr_op_gt,
    ciwic_expr_op_le,
    ciwic_expr_op_ge,
    ciwic_expr_op_eq,
    ciwic_expr_op_neq,
    ciwic_expr_op_and,
    ciwic_expr_op_xor,
    ciwic_expr_op_or,
    ciwic_expr_op_land,
    ciwic_expr_op_lor,
    ciwic_expr_op_comma,
} ciwic_expr_binary_op;

typedef struct ciwic_type_name {
    char *name;
} ciwic_type_name;

typedef struct ciwic_initializer_list {
    struct ciwic_expr *list;
} ciwic_initializer_list;

typedef struct ciwic_expr {
    ciwic_expr_type type;
    union {
        string identifier;
        string constant;
        struct {
            ciwic_expr_unary_op op;
            struct ciwic_expr *inner;
        } unary_op;
        struct {
            ciwic_expr_binary_op op;
            struct ciwic_expr *fst;
            struct ciwic_expr *snd;
        } binary_op;
        struct {
            struct ciwic_expr* fun;
            struct ciwic_expr_arg_list* args; // Can be null
        } call;
        struct {
            ciwic_type_name type_name;
            ciwic_initializer_list initializer_list;
        } initialize;
        struct {
            struct ciwic_expr *val;
            struct ciwic_expr *pos;
        } subscript;
        struct {
            struct ciwic_expr *expr;
            string identifier;
        } member;
        struct ciwic_expr *sizeof_expr;
        ciwic_type_name sizeof_type;
        struct {
            ciwic_type_name type_name;
            struct ciwic_expr *expr;
        } cast;
        struct {
            struct ciwic_expr *cond;
            struct ciwic_expr *left;
            struct ciwic_expr *right;
        } conditional;
        struct {
            ciwic_expr_binary_op op;
            struct ciwic_expr *left;
            struct ciwic_expr *right;
        } assignment;
    };
} ciwic_expr;

typedef struct ciwic_expr_arg_list {
    struct ciwic_expr head;
    struct ciwic_expr_arg_list *rest; // Can be null
} ciwic_expr_arg_list;


void ciwic_print_expr(ciwic_expr *expr, int indent);
