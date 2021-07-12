#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <parser.h>
#include <ast.h>

/* Based on C99 standard N1256 draft from:
 * http://www.open-std.org/jtc1/sc22/WG14/www/docs/n1256.pdf
 */

const int ciwic_parser_keywords_len = 37;
const char* ciwic_parser_keywords[37] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
    "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary" };

const char* ciwic_prim_types_keywords[12] = {"void", "char", "short", "int",
    "long", "long", "float", "double", "signed", "unsigned", "_Bool",
    "_Complex"};

const ciwic_type_prim ciwic_prim_types_list[12] = {ciwic_type_void,
    ciwic_type_char, ciwic_type_short, ciwic_type_int, ciwic_type_long,
    ciwic_type_long_long, ciwic_type_float, ciwic_type_double,
    ciwic_type_signed, ciwic_type_unsigned, ciwic_type_bool,
    ciwic_type_complex};


ciwic_parser ciwic_parser_new(char *buf, int len) {
    ciwic_parser res;
    res.text = buf;
    res.pos = 0;
    res.len = len;
    return res;
}

int ciwic_parser_char(ciwic_parser *parser, char *res) {
    if (parser->pos >= parser->len) {
        return 1;
    }
    *res = parser->text[parser->pos];
    parser->pos += 1;

    return 0;
}

int ciwic_parser_lookahead(ciwic_parser *parser, char *res) {
    if (parser->pos >= parser->len) {
        return 1;
    }
    *res = parser->text[parser->pos];

    return 0;
}

int ciwic_parser_match_char(ciwic_parser *parser, const char match) {
    char res;
    if (ciwic_parser_lookahead(parser, &res)) {
        return 1;
    }

    if (match != res) {
        return 1;
    }

    if (ciwic_parser_char(parser, &res)) {
        return 1;
    }

    return 0;
}

int ciwic_parser_match_string(ciwic_parser *parser, const char* str) {
    int pos = parser->pos;

    for (int i = 0;; i++) {
        if (str[i] == 0) {
            return 0;
        }

        if (ciwic_parser_match_char(parser, str[i])) {
            parser->pos = pos;
            return 1;
        }
    }

    return 0;
}

int ciwic_parser_whitespace(ciwic_parser *parser) {
    char c;
    int len = 0;

    while (!ciwic_parser_lookahead(parser, &c)) {
        if (c != ' ' && c != '\n' && c != '\t') {
            break;
        }
        if (!ciwic_parser_char(parser, &c)) {
            break;
        }
        len += 1;
    }

    if (len == 0) {
        return 1;
    }

    return 0;
}

int ciwic_parser_is_letter(char l) {
    return (l >= 'a' && l <= 'z') || (l >= 'A' && l <= 'Z') || l == '_';
}

int ciwic_parser_is_digit(char l) {
    return l >= '0' && l <= '9';
}

int ciwic_parser_letter(ciwic_parser *parser, char *res) {
    char letter;
    if (ciwic_parser_lookahead(parser, &letter)) {
        return 1;
    }

    if (!ciwic_parser_is_letter(letter)) {
        return 1;
    }

    return ciwic_parser_char(parser, res);
}

int ciwic_parser_digit(ciwic_parser *parser, char *res) {
    char digit;
    if (ciwic_parser_lookahead(parser, &digit)) {
        return 1;
    }

    if (!ciwic_parser_is_digit(digit)) {
        return 1;
    }

    return ciwic_parser_char(parser, res);
}

int ciwic_parser_letterdigit(ciwic_parser *parser, char *res) {
    if (!ciwic_parser_letter(parser, res)) {
        return 0;
    }

    if (!ciwic_parser_digit(parser, res)) {
        return 0;
    }

    return 1;
}

int ciwic_parser_digits(ciwic_parser *parser, string *res) {
    char digit;
    int len = 0;
    char *start = &parser->text[parser->pos];

    while (!ciwic_parser_digit(parser, &digit)) {
        len += 1;
    }

    res->text = start;
    res->len  = len;

    return 0;
}

int ciwic_parser_dec_nat(ciwic_parser *parser, int *res) {
    int val = 0;
    int len = 0;
    char c;

    while (!ciwic_parser_digit(parser, &c)) {
        if (val > INT_MAX/10) {
            return 1;
        }
        val *= 10;
        int cval = (int) (c - '0');
        if (val > INT_MAX - cval) {
            return 1;
        }
        val += cval;
        len += 1;
    }

    if (len == 0) {
        return 1;
    }

    *res = val;

    return 0;
}

int ciwic_parser_nat(ciwic_parser *parser, int *res) {
    return ciwic_parser_dec_nat(parser, res);
}

int ciwic_parser_int(ciwic_parser *parser, int *res) {
    int pos = parser->pos;
    int neg = 0;
    if (!ciwic_parser_match_char(parser, '-')) {
        neg = 1;
    }

    if (ciwic_parser_nat(parser, res)) {
        parser->pos = pos;
        return 1;
    }

    if (neg) {
        *res = *res * (-1);
    }

    return 0;
}

int ciwic_parser_ident(ciwic_parser *parser, string *ident) {
    char c;
    int len = 0;
    char *start = &parser->text[parser->pos];

    if (ciwic_parser_letter(parser, &c)) {
        return 1;
    }

    len += 1;

    while (!ciwic_parser_letterdigit(parser, &c)) {
        len += 1;
    }

    ident->text = start;
    ident->len  = len;
    
    return 0;
}

int ciwic_parser_keyword(ciwic_parser *parser, const char *keyword) {
    char c;
    int pos = parser->pos;

    ciwic_parser_whitespace(parser);

    if (ciwic_parser_match_string(parser, keyword)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_lookahead(parser, &c)) {
        // End of file is fine here
        return 0;
    }

    if (ciwic_parser_is_letter(c) || ciwic_parser_is_digit(c)) {
        // We have not reached the end of the identifier
        parser->pos = pos;
        return 1;
    }

    return 0;
}

int ciwic_parser_identifier(ciwic_parser *parser, string *identifier) {
    // TODO: check for typename

    string ident;
    int pos = parser->pos;

    ciwic_parser_whitespace(parser);

    if (ciwic_parser_ident(parser, &ident)) {
        return 1;
    }

    for (int i = 0; i < ciwic_parser_keywords_len; i++) {
        size_t len = strlen(ciwic_parser_keywords[i]);
        if (ident.len != len) {
            continue;
        }
        if (memcmp(ident.text, ciwic_parser_keywords[i], len) == 0) {
            parser->pos = pos;
            return 1;
        }
    }

    *identifier = ident;

    return 0;
}

