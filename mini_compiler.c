/*
 * ============================================================
 *  mini_compiler.c  –  A tiny expression compiler in C (C11)
 *
 *  What this teaches:
 *    1. Lexing  / Tokenisation  – turning raw text into tokens
 *    2. Parsing / AST building  – understanding structure
 *    3. Evaluation              – walking the tree & computing
 *
 *  Supports: integers, +  -  *  /  %  ( )  and variables
 *
 *  Compile:  gcc -std=c11 -Wall -Wextra -o mini_compiler mini_compiler.c
 *  Run:      ./mini_compiler
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── 1. TOKEN TYPES ────────────────────────────────────────── */
typedef enum {
    TOK_NUM,   /* 42        */
    TOK_PLUS,  /* +         */
    TOK_MINUS, /* -         */
    TOK_MUL,   /* *         */
    TOK_DIV,   /* /         */
    TOK_MOD,   /* %         */
    TOK_LPAREN,/* (         */
    TOK_RPAREN,/* )         */
    TOK_VAR,   /* x, y, z  */
    TOK_EOF    /* end       */
} TokenType;

typedef struct {
    TokenType type;
    int       value;   /* used when type == TOK_NUM */
    char      name;    /* used when type == TOK_VAR */
} Token;

/* ── 2. LEXER ───────────────────────────────────────────────
   Reads a string character by character and emits tokens.    */
typedef struct {
    const char *src;
    int         pos;
} Lexer;

static Token next_token(Lexer *lex) {
    /* skip whitespace */
    while (lex->src[lex->pos] == ' ' || lex->src[lex->pos] == '\t')
        lex->pos++;

    char c = lex->src[lex->pos];

    if (c == '\0') return (Token){ TOK_EOF, 0, '\0' };

    if (isdigit(c)) {
        int val = 0;
        while (isdigit(lex->src[lex->pos]))
            val = val * 10 + (lex->src[lex->pos++] - '0');
        return (Token){ TOK_NUM, val, '\0' };
    }

    if (isalpha(c) && c != 'e') {          /* single-letter variable */
        lex->pos++;
        return (Token){ TOK_VAR, 0, c };
    }

    lex->pos++;
    switch (c) {
        case '+': return (Token){ TOK_PLUS,   0, '\0' };
        case '-': return (Token){ TOK_MINUS,  0, '\0' };
        case '*': return (Token){ TOK_MUL,    0, '\0' };
        case '/': return (Token){ TOK_DIV,    0, '\0' };
        case '%': return (Token){ TOK_MOD,    0, '\0' };
        case '(': return (Token){ TOK_LPAREN, 0, '\0' };
        case ')': return (Token){ TOK_RPAREN, 0, '\0' };
        default:
            fprintf(stderr, "Unknown character: '%c'\n", c);
            exit(1);
    }
}

/* ── 3. ABSTRACT SYNTAX TREE (AST) NODE ────────────────────
   Each node is either a number, a variable, or a binary op. */
typedef enum { NODE_NUM, NODE_VAR, NODE_BIN } NodeKind;

typedef struct Node {
    NodeKind    kind;
    int         num_val;   /* NODE_NUM */
    char        var_name;  /* NODE_VAR */
    char        op;        /* NODE_BIN operator: + - * / % */
    struct Node *left;     /* NODE_BIN children */
    struct Node *right;
} Node;

static Node *make_num(int v) {
    Node *n = malloc(sizeof(Node));
    n->kind = NODE_NUM; n->num_val = v;
    n->left = n->right = NULL;
    return n;
}
static Node *make_var(char name) {
    Node *n = malloc(sizeof(Node));
    n->kind = NODE_VAR; n->var_name = name;
    n->left = n->right = NULL;
    return n;
}
static Node *make_bin(char op, Node *l, Node *r) {
    Node *n = malloc(sizeof(Node));
    n->kind = NODE_BIN; n->op = op;
    n->left = l; n->right = r;
    return n;
}

/* ── 4. PARSER ──────────────────────────────────────────────
   Recursive-descent parser implementing operator precedence:
     expr   → term   (('+' | '-') term)*
     term   → factor (('*' | '/' | '%') factor)*
     factor → NUM | VAR | '(' expr ')'                        */
typedef struct {
    Lexer lex;
    Token cur;
} Parser;

static void advance(Parser *p)      { p->cur = next_token(&p->lex); }
static Node *parse_expr(Parser *p);

