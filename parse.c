/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

static TokenType token; /* holds current token */

/* function prototypes for recursive calls */
static TreeNode *stmt_sequence(void);
static TreeNode *statement(void);
static TreeNode *if_stmt(void);
static TreeNode *repeat_stmt(void);
static TreeNode *assign_stmt(void);
static TreeNode *read_stmt(void);
static TreeNode *write_stmt(void);
static TreeNode *exp(void);
static TreeNode *simple_exp(void);
static TreeNode *term(void);
static TreeNode *factor(void);

static void syntaxError(char *message)
{
    fprintf(listing, "\n>>> ");
    fprintf(listing, "Syntax error at line %d: %s", lineno, message);
    Error = TRUE;
}

// 检查当前读取的 token 是否是 expected，是则继续读取下一个放入全局变量 token 中
static void match(TokenType expected)
{
    if (token == expected)
        token = getToken();
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        fprintf(listing, "      ");
    }
}

// program -> stmt_sequence

// stmt-sequence -> stmt-sequence `;` statement | statement
TreeNode *stmt_sequence(void)
{
    TreeNode *t = statement();
    TreeNode *p = t;
    while ((token != ENDFILE) && (token != END) &&
           (token != ELSE) && (token != UNTIL))
    {
        TreeNode *q;
        match(SEMI);
        q = statement();
        if (q != NULL)
        {
            if (t == NULL)
                t = p = q;
            else /* now p cannot be NULL either */
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// statement -> if_stmt | repeat_stmt | assign-stmt | read-stmt | write-stmt
TreeNode *statement(void)
{
    TreeNode *t = NULL;
    switch (token)
    {
    case IF:
        t = if_stmt();
        break;
    case REPEAT:
        t = repeat_stmt();
        break;
    case ID:
        t = assign_stmt();
        break;
    case READ:
        t = read_stmt();
        break;
    case WRITE:
        t = write_stmt();
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    } /* end case */
    return t;
}

// if-stmt -> `if` exp `then` stmt-sequence `end`
//          | `if` exp `then` stmt-sequence `else` stmt-sequence `end`
TreeNode *if_stmt(void)
{
    TreeNode *t = newStmtNode(IfK);
    match(IF);
    if (t != NULL)
        t->child[0] = exp();
    match(THEN);
    if (t != NULL)
        t->child[1] = stmt_sequence();
    if (token == ELSE)
    {
        match(ELSE);
        if (t != NULL)
            t->child[2] = stmt_sequence();
    }
    match(END);
    return t;
}

// repeat-stmt -> `repeat` stmt-sequence `until` exp
TreeNode *repeat_stmt(void)
{
    TreeNode *t = newStmtNode(RepeatK);
    match(REPEAT);
    if (t != NULL)
        t->child[0] = stmt_sequence();
    match(UNTIL);
    if (t != NULL)
        t->child[1] = exp();
    return t;
}

// assign-stmt -> identifier := exp
TreeNode *assign_stmt(void)
{
    TreeNode *t = newStmtNode(AssignK);
    if ((t != NULL) && (token == ID))
        t->attr.name = copyString(tokenString);
    match(ID);
    match(ASSIGN);
    if (t != NULL)
        t->child[0] = exp();
    return t;
}

// read-stmt -> read identifier
TreeNode *read_stmt(void)
{
    TreeNode *t = newStmtNode(ReadK);
    match(READ);
    if ((t != NULL) && (token == ID))
        t->attr.name = copyString(tokenString);
    match(ID);
    return t;
}

// write_stmt -> write exp
TreeNode *write_stmt(void)
{
    TreeNode *t = newStmtNode(WriteK);
    match(WRITE);
    if (t != NULL)
        t->child[0] = exp();
    return t;
}

// exp -> simple-exp comparison-op simple-exp | simple-exp      // comparison-op -> < | =
TreeNode *exp(void)
{
    TreeNode *t = simple_exp();
    if ((token == LT) || (token == EQ))
    {
        TreeNode *p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
        }
        match(token);
        if (t != NULL)
            t->child[1] = simple_exp();
    }
    return t;
}

// simple_exp -> simple-exp addop term | term       // addop -> + | -
TreeNode *simple_exp(void)
{
    TreeNode *t = term();
    while ((token == PLUS) || (token == MINUS))
    {
        TreeNode *p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->child[1] = term();
        }
    }
    return t;
}

// term -> factor { mulop factor }      // mulop -> * | /
TreeNode *term(void)
{
    TreeNode *t = factor();
    while ((token == TIMES) || (token == OVER))
    {
        TreeNode *p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            p->child[1] = factor();
        }
    }
    return t;
}

// factor -> (exp) | number | identifier
TreeNode *factor(void)
{
    TreeNode *t = NULL;
    switch (token)
    {
    case NUM: // number
        t = newExpNode(ConstK);
        if ((t != NULL) && (token == NUM))
            t->attr.val = atoi(tokenString);
        match(NUM);
        break;
    case ID: // identifier
        t = newExpNode(IdK);
        if ((t != NULL) && (token == ID))
            t->attr.name = copyString(tokenString);
        match(ID);
        break;
    case LPAREN: // (exp)
        match(LPAREN);
        t = exp();
        match(RPAREN);
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    }
    return t;
}

/****************************************/
/* the primary function of the parser   */
/****************************************/
/* Function parse returns the newly
 * constructed syntax tree
 */
TreeNode *parse(void)
{
    TreeNode *t;
    token = getToken();
    t = stmt_sequence();
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