int ciwic_parser_punctuation(ciwic_parser *parser, const char* punct) {
    const char* punctuators[55] = {"%:%:", "%:", "%>", "<%", ":>", "<:", "##",
        "#", ",", "|=", "^=", "&=", ">>=", "<<=", "-=", "+=", "%=", "/=", "*=",
        "==", "=", "...", ";", ":", "?", "||", "&&", "|", "^", "!=", "<=",
        ">=", "<<", ">>", "<", ">", "&", "/", "!", "~", "->", "--", "++", "-",
        "+", "*", "%", "&", ".", "{", "}", "(", ")", "[", "]"};

    int pos = parser->pos;

    ciwic_parser_whitespace(parser);

    for (int i = 0; i < 55; i++) {
        if (!ciwic_parser_match_string(parser, punctuators[i])) {
            if (strcmp(punct, punctuators[i]) != 0) {
                parser->pos = pos;
                return 1;
            }

            return 0;
        }
    }

    return 1;
}

int ciwic_parser_primary_expr(ciwic_parser *parser, ciwic_expr *res) {
    int pos = parser->pos;

    string identifier;
    if (!ciwic_parser_identifier(parser, &identifier)) {
        res->type = ciwic_expr_type_identifier;
        res->identifier = identifier;
        return 0;
    }

    // TODO: constant
    // TODO: string literal

    if (!ciwic_parser_punctuation(parser, "(")) {
        if (ciwic_parser_expr(parser, res)) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }
        return 0;
    }

    return 1;
}

int ciwic_parser_postfix_expr(ciwic_parser *parser, ciwic_expr *inner, ciwic_expr *res) {
    int pos = parser->pos;

    if (inner == NULL) {
        ciwic_expr start;
        if (!ciwic_parser_primary_expr(parser, &start)) {
            if (!ciwic_parser_postfix_expr(parser, &start, res)) {
                return 0;
            }
            *res = start;
            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "(")) {
            ciwic_type_name type_name;
            ciwic_initializer_list initializer_list;
            if (ciwic_parser_type_name(parser, &type_name)) {
                parser->pos = pos;
                return 1;
            }
            
            if (ciwic_parser_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_punctuation(parser, "{")) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_initializer_list(parser, &initializer_list)) {
                parser->pos = pos;
                return 1;
            }

            ciwic_parser_punctuation(parser, ",");

            if (ciwic_parser_punctuation(parser, "}")) {
                parser->pos = pos;
                return 1;
            }

            res->type = ciwic_expr_type_initialize;
            res->initialize.type_name = type_name;
            res->initialize.initializer_list = initializer_list;
            return 0;
        }

        parser->pos = pos;
        return 1;
    } else {
        if (!ciwic_parser_punctuation(parser, "[")) {
            ciwic_expr subscript, expr;
            if (ciwic_parser_expr(parser, &expr)) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_punctuation(parser, "]")) {
                parser->pos = pos;
                return 1;
            }

            subscript.type = ciwic_expr_type_subscript;
            subscript.subscript.val = malloc(sizeof(ciwic_expr));
            *subscript.subscript.val = *inner;
            subscript.subscript.pos = malloc(sizeof(ciwic_expr));
            *subscript.subscript.pos = expr;

            if (ciwic_parser_postfix_expr(parser, &subscript, res)) {
                parser->pos = pos;
                return 1;
            }

            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "(")) {
            ciwic_expr call;
            ciwic_expr_arg_list arg_list;

            call.type = ciwic_expr_type_call;
            call.call.fun = malloc(sizeof(ciwic_expr));
            *call.call.fun = *inner;

            if (!ciwic_parser_expr_arg_list(parser, &arg_list)) {
                call.call.args = malloc(sizeof(ciwic_expr_arg_list));
                *call.call.args = arg_list;
            } else {
                call.call.args = NULL;
            }

            if (ciwic_parser_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            return ciwic_parser_postfix_expr(parser, &call, res);
        }

        if (!ciwic_parser_punctuation(parser, ".")) {
            ciwic_expr member;
            string identifier;

            if (ciwic_parser_identifier(parser, &identifier)) {
                parser->pos = pos;
                return 1;
            }

            member.type = ciwic_expr_type_member;
            member.member.expr = malloc(sizeof(ciwic_expr));
            *member.member.expr = *inner;
            member.member.identifier = identifier;

            if (ciwic_parser_postfix_expr(parser, &member, res)) {
                parser->pos = pos;
                return 1;
            }
            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "->")) {
            ciwic_expr member;
            string identifier;

            if (ciwic_parser_identifier(parser, &identifier)) {
                parser->pos = pos;
                return 1;
            }

            member.type = ciwic_expr_type_member_deref;
            member.member.expr = malloc(sizeof(ciwic_expr));
            *member.member.expr = *inner;
            member.member.identifier = identifier;

            if (ciwic_parser_postfix_expr(parser, &member, res)) {
                parser->pos = pos;
                return 1;
            }
            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "++")) {
            ciwic_expr expr;
            expr.type = ciwic_expr_type_unary_op;
            expr.unary_op.op = ciwic_expr_op_post_inc;
            expr.unary_op.inner = malloc(sizeof(ciwic_expr));
            *expr.unary_op.inner = *inner;

            if (ciwic_parser_postfix_expr(parser, &expr, res)) {
                parser->pos = pos;
                return 1;
            }
            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "--")) {
            ciwic_expr expr;
            expr.type = ciwic_expr_type_unary_op;
            expr.unary_op.op = ciwic_expr_op_post_dec;
            expr.unary_op.inner = malloc(sizeof(ciwic_expr));
            *expr.unary_op.inner = *inner;

            if (ciwic_parser_postfix_expr(parser, &expr, res)) {
                parser->pos = pos;
                return 1;
            }
            return 0;
        }

        *res = *inner;

        return 0;
    }
}

int ciwic_parser_expr_arg_list(ciwic_parser *parser, ciwic_expr_arg_list *res) {
    ciwic_expr arg;
    ciwic_expr_arg_list rest;

    int pos = parser->pos;

    if (ciwic_parser_assignment_expr(parser, &arg)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ",")) {
        res->head = arg;
        res->rest = NULL;
        return 0;
    }

    if (ciwic_parser_expr_arg_list(parser, &rest)) {
        parser->pos = pos;
        return 1;
    }

    res->head = arg;
    res->rest = malloc(sizeof(ciwic_expr_arg_list));
    *res->rest = rest;

    return 0;
}

