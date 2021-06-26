#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <parser.h>
#include <ast.h>

/* Based on C99 standard N1256 draft from:
 * http://www.open-std.org/jtc1/sc22/WG14/www/docs/n1256.pdf
 */

const int ciwic_parser_keywords_len = 32;
const char* ciwic_parser_keywords[32] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while" };

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

int ciwic_parser_match_punctuation(ciwic_parser *parser, const char* punct) {
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

    if (!ciwic_parser_match_punctuation(parser, "(")) {
        if (ciwic_parser_expr(parser, res)) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_match_punctuation(parser, ")")) {
            parser->pos = pos;
            return 1;
        }
        printf("found primary parens\n");
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

        if (!ciwic_parser_match_punctuation(parser, "(")) {
            ciwic_type_name type_name;
            ciwic_initializer_list initializer_list;
            if (ciwic_parser_type_name(parser, &type_name)) {
                parser->pos = pos;
                return 1;
            }
            
            if (ciwic_parser_match_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_match_punctuation(parser, "{")) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_initializer_list(parser, &initializer_list)) {
                parser->pos = pos;
                return 1;
            }

            ciwic_parser_match_punctuation(parser, ",");

            if (ciwic_parser_match_punctuation(parser, "}")) {
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
        if (!ciwic_parser_match_punctuation(parser, "[")) {
            ciwic_expr subscript, expr;
            if (ciwic_parser_expr(parser, &expr)) {
                parser->pos = pos;
                return 1;
            }

            if (ciwic_parser_match_punctuation(parser, "]")) {
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

        if (!ciwic_parser_match_punctuation(parser, "(")) {
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

            if (ciwic_parser_match_punctuation(parser, ")")) {
                parser->pos = pos;
                return 1;
            }

            return ciwic_parser_postfix_expr(parser, &call, res);
        }

        if (!ciwic_parser_match_punctuation(parser, ".")) {
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

        if (!ciwic_parser_match_punctuation(parser, "->")) {
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

        if (!ciwic_parser_match_punctuation(parser, "++")) {
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

        if (!ciwic_parser_match_punctuation(parser, "--")) {
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

    if (ciwic_parser_match_punctuation(parser, ",")) {
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
        if (!ciwic_parser_match_punctuation(parser, unary_ops_punct[i])) {
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

        if (!ciwic_parser_match_punctuation(parser, "(")) {
            if (ciwic_parser_type_name(parser, &type_name)) {
                parser->pos = pos;
                return 1;
            }
            if (ciwic_parser_match_punctuation(parser, ")")) {
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

    if (!ciwic_parser_match_punctuation(parser, "(")) {
        if (ciwic_parser_type_name(parser, &type_name)) {
            parser->pos = pos;
            return 1;
        }
        if (ciwic_parser_match_punctuation(parser, ")")) {
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
            if (!ciwic_parser_match_punctuation(parser, op_table_punct[level][i])) {
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
    ciwic_expr left, right;

    int pos = parser->pos;

    if (cond == NULL) {
        if (ciwic_parser_logical_or_expr(parser, cond)) {
            parser->pos = pos;
            return 1;
        }
    }

    if (ciwic_parser_match_punctuation(parser, "?")) {
        *res = *cond;
        return 0;
    }

    if (ciwic_parser_expr(parser, &left)) {
        parser->pos = pos;
        return 1;
    }

    if (ciwic_parser_match_punctuation(parser, ":")) {
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
        parser->pos = pos;
        return 1;
    }

    for (int i = 0; i < 11; i++) {
        if (!ciwic_parser_match_punctuation(parser, op_table_punct[i])) {
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

    if (ciwic_parser_match_punctuation(parser, ",")) {
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

int ciwic_parser_initializer_list(ciwic_parser *parser, ciwic_initializer_list *list) {
    // TODO: implement this
    return 1;
}

int ciwic_parser_type_name(ciwic_parser *parser, ciwic_type_name *name) {
    // TODO: implement this
    return 1;
}