static Node *parse_factor(Parser *p) {
    if (p->cur.type == TOK_NUM) {
        Node *n = make_num(p->cur.value);
        advance(p);
        return n;
    }
    if (p->cur.type == TOK_VAR) {
        Node *n = make_var(p->cur.name);
        advance(p);
        return n;
    }
    if (p->cur.type == TOK_LPAREN) {
        advance(p);
        Node *n = parse_expr(p);
        if (p->cur.type != TOK_RPAREN) { fputs("Expected ')'\n", stderr); exit(1); }
        advance(p);
        return n;
    }
    /* unary minus */
    if (p->cur.type == TOK_MINUS) {
        advance(p);
        return make_bin('-', make_num(0), parse_factor(p));
    }
    fprintf(stderr, "Unexpected token type %d\n", p->cur.type);
    exit(1);
}

static Node *parse_term(Parser *p) {
    Node *left = parse_factor(p);
    while (p->cur.type == TOK_MUL ||
           p->cur.type == TOK_DIV ||
           p->cur.type == TOK_MOD) {
        char op = (p->cur.type == TOK_MUL) ? '*' :
                  (p->cur.type == TOK_DIV) ? '/' : '%';
        advance(p);
        left = make_bin(op, left, parse_factor(p));
    }
    return left;
}

static Node *parse_expr(Parser *p) {
    Node *left = parse_term(p);
    while (p->cur.type == TOK_PLUS || p->cur.type == TOK_MINUS) {
        char op = (p->cur.type == TOK_PLUS) ? '+' : '-';
        advance(p);
        left = make_bin(op, left, parse_term(p));
    }
    return left;
}

/* ── 5. VARIABLE STORE ──────────────────────────────────── */
static int var_store[26];   /* a–z */

/* ── 6. EVALUATOR ───────────────────────────────────────────
   Walks the AST recursively and computes the result.          */
static int eval(const Node *n) {
    if (n->kind == NODE_NUM) return n->num_val;
    if (n->kind == NODE_VAR) return var_store[n->var_name - 'a'];
    int l = eval(n->left), r = eval(n->right);
    switch (n->op) {
        case '+': return l + r;
        case '-': return l - r;
        case '*': return l * r;
        case '/':
            if (r == 0) { fputs("Division by zero!\n", stderr); exit(1); }
            return l / r;
        case '%':
            if (r == 0) { fputs("Modulo by zero!\n", stderr); exit(1); }
            return l % r;
        default: exit(1);
    }
}

/* ── 7. AST PRETTY-PRINTER ──────────────────────────────── */
static void print_ast(const Node *n, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    if (n->kind == NODE_NUM) { printf("NUM(%d)\n", n->num_val); return; }
    if (n->kind == NODE_VAR) { printf("VAR(%c)\n", n->var_name); return; }
    printf("OP('%c')\n", n->op);
    print_ast(n->left,  indent + 1);
    print_ast(n->right, indent + 1);
}

/* ── 8. FREE AST ────────────────────────────────────────── */
static void free_ast(Node *n) {
    if (!n) return;
    free_ast(n->left);
    free_ast(n->right);
    free(n);
}

/* ── 9. DRIVER ──────────────────────────────────────────── */
static Node *compile(const char *src) {
    Parser p;
    p.lex.src = src;
    p.lex.pos = 0;
    advance(&p);           /* prime the first token */
    Node *ast = parse_expr(&p);
    if (p.cur.type != TOK_EOF) { fputs("Trailing input\n", stderr); exit(1); }
    return ast;
}

int main(void) {
    printf("==============================================\n");
    printf("   Mini Compiler  –  Expression Evaluator\n");
    printf("==============================================\n\n");

    /* pre-load some variables */
    var_store['x' - 'a'] = 10;
    var_store['y' - 'a'] = 3;
    var_store['z' - 'a'] = 7;
    printf("Variables: x = %d,  y = %d,  z = %d\n\n",
           var_store['x'-'a'], var_store['y'-'a'], var_store['z'-'a']);

    const char *tests[] = {
        "3 + 5 * 2",
        "(3 + 5) * 2",
        "10 / 2 - 1",
        "x * y + z",
        "(x + z) % y",
        "100 - x * (y + 2)",
        NULL
    };

    for (int i = 0; tests[i]; i++) {
        printf("────────────────────────────────────────\n");
        printf("Expression : %s\n", tests[i]);

        Node *ast = compile(tests[i]);

        printf("AST        :\n");
        print_ast(ast, 2);

        int result = eval(ast);
        printf("Result     : %d\n\n", result);

        free_ast(ast);
    }

    printf("==============================================\n");
    printf("Each expression above went through:\n");
    printf("  1. Lexing   – text split into tokens\n");
    printf("  2. Parsing  – tokens shaped into an AST\n");
    printf("  3. Eval     – AST walked to get the answer\n");
    printf("That is the core of every real compiler.\n");
    printf("==============================================\n");

    return 0;
}