int ciwic_parser_unary_expr(ciwic_parser *parser, ciwic_expr *res) {
    const char* unary_ops_punct[8] = {"++", "--", "&", "*", "+", "-", "~", "!"};

    ciwic_expr_unary_op unary_ops_op[8] = {ciwic_expr_op_pre_inc,
        ciwic_expr_op_pre_dec, ciwic_expr_op_ref, ciwic_expr_op_deref,
        ciwic_expr_op_pos, ciwic_expr_op_neg, ciwic_expr_op_bitneg,
        ciwic_expr_op_boolneg};

    int pos = parser->pos;

    for (int i = 0; i < 8; i++) {
        if (!ciwic_parser_punctuation(parser, unary_ops_punct[i])) {
            ciwic_expr inner;
            if (ciwic_parser_unary_expr(parser, &inner)) {
                parser->pos = pos;
                return 1;
            }
            res->type = ciwic_expr_type_unary_op;
            res->unary_op.op = unary_ops_op[i];
            res->unary_op.inner = malloc(sizeof(ciwic_expr));
            *res->unary_op.inner = inner;
            return 0;
        }
    }

    if (!ciwic_parser_keyword(parser, "sizeof")) {
        ciwic_expr expr;
        ciwic_type_name type_name;
        if (!ciwic_parser_unary_expr(parser, &expr)) {
            res->type = ciwic_expr_type_sizeof_expr;
            res->sizeof_expr = malloc(sizeof(ciwic_expr));
            *res->sizeof_expr = expr;
            return 0;
        }

        if (!ciwic_parser_punctuation(parser, "(")) {
            if (ciwic_parser_type_name(parser, &type_name)) {
                parser->pos = pos;
                return 1;
            }
            if (ciwic_parser_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            res->type = ciwic_expr_type_sizeof_type;
            res->sizeof_type = type_name;
            return 0;
        }

        parser->pos = pos;
        return 1;
    }

    if (!ciwic_parser_postfix_expr(parser, NULL, res)) {
        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_cast_expr(ciwic_parser *parser, ciwic_expr *res) {
    ciwic_expr expr;
    ciwic_type_name type_name;

    int pos = parser->pos;

    if (!ciwic_parser_unary_expr(parser, res)) {
        return 0;
    }

    if (!ciwic_parser_punctuation(parser, "(")) {
        if (ciwic_parser_type_name(parser, &type_name)) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_cast_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        res->type = ciwic_expr_type_cast;
        res->cast.type_name = type_name;
        res->cast.expr = malloc(sizeof(ciwic_expr));
        *res->cast.expr = expr;

        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_binop_expr(ciwic_parser *parser, int level, ciwic_expr *inner, ciwic_expr *res) {
    const int op_table_lens[10] = {3, 2, 2, 4, 2, 1, 1, 1, 1, 1};

    const char* op_table_punct[10][4] = {
        {"*", "/", "%"},
        {"+", "-"},
        {"<<", ">>"},
        {"<", ">", "<=", ">="},
        {"==", "!=",},
        {"&"},
        {"^"},
        {"|"},
        {"&&"},
        {"||"}
    };

    const ciwic_expr_binary_op op_table_vals[10][4] = {
        {ciwic_expr_op_mul, ciwic_expr_op_div, ciwic_expr_op_mod},
        {ciwic_expr_op_add, ciwic_expr_op_sub},
        {ciwic_expr_op_sl, ciwic_expr_op_sr},
        {ciwic_expr_op_lt, ciwic_expr_op_gt, ciwic_expr_op_le, ciwic_expr_op_ge},
        {ciwic_expr_op_eq, ciwic_expr_op_neq},
        {ciwic_expr_op_and},
        {ciwic_expr_op_xor},
        {ciwic_expr_op_or},
        {ciwic_expr_op_land},
        {ciwic_expr_op_lor}
    };

    int pos = parser->pos;

    if (level == 0) {
        return ciwic_parser_cast_expr(parser, res);
    }
    level--;

    ciwic_expr expr, outer;
    ciwic_expr_binary_op op;
    int found_op = 0;
    
    if (inner == NULL) {
        if (ciwic_parser_binop_expr(parser, level, NULL, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_binop_expr(parser, level+1, &expr, res)) {
            parser->pos = pos;
            return 1;
        }

        return 0;
    } else {
        for (int i = 0; i < op_table_lens[level]; i++) {
            if (!ciwic_parser_punctuation(parser, op_table_punct[level][i])) {
                op = op_table_vals[level][i];
                found_op = 1;
                break;
            }
        }

        if (!found_op) {
            *res = *inner;
            return 0;
        }


        if (ciwic_parser_binop_expr(parser, level, NULL, &expr)) {
            parser->pos = pos;
            return 1;
        }

        outer.type = ciwic_expr_type_binary_op;
        outer.binary_op.op = op;
        outer.binary_op.fst = malloc(sizeof(ciwic_expr));
        *outer.binary_op.fst = *inner;
        outer.binary_op.snd = malloc(sizeof(ciwic_expr));
        *outer.binary_op.snd = expr;

        if (ciwic_parser_binop_expr(parser, level+1, &outer, res)) {
            parser->pos = pos;
            return 1;
        }

        return 0;
    }
 }

int ciwic_parser_logical_or_expr(ciwic_parser *parser, ciwic_expr *res) {
    return ciwic_parser_binop_expr(parser, 10, NULL, res);
}

int ciwic_parser_conditional_expr(ciwic_parser *parser, ciwic_expr *cond, ciwic_expr *res) {
    ciwic_expr first, left, right;

    int pos = parser->pos;

    if (cond == NULL) {
        cond = &first;
        if (ciwic_parser_logical_or_expr(parser, cond)) {
            parser->pos = pos;
            return 1;
        }
    }

    if (ciwic_parser_punctuation(parser, "?")) {
        *res = *cond;
        return 0;
    }

    if (ciwic_parser_expr(parser, &left)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ":")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_conditional_expr(parser, NULL, &right)) {
        parser->pos = pos;
        return 1;
    }

    res->type = ciwic_expr_type_conditional;
    res->conditional.cond = malloc(sizeof(ciwic_expr));
    *res->conditional.cond = *cond;
    res->conditional.left = malloc(sizeof(ciwic_expr));
    *res->conditional.left = left;
    res->conditional.right = malloc(sizeof(ciwic_expr));
    *res->conditional.right = right;

    return 0;
}

int ciwic_parser_const_expr(ciwic_parser *parser, ciwic_expr *expr) {
    return ciwic_parser_conditional_expr(parser, NULL, expr);
}

int ciwic_parser_conditional_expr_with_unary(ciwic_parser *parser, ciwic_expr *unary, ciwic_expr *res) {
    ciwic_expr inner, next;
    int pos = parser->pos;

    if (unary == NULL) {
        return ciwic_parser_conditional_expr(parser, NULL, res);
    }

    inner = *unary;

    for (int i = 1; i <= 10; i++) {
        if (ciwic_parser_binop_expr(parser, i, &inner, &next)) {
            parser->pos = pos;
            return 1;
        }
        inner = next;
    }

    if (ciwic_parser_conditional_expr(parser, &inner, res)) {
        parser->pos = pos;
        return 1;
    }

    return 0;
}

int ciwic_parser_assignment_expr(ciwic_parser *parser, ciwic_expr *res) {
    const char* op_table_punct[11] = {"=", "*=", "/=", "%=", "+=", "-=", "<<=",
        ">>=", "&=", "^=", "|="};

    const ciwic_expr_binary_op op_table_val[11] = { ciwic_expr_op_comma,
        ciwic_expr_op_mul, ciwic_expr_op_div, ciwic_expr_op_mod,
        ciwic_expr_op_add, ciwic_expr_op_sub, ciwic_expr_op_sl,
        ciwic_expr_op_sr, ciwic_expr_op_and, ciwic_expr_op_xor,
        ciwic_expr_op_or};

    ciwic_expr left, right;

    int pos = parser->pos;

    if (ciwic_parser_unary_expr(parser, &left)) {
        // FIXME: this is inefficient
        return ciwic_parser_conditional_expr(parser, NULL, res);
    }

    for (int i = 0; i < 11; i++) {
        if (!ciwic_parser_punctuation(parser, op_table_punct[i])) {
            if (ciwic_parser_assignment_expr(parser, &right)) {
                parser->pos = pos;
                return 1;
            }

            res->type = ciwic_expr_type_assignment;
            res->assignment.op = op_table_val[i];
            res->assignment.left = malloc(sizeof(ciwic_expr));
            *res->assignment.left = left;
            res->assignment.right = malloc(sizeof(ciwic_expr));
            *res->assignment.right = right;

            return 0;
        }
    }

    if (ciwic_parser_conditional_expr_with_unary(parser, &left, res)) {
        parser->pos = pos;
        return 1;
    }

    return 0;
}

int ciwic_parser_expr(ciwic_parser *parser, ciwic_expr *res) {
    ciwic_expr fst, snd;

    int pos = parser->pos;

    if (ciwic_parser_assignment_expr(parser, &fst)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ",")) {
        *res = fst;
        return 0;
    }

    if (ciwic_parser_expr(parser, &snd)) {
        parser->pos = pos;
        return 1;
    }

    res->type = ciwic_expr_type_binary_op;
    res->binary_op.op = ciwic_expr_op_comma;
    res->binary_op.fst = malloc(sizeof(ciwic_expr));
    *res->binary_op.fst = fst;
    res->binary_op.snd = malloc(sizeof(ciwic_expr));
    *res->binary_op.snd = snd;

    return 0;
}

// Declarations

int ciwic_parser_storage_class(ciwic_parser *parser, ciwic_storage_class *storage) {
    if (!ciwic_parser_keyword(parser, "typedef")) {
        *storage = ciwic_specifier_typedef;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "extern")) {
        *storage = ciwic_specifier_extern;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "static")) {
        *storage = ciwic_specifier_static;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "auto")) {
        *storage = ciwic_specifier_auto;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "register")) {
        *storage = ciwic_specifier_register;
        return 0;
    }

    return 1;
}

