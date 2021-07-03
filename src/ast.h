#pragma once

#include <parselib.h>

// Expressions

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

typedef enum {
    ciwic_specifier_typedef = 1 << 0,
    ciwic_specifier_extern = 1 << 1,
    ciwic_specifier_static = 1 << 2,
    ciwic_specifier_auto = 1 << 3,
    ciwic_specifier_register = 1 << 4,
} ciwic_storage_class;

typedef enum {
    ciwic_type_qualifier_const = 1 << 0,
    ciwic_type_qualifier_restrict = 1 << 1,
    ciwic_type_qualifier_volatile = 1 << 2,
} ciwic_type_qualifier;

typedef enum {
    ciwic_function_specifier_inline = 1 << 0,
} ciwic_function_specifier;

typedef enum {
    ciwic_type_void = 1 << 0,
    ciwic_type_char = 1 << 1,
    ciwic_type_short = 1 << 2,
    ciwic_type_int = 1 << 3,
    ciwic_type_long = 1 << 4,
    ciwic_type_long_long = 1 << 5,
    ciwic_type_float = 1 << 6,
    ciwic_type_double = 1 << 7,
    ciwic_type_signed = 1 << 8,
    ciwic_type_unsigned = 1 << 9,
    ciwic_type_bool = 1 << 10,
    ciwic_type_complex = 1 << 11,
} ciwic_type_prim;

typedef enum {
    ciwic_type_spec_none,
    ciwic_type_spec_prim,
    ciwic_type_spec_enum,
    ciwic_type_spec_struct,
    ciwic_type_spec_union,
    ciwic_type_spec_typedef_name,
} ciwic_type_spec;

typedef struct ciwic_declaration_specifiers {
    int storage_class;
    int func_specifiers;
    int type_qualifiers;
    ciwic_type_spec type_spec;
    union {
        struct {
            // INVARIANT: At most one of identifer or decl is null
            string *identifier; // Can be null
            struct ciwic_struct_list *decl; // Can be null
        } struct_or_union;
        struct {
            // INVARIANT: At most one of identifer or decl is null
            string *identifier; // Can be null
            struct ciwic_enum_list *decl; // Can be null
        } enum_;
        string typedef_name;
        int prim_type;
    };
} ciwic_declaration_specifiers;


typedef struct ciwic_type_name {
    ciwic_declaration_specifiers specifiers;
    struct ciwic_declarator *declarator; // Can be null
} ciwic_type_name;

typedef struct ciwic_initializer_list {
    struct ciwic_designator_list *designation; // Can be null
    struct ciwic_initializer *initializer;
    struct ciwic_initializer_list *rest; // Can be null
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


// Declarations

typedef enum {
    ciwic_declarator_pointer,
    ciwic_declarator_identifier,
    ciwic_declarator_array,
    ciwic_declarator_func,
    // TODO: add support for old type functions
    ciwic_declarator_func_old,
} ciwic_declarator_type;

typedef struct ciwic_declarator {
    ciwic_declarator_type type;
    struct ciwic_declarator *inner; // Can be null
    union {
        int pointer_qualifiers;
        string ident;
        struct {
            int is_static;
            int is_var_len;
            int type_qualifiers;
            ciwic_expr *expr; // Can be null
        } array;
        struct {
            int has_ellipsis;
            struct ciwic_param_list *param_list; // Can be null
        } func;
    };
} ciwic_declarator;

typedef enum {
    ciwic_initializer_init_expr,
    ciwic_initializer_init_list,
} ciwic_initializer_type;

typedef struct ciwic_initializer {
    ciwic_initializer_type type;
    union {
        ciwic_expr expr;
        ciwic_initializer_list list;
    };
} ciwic_initializer;

typedef enum {
    ciwic_designator_expr,
    ciwic_designator_ident,
} ciwic_designator_type;

typedef struct ciwic_designator_list {
    ciwic_designator_type type;
    union {
        ciwic_expr expr;
        string ident;
    };
    struct ciwic_designator_list *rest; // Can be null
} ciwic_designator_list;

typedef struct ciwic_struct_declarator_list {
    // INVARIANT: at most one of declarator and expr is null
    ciwic_declarator *declarator; // Can be null
    ciwic_expr *expr; // Can be null
    struct ciwic_struct_declarator_list *rest; // Can be null
} ciwic_struct_declarator_list;

typedef struct ciwic_struct_list {
    ciwic_declaration_specifiers specifiers;
    ciwic_struct_declarator_list declarator_list;
    struct ciwic_struct_list *rest; // Can be null
} ciwic_struct_list;

typedef struct ciwic_enum_list {
    string name;
    ciwic_expr *expr; // Can be null
    struct ciwic_enum_list *rest; // Can be null
} ciwic_enum_list;

typedef struct ciwic_init_declarator_list {
    ciwic_declarator declarator;
    ciwic_initializer *initializer; // Can be null
    struct ciwic_init_declarator_list *rest; // Can be null
} ciwic_init_declarator_list;

typedef struct ciwic_declaration {
    ciwic_declaration_specifiers specifiers;
    ciwic_init_declarator_list list;
} ciwic_declaration;

typedef struct ciwic_param_list {
    ciwic_declaration_specifiers specifiers;
    ciwic_declarator *declarator; // Can be null
    struct ciwic_param_list *rest; // Can be null
} ciwic_param_list;


int ciwic_declarator_is_abstract(ciwic_declarator *declarator);

void ciwic_print_expr(ciwic_expr *expr, int indent);
void ciwic_print_declaration_specifiers(ciwic_declaration_specifiers *specs, int indent);
void ciwic_print_declarator(ciwic_declarator *decl, int indent);
void ciwic_print_declaration(ciwic_declaration *decl, int indent);
void ciwic_print_initializer(ciwic_initializer *init, int indent);