int ciwic_parser_type_qualifier(ciwic_parser *parser, ciwic_type_qualifier *qualifier) {
    if (!ciwic_parser_keyword(parser, "const")) {
        *qualifier = ciwic_type_qualifier_const;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "restrict")) {
        *qualifier = ciwic_type_qualifier_restrict;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "volatile")) {
        *qualifier = ciwic_type_qualifier_volatile;
        return 0;
    }

    return 1;
}

int ciwic_parser_function_specifier(ciwic_parser *parser, ciwic_function_specifier *specifier) {
    if (!ciwic_parser_keyword(parser, "inline")) {
        *specifier = ciwic_function_specifier_inline;
        return 0;
    }

    return 1;
}

int ciwic_parser_type_prim(ciwic_parser *parser, ciwic_type_prim *type) {
    for (int i = 0; i < 12; i++) {
        if(!ciwic_parser_keyword(parser, ciwic_prim_types_keywords[i])) {
            *type = ciwic_prim_types_list[i];
            return 0;
        }
    }

    return 1;
}

int ciwic_parser_enum_list_inner(ciwic_parser *parser, ciwic_enum_list *list) {
    string name;
    ciwic_expr expr;
    ciwic_expr *expr_ptr = NULL;
    ciwic_enum_list inner;

    int pos = parser->pos;

    if (ciwic_parser_identifier(parser, &name)) {
        return 0;
    }

    if (!ciwic_parser_punctuation(parser, "=")) {
        if (ciwic_parser_const_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }
        expr_ptr = malloc(sizeof(ciwic_expr));
        *expr_ptr = expr;
    }

    list->name = name;
    list->expr = expr_ptr;

    int comma_res = ciwic_parser_punctuation(parser, ",");

    if (comma_res || ciwic_parser_enum_list_inner(parser, &inner)) {
        list->rest = NULL;
        return 0;
    }

    list->rest = malloc(sizeof(ciwic_enum_list));
    *list->rest = inner;

    return 0;
}

int ciwic_parser_enum_list(ciwic_parser *parser, ciwic_enum_list *list) {
    ciwic_enum_list inner;
    int pos = parser->pos;

    if (ciwic_parser_punctuation(parser, "{")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_enum_list_inner(parser, &inner)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, "}")) {
        parser->pos = pos;
        return 1;
    }

    *list = inner;
    return 0;
}

int ciwic_parser_struct_declarator_list(ciwic_parser *parser, ciwic_struct_declarator_list *list) {
    ciwic_declarator decl;
    ciwic_expr expr;
    ciwic_struct_declarator_list rest;
    int expr_res = 1, rest_res = 1;

    int pos = parser->pos;

    int decl_res = ciwic_parser_declarator(parser, NULL, &decl);

    if (!ciwic_parser_punctuation(parser, ":")) {
        if ((expr_res = ciwic_parser_const_expr(parser, &expr))) {
           parser->pos = pos;
            return 1;
        }
    }

    if (decl_res && expr_res) {
        parser->pos = pos;
        return 1;
    }

    if (!ciwic_parser_punctuation(parser, ",")) {
        if ((rest_res = ciwic_parser_struct_declarator_list(parser, &rest))) {
            parser->pos = pos;
            return 1;
        }
    }

    if (!decl_res) {
        list->declarator = malloc(sizeof(ciwic_declarator));
        *list->declarator = decl;
    } else {
        list->declarator = NULL;
    }

    if (!expr_res) {
        list->expr = malloc(sizeof(ciwic_expr));
        *list->expr = expr;
    } else {
        list->expr = NULL;
    }

    if (!rest_res) {
        list->rest = malloc(sizeof(ciwic_struct_declarator_list));
        *list->rest = rest;
    } else {
        list->rest = NULL;
    }

    return 0;
}

int ciwic_parser_specifier_qualifier_list(ciwic_parser *parser, ciwic_declaration_specifiers* specifiers) {
    ciwic_declaration_specifiers specs;

    int pos = parser->pos;

    if (ciwic_parser_declaration_specifiers(parser, &specs)) {
        parser->pos = pos;
        return 1;
    }

    if (specs.storage_class != 0) {
        parser->pos = pos;
        return 1;
    }

    if (specs.func_specifiers != 0) {
        parser->pos = pos;
        return 1;
    }

    *specifiers = specs;
    return 0;
}

int ciwic_parser_struct_list_inner(ciwic_parser *parser, ciwic_struct_list *list) {
    ciwic_declaration_specifiers specifiers;
    ciwic_struct_declarator_list decl_list;
    ciwic_struct_list rest;

    int pos = parser->pos;

    if (ciwic_parser_specifier_qualifier_list(parser, &specifiers)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_struct_declarator_list(parser, &decl_list)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ";")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_struct_list_inner(parser, &rest)) {
        list->specifiers = specifiers;
        list->declarator_list = decl_list;
        list->rest = NULL;
        return 0;
    }

    list->specifiers = specifiers;
    list->declarator_list = decl_list;
    list->rest = malloc(sizeof(ciwic_struct_list));
    *list->rest = rest;
    return 0;
}

int ciwic_parser_struct_list(ciwic_parser *parser, ciwic_struct_list *list) {
    ciwic_struct_list inner;
    int pos = parser->pos;

    if (ciwic_parser_punctuation(parser, "{")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_struct_list_inner(parser, &inner)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, "}")) {
        parser->pos = pos;
        return 1;
    }

    *list = inner;
    return 0;
}

int ciwic_parser_declaration_specifiers(ciwic_parser *parser, ciwic_declaration_specifiers* specifiers) {
    ciwic_storage_class storage_class;
    ciwic_function_specifier function_specifier;
    ciwic_type_qualifier type_qualifier;
    ciwic_type_prim prim_type;
    ciwic_declaration_specifiers inner = {
        .storage_class = 0,
        .func_specifiers = 0,
        .type_qualifiers = 0,
        .type_spec = ciwic_type_spec_none
    };
    int is_struct;

    int pos = parser->pos;

    if (!ciwic_parser_storage_class(parser, &storage_class)) {
        ciwic_parser_declaration_specifiers(parser, &inner);
        inner.storage_class |= storage_class;
        *specifiers = inner;
        return 0;
    }

    if (!ciwic_parser_function_specifier(parser, &function_specifier)) {
        ciwic_parser_declaration_specifiers(parser, &inner);
        inner.func_specifiers |= function_specifier;
        *specifiers = inner;
        return 0;
    }

    if (!ciwic_parser_type_qualifier(parser, &type_qualifier)) {
        ciwic_parser_declaration_specifiers(parser, &inner);
        inner.type_qualifiers |= type_qualifier;
        *specifiers = inner;
        return 0;
    }

    if (!ciwic_parser_type_prim(parser, &prim_type)) {
        ciwic_parser_declaration_specifiers(parser, &inner);
        if (inner.type_spec == ciwic_type_spec_prim) {
            if (prim_type == ciwic_type_long && inner.prim_type & ciwic_type_long) {
                if (inner.prim_type & ciwic_type_long_long) {
                    // We cannot have more than two longs
                    parser->pos = pos;
                    return 1;
                }
                inner.prim_type |= ciwic_type_long_long;
            } else if (inner.prim_type & prim_type) {
                // We cannot have more of one of any other type specifiers
                parser->pos = pos;
                return 1;
            } else {
                inner.prim_type |= prim_type;
            }
        } else if (inner.type_spec == ciwic_type_spec_none) {
            inner.type_spec = ciwic_type_spec_prim;
            inner.prim_type |= prim_type;
        } else {
            // We cannot have a primitive type at the same time as another type
            // specifier
            parser->pos = pos;
            return 1;
        }

        *specifiers = inner;
        return 0;
    }

    if (!ciwic_parser_keyword(parser, "enum")) {
        string identifier;
        ciwic_enum_list decl;

        int ident_res = ciwic_parser_identifier(parser, &identifier);
        int decl_res  = ciwic_parser_enum_list(parser, &decl);

        if (!ident_res && !decl_res) {
            parser->pos = pos;
            return 1;
        }

        ciwic_parser_declaration_specifiers(parser, &inner);

        if (inner.type_spec != ciwic_type_spec_none) {
            parser->pos = pos;
            return 1;
        }

        inner.type_spec = ciwic_type_spec_enum;
        if (!ident_res) {
            inner.enum_.identifier  = malloc(sizeof(string));
            *inner.enum_.identifier = identifier;
        }
        if (!decl_res) {
            inner.enum_.decl  = malloc(sizeof(ciwic_enum_list));
            *inner.enum_.decl = decl;
        }

        *specifiers = inner;
        return 0;
    }

    if ((is_struct = !ciwic_parser_keyword(parser, "struct")) || !ciwic_parser_keyword(parser, "union")) {
        string identifier;
        ciwic_struct_list decl;

        int ident_res = ciwic_parser_identifier(parser, &identifier);
        int decl_res = ciwic_parser_struct_list(parser, &decl);

        if (ident_res && decl_res) {
            parser->pos = pos;
            return 1;
        }

        ciwic_parser_declaration_specifiers(parser, &inner);

        if (inner.type_spec != ciwic_type_spec_none) {
            parser->pos = pos;
            return 1;
        }

        if (is_struct)
            inner.type_spec = ciwic_type_spec_struct;
        else 
            inner.type_spec = ciwic_type_spec_union;

        if (!ident_res) {
            inner.struct_or_union.identifier  = malloc(sizeof(string));
            *inner.struct_or_union.identifier = identifier;
        }
        if (!decl_res) {
            inner.struct_or_union.decl  = malloc(sizeof(ciwic_struct_list));
            *inner.struct_or_union.decl = decl;
        }

        *specifiers = inner;
        return 0;
    }

    // TODO: typedef name

    return 1;
}

int ciwic_parser_type_qualifiers(ciwic_parser *parser, int *type_qualifiers) {
    ciwic_type_qualifier type_qual;

    if (ciwic_parser_type_qualifier(parser, &type_qual)) {
        return 1;
    }

    if (ciwic_parser_type_qualifiers(parser, type_qualifiers)) {
        *type_qualifiers = type_qual;
        return 0;
    }

    *type_qualifiers |= type_qual;
    return 0;
}

int ciwic_parser_param_list(ciwic_parser *parser, ciwic_param_list *params) {
    ciwic_declaration_specifiers specifiers;
    ciwic_declarator declarator;
    ciwic_param_list rest;
    int has_rest = 0;

    int pos = parser->pos;

    if (ciwic_parser_declaration_specifiers(parser, &specifiers)) {
        parser->pos = pos;
        return 1;
    }

    int has_declarator = !ciwic_parser_declarator(parser, NULL, &declarator);

    int last_pos = parser->pos;
    if (!ciwic_parser_punctuation(parser, ",")) {
        if (ciwic_parser_param_list(parser, &rest)) {
            parser->pos = last_pos;
        } else {
            has_rest = 1;
        }
    }

    params->specifiers = specifiers;

    if (has_declarator) {
        params->declarator = malloc(sizeof(ciwic_declarator));
        *params->declarator = declarator;
    } else {
        params->declarator = NULL;
    }

    if (has_rest) {
        params->rest = malloc(sizeof(ciwic_param_list));
        *params->rest = rest;
    } else {
        params->rest = NULL;
    }

    return 0;
}

int ciwic_parser_declarator(ciwic_parser *parser, ciwic_declarator *prev, ciwic_declarator *decl) {
    ciwic_declarator outer, inner;
    string ident;

    int pos = parser->pos;

    if (prev == NULL) {
        if (!ciwic_parser_punctuation(parser, "*")) {
            int pointer_qualifiers = 0;
            ciwic_parser_type_qualifiers(parser, &pointer_qualifiers);

            int has_inner = !ciwic_parser_declarator(parser, NULL, &inner);

            outer.type = ciwic_declarator_pointer;
            if (has_inner) {
                outer.inner = malloc(sizeof(ciwic_declarator));
                *outer.inner = inner;
            } else {
                outer.inner = NULL;
            }
            outer.pointer_qualifiers = pointer_qualifiers;

            if (ciwic_parser_declarator(parser, &outer, decl)) {
                parser->pos = pos;
                return 1;
            }

            return 0;
        }

        if (!ciwic_parser_identifier(parser, &ident)) {
            outer.type = ciwic_declarator_identifier;
            outer.inner = NULL;
            outer.ident = ident;

            if (ciwic_parser_declarator(parser, &outer, decl)) {
                parser->pos = pos;
                return 1;
            }

            return 0;
        }
    }

    if (!ciwic_parser_punctuation(parser, "[")) {
        int is_static = 0;
        int is_var_len = 0;
        int type_qualifiers = 0;
        int has_expr = 0;
        ciwic_expr expr;

        if (!ciwic_parser_keyword(parser, "static")) {
            is_static = 1;
        }

        ciwic_parser_type_qualifiers(parser, &type_qualifiers);

        if (!is_static && !ciwic_parser_keyword(parser, "static")) {
            is_static = 1;
        }

        if (!is_static && !ciwic_parser_punctuation(parser, "*")) {
            is_var_len = 1;
        }

        if (!is_var_len && !ciwic_parser_expr(parser, &expr)) {
            has_expr = 1;
        }

        if (ciwic_parser_punctuation(parser, "]")) {
            parser->pos = pos;
            return 1;
        }

        inner.type = ciwic_declarator_array;
        if (prev != NULL) {
            inner.inner = malloc(sizeof(ciwic_declarator));
            *inner.inner = *prev;
        } else {
            inner.inner = NULL;
        }
        inner.array.is_static = is_static;
        inner.array.is_var_len = is_var_len;
        inner.array.type_qualifiers = type_qualifiers;
        if (has_expr) {
            inner.array.expr = malloc(sizeof(ciwic_expr));
            *inner.array.expr = expr;
        } else {
            inner.array.expr = NULL;
        }

        if (ciwic_parser_declarator(parser, &inner, decl)) {
            parser->pos = pos;
            return 1;
        }

        return 0;
    }

    if (!ciwic_parser_punctuation(parser, "(")) {
        if (!ciwic_parser_declarator(parser, NULL, &inner)) {
            if (ciwic_parser_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_declarator(parser, &inner, decl)) {
                parser->pos = pos;
                return 1;
            }

            return 0;
        }

        ciwic_param_list params;

        int has_params = !ciwic_parser_param_list(parser, &params);
        int has_ellipsis = 0;

        if (!ciwic_parser_punctuation(parser, ",")) {
            if (ciwic_parser_punctuation(parser, "...")) {
                parser->pos = pos;
                return 1;
            } else {
                has_ellipsis = 1;
            }
        }

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        inner.type = ciwic_declarator_func;
        if (prev != NULL) {
            inner.inner = malloc(sizeof(ciwic_declarator));
            *inner.inner = *prev;
        } else {
            inner.inner = NULL;
        }
        inner.func.has_ellipsis = has_ellipsis;
        
        if (has_params) {
            inner.func.param_list = malloc(sizeof(ciwic_param_list));
            *inner.func.param_list = params;
        } else {
            inner.func.param_list = NULL;
        }

        if (ciwic_parser_declarator(parser, &inner, decl)) {
            parser->pos = pos;
            return 1;
        }

        return 0;
    }

    if (prev != NULL) {
        *decl = *prev;

        return 0;
    }

    return 1;
}

int ciwic_parser_type_name(ciwic_parser *parser, ciwic_type_name *name) {
    ciwic_declaration_specifiers specifiers;
    ciwic_declarator declarator;

    int pos = parser->pos;

    if (ciwic_parser_declaration_specifiers(parser, &specifiers)) {
        parser->pos = pos;
        return 1;
    }

    int has_declarator = !ciwic_parser_declarator(parser, NULL, &declarator);

    if (has_declarator && !ciwic_declarator_is_abstract(&declarator)) {
        parser->pos = pos;
        return 1;
    }

    name->specifiers = specifiers;

    if (has_declarator) {
        name->declarator = malloc(sizeof(ciwic_declarator));
        *name->declarator = declarator;
    } else {
        name->declarator = NULL;
    }

    return 0;
}

int ciwic_parser_designation(ciwic_parser *parser, ciwic_designator_list *designation) {
    ciwic_designator_list rest;
    int pos = parser->pos;

    if (!ciwic_parser_punctuation(parser, "[")) {
        ciwic_expr expr;
        if (ciwic_parser_const_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, "]")) {
            parser->pos = pos;
            return 1;
        }

        if (!ciwic_parser_designation(parser, &rest)) {
            designation->rest = malloc(sizeof(ciwic_designator_list));
            *designation->rest = rest;
        } else {
            designation->rest = NULL;
        }

        designation->type = ciwic_designator_expr;
        designation->expr = expr;
        return 0;
    }

    if (!ciwic_parser_punctuation(parser, ".")) {
        string ident;

        if (ciwic_parser_identifier(parser, &ident)) {
            parser->pos = pos;
            return 1;
        }

        if (!ciwic_parser_designation(parser, &rest)) {
            designation->rest = malloc(sizeof(ciwic_designator_list));
            *designation->rest = rest;
        } else {
            designation->rest = NULL;
        }

        designation->type = ciwic_designator_ident;
        designation->ident = ident;
        return 0;
    }

    return 1;
}

int ciwic_parser_initializer(ciwic_parser *parser, ciwic_initializer *init) {
    ciwic_expr expr;
    ciwic_initializer_list list;

    int pos = parser->pos;

    if (!ciwic_parser_assignment_expr(parser, &expr)) {
        init->type = ciwic_initializer_init_expr;
        init->expr = expr;
        return 0;
    }

    if (!ciwic_parser_punctuation(parser, "{")) {
        if (ciwic_parser_initializer_list(parser, &list)) {
            parser->pos = pos;
            return 1;
        }
        ciwic_parser_punctuation(parser, ",");

        if (ciwic_parser_punctuation(parser, "}")) {
            parser->pos = pos;
            return 1;
        }

        init->type = ciwic_initializer_init_list;
        init->list = list;
        return 0;
    }

    return 1;
}

int ciwic_parser_initializer_list(ciwic_parser *parser, ciwic_initializer_list *list) {
    ciwic_designator_list designation;
    ciwic_initializer initializer;
    ciwic_initializer_list rest;
    int has_rest = 0;

    int pos = parser->pos;

    int has_designator = !ciwic_parser_designation(parser, &designation);

    if (has_designator && ciwic_parser_punctuation(parser, "=")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_initializer(parser, &initializer)) {
        parser->pos = pos;
        return 1;
    }

    int last_pos = parser->pos;

    if (!ciwic_parser_punctuation(parser, ",")) {
        has_rest = !ciwic_parser_initializer_list(parser, &rest);
        if (!has_rest) {
            parser->pos = last_pos;
        }
    }

    list->initializer = malloc(sizeof(ciwic_initializer));
    *list->initializer = initializer;

    if (has_designator) {
        list->designation = malloc(sizeof(ciwic_designator_list));
        *list->designation = designation;
    } else {
        list->designation = NULL;
    }

    if (has_rest) {
        list->rest = malloc(sizeof(ciwic_initializer_list));
        *list->rest = rest;
    } else {
        list->rest = NULL;
    }

    return 0;
}

int ciwic_parser_init_declarator_list(ciwic_parser *parser, ciwic_init_declarator_list *list) {
    ciwic_declarator declarator;
    ciwic_initializer initializer;
    ciwic_init_declarator_list rest;

    int has_initializer = 0;
    int has_rest = 0;

    int pos = parser->pos;

    if (ciwic_parser_declarator(parser, NULL, &declarator)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_declarator_is_abstract(&declarator)) {
        parser->pos = pos;
        return 1;
    }

    if (!ciwic_parser_punctuation(parser, "=")) {
        has_initializer = !ciwic_parser_initializer(parser, &initializer);
    }

    if (!ciwic_parser_punctuation(parser, ",")) {
        if (!(has_rest = !ciwic_parser_init_declarator_list(parser, &rest))) {
            parser->pos = pos;
            return 1;
        }
    }

    list->declarator = declarator;

    if (has_initializer) {
        list->initializer = malloc(sizeof(ciwic_initializer));
        *list->initializer = initializer;
    } else {
        list->initializer = NULL;
    }

    if (has_rest) {
        list->rest = malloc(sizeof(ciwic_init_declarator_list));
        *list->rest = rest;
    } else {
        list->rest = NULL;
    }

    return 0;
}

int ciwic_parser_declaration(ciwic_parser *parser, ciwic_declaration *decl) {
    ciwic_declaration_specifiers specifiers;
    ciwic_init_declarator_list list;

    int pos = parser->pos;

    if (ciwic_parser_declaration_specifiers(parser, &specifiers)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_init_declarator_list(parser, &list)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ";")) {
        parser->pos = pos;
        return 1;
    }

    decl->specifiers = specifiers;
    decl->list = list;
    return 0;
}


int ciwic_parser_labeled_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    string ident;
    ciwic_expr expr;
    ciwic_statement rest;

    int pos = parser->pos;

    if (!ciwic_parser_identifier(parser, &ident)) {
        if (ciwic_parser_punctuation(parser, ":")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &rest)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_label;
        stmt->labeled.label_ident = ident;
        stmt->labeled.stmt = malloc(sizeof(ciwic_statement));
        *stmt->labeled.stmt = rest;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "case")) {
        if (ciwic_parser_const_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_punctuation(parser, ":")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &rest)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_case;
        stmt->labeled.case_expr = expr;
        stmt->labeled.stmt = malloc(sizeof(ciwic_statement));
        *stmt->labeled.stmt = rest;
        return 0;
    }
    if (!ciwic_parser_keyword(parser, "default")) {
        if (ciwic_parser_punctuation(parser, ":")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &rest)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_default;
        stmt->labeled.case_expr = expr;
        stmt->labeled.stmt = malloc(sizeof(ciwic_statement));
        *stmt->labeled.stmt = rest;
        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_block_list(ciwic_parser *parser, ciwic_statement *stmt) {
    ciwic_statement head;
    ciwic_statement rest;

    int pos = parser->pos;

    if (ciwic_parser_statement(parser, &head)) {
        parser->pos = pos;
        return 1;
    }

    int has_rest = !ciwic_parser_block_list(parser, &rest);

    stmt->type = ciwic_statement_block;

    stmt->block.head = malloc(sizeof(ciwic_statement));
    *stmt->block.head = head;

    if (has_rest) {
        stmt->block.rest = malloc(sizeof(ciwic_statement));
        *stmt->block.rest = rest;
    } else {
        stmt->block.rest = NULL;
    }

    return 0;
}

int ciwic_parser_compound_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    ciwic_statement inner;
    int pos = parser->pos;

    if (ciwic_parser_punctuation(parser, "{")) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_block_list(parser, &inner)) {
        inner.type = ciwic_statement_null;
    }

    if (ciwic_parser_punctuation(parser, "}")) {
        parser->pos = pos;
        return 1;
    }

    *stmt = inner;
    return 0;
}

int ciwic_parser_expr_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    ciwic_expr expr;

    int pos = parser->pos;

    if (ciwic_parser_expr(parser, &expr)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_punctuation(parser, ";")) {
        parser->pos = pos;
        return 1;
    }

    stmt->type = ciwic_statement_expr;
    stmt->expr = expr;
    return 0;
}

int ciwic_parser_selection_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    ciwic_expr expr;
    ciwic_statement fst_stmt, else_stmt;

    int pos = parser->pos;

    if (!ciwic_parser_keyword(parser, "if")) {
        if (ciwic_parser_punctuation(parser, "(")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &fst_stmt)) {
            parser->pos = pos;
            return 1;
        }

        int has_else = !ciwic_parser_keyword(parser, "else");

        if (has_else && ciwic_parser_statement(parser, &else_stmt)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_if;
        stmt->if_stmt.expr = expr;
        stmt->if_stmt.if_then = malloc(sizeof(ciwic_statement));
        *stmt->if_stmt.if_then = fst_stmt;

        if (has_else) {
            stmt->if_stmt.if_else = malloc(sizeof(ciwic_statement));
            *stmt->if_stmt.if_else = else_stmt;
        } else {
            stmt->if_stmt.if_else = NULL;
        }

        return 0;
    }

    if (!ciwic_parser_keyword(parser, "switch")) {
        if (ciwic_parser_punctuation(parser, "(")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &fst_stmt)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_switch;
        stmt->switch_stmt.expr = expr;
        stmt->switch_stmt.stmt = malloc(sizeof(ciwic_statement));
        *stmt->switch_stmt.stmt = fst_stmt;
        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_iteration_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    ciwic_declaration pre_decl;
    ciwic_expr expr, pre_expr, test_expr, post_expr;
    ciwic_statement inner_stmt;

    int pos = parser->pos;

    if (!ciwic_parser_keyword(parser, "while")) {
        if (ciwic_parser_punctuation(parser, "(")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &inner_stmt)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_while;
        stmt->while_stmt.expr = expr;
        stmt->while_stmt.stmt = malloc(sizeof(ciwic_statement));
        *stmt->while_stmt.stmt = inner_stmt;

        return 0;
    }

    if (!ciwic_parser_keyword(parser, "do")) {
        if (ciwic_parser_statement(parser, &inner_stmt)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_keyword(parser, "while")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, "(")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_expr(parser, &expr)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_do_while;
        stmt->while_stmt.expr = expr;
        stmt->while_stmt.stmt = malloc(sizeof(ciwic_statement));
        *stmt->while_stmt.stmt = inner_stmt;

        return 0;
    }

    if (!ciwic_parser_keyword(parser, "for")) {
        if (ciwic_parser_punctuation(parser, "(")) {
            parser->pos = pos;
            return 1;
        }

        int has_pre_decl = !ciwic_parser_declaration(parser, &pre_decl);

        int has_pre_expr = !has_pre_decl && ciwic_parser_expr(parser, &pre_expr);

        if (!has_pre_decl && ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        int has_test_expr = !ciwic_parser_expr(parser, &test_expr);

        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        int has_post_expr = !ciwic_parser_expr(parser, &post_expr);

        if (ciwic_parser_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_statement(parser, &inner_stmt)) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_for;

        if (has_pre_decl) {
            stmt->for_stmt.pre_decl = malloc(sizeof(ciwic_declaration));
            *stmt->for_stmt.pre_decl = pre_decl;
        } else {
            stmt->for_stmt.pre_decl = NULL;
        }

        if (has_pre_expr) {
            stmt->for_stmt.pre_expr = malloc(sizeof(ciwic_expr));
            *stmt->for_stmt.pre_expr = pre_expr;
        } else {
            stmt->for_stmt.pre_expr = NULL;
        }

        if (has_test_expr) {
            stmt->for_stmt.test_expr = malloc(sizeof(ciwic_expr));
            *stmt->for_stmt.test_expr = test_expr;
        } else {
            stmt->for_stmt.test_expr = NULL;
        }

        if (has_post_expr) {
            stmt->for_stmt.post_expr = malloc(sizeof(ciwic_expr));
            *stmt->for_stmt.post_expr = post_expr;
        } else {
            stmt->for_stmt.post_expr = NULL;
        }

        stmt->for_stmt.stmt = malloc(sizeof(ciwic_statement));
        *stmt->for_stmt.stmt = inner_stmt;

        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_jump_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    int pos = parser->pos;

    if (!ciwic_parser_keyword(parser, "goto")) {
        string ident;
        if (ciwic_parser_identifier(parser, &ident)) {
            parser->pos = pos;
            return 1;
        }

        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_goto;
        stmt->goto_ident = ident;
        return 0;
    }

    if (!ciwic_parser_keyword(parser, "continue")) {
        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_continue;
        return 0;
    }

    if (!ciwic_parser_keyword(parser, "break")) {
        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_break;
        return 0;
    }

    if (!ciwic_parser_keyword(parser, "return")) {
        ciwic_expr expr;

        int has_expr = !ciwic_parser_expr(parser, &expr);

        if (ciwic_parser_punctuation(parser, ";")) {
            parser->pos = pos;
            return 1;
        }

        stmt->type = ciwic_statement_return;

        if (has_expr) {
            stmt->return_expr = malloc(sizeof(ciwic_expr));
            *stmt->return_expr = expr;
        } else {
            stmt->return_expr = NULL;
        }

        return 0;
    }

    parser->pos = pos;
    return 1;
}

int ciwic_parser_statement(ciwic_parser *parser, ciwic_statement *stmt) {
    if (!ciwic_parser_labeled_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_compound_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_expr_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_selection_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_iteration_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_jump_statement(parser, stmt)) {
        return 0;
    }
    if (!ciwic_parser_punctuation(parser, ";")) {
        stmt->type = ciwic_statement_null;
        return 0;
    }

    return 1;
}

int ciwic_parser_declaration_list(ciwic_parser *parser, ciwic_declaration_list *list) {
    ciwic_declaration decl;
    ciwic_declaration_list rest;

    int pos = parser->pos;

    if (ciwic_parser_declaration(parser, &decl)) {
        parser->pos = pos;
        return 1;
    }

    int has_rest = !ciwic_parser_declaration_list(parser, &rest);

    list->head = decl;

    if (has_rest) {
        list->rest = malloc(sizeof(ciwic_declaration_list));
        *list->rest = rest;
    } else {
        list->rest = NULL;
    }

    return 0;
}

int ciwic_parser_func_definition(ciwic_parser *parser, ciwic_func_definition *def) {
    ciwic_declaration_specifiers specifiers;
    ciwic_declarator declarator;
    ciwic_declaration_list decl_list;
    ciwic_statement stmt;

    int pos = parser->pos;

    if (ciwic_parser_declaration_specifiers(parser, &specifiers)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_declarator(parser, NULL, &declarator)) {
        parser->pos = pos;
        return 1;
    }

    int has_decl_list = !ciwic_parser_declaration_list(parser, &decl_list);

    if (ciwic_parser_compound_statement(parser, &stmt)) {
        parser->pos = pos;
        return 1;
    }

    def->specifiers = specifiers;
    def->declarator = declarator;

    if (has_decl_list) {
        def->decl_list = malloc(sizeof(ciwic_declaration_list));
        *def->decl_list = decl_list;
    } else {
        def->decl_list = NULL;
    }

    def->statement = stmt;

    return 0;
}

int ciwic_parser_translation_unit(ciwic_parser *parser, ciwic_translation_unit *translation_unit) {
    ciwic_func_definition func;
    ciwic_declaration decl;
    ciwic_translation_unit rest;

    int pos = parser->pos;

    if (!ciwic_parser_func_definition(parser, &func)) {
        int has_rest = !ciwic_parser_translation_unit(parser, &rest);

        translation_unit->def_type = ciwic_definition_func;
        translation_unit->func = func;

        if (has_rest) {
            translation_unit->rest = malloc(sizeof(ciwic_translation_unit));
            *translation_unit->rest = rest;
        } else {
            translation_unit->rest = NULL;
        }

        return 0;
    }

    if (!ciwic_parser_declaration(parser, &decl)) {
        int has_rest = !ciwic_parser_translation_unit(parser, &rest);

        translation_unit->def_type = ciwic_definition_decl;
        translation_unit->decl = decl;

        if (has_rest) {
            translation_unit->rest = malloc(sizeof(ciwic_translation_unit));
            *translation_unit->rest = rest;
        } else {
            translation_unit->rest = NULL;
        }

        return 0;
    }

    parser->pos = pos;
    return 1;
}
