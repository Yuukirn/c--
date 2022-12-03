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

// 1. program -> declaration-list
// 2. declaration-list -> declaration-list declaration | declaration
// 3. declaration -> var-declaration | fun-declaration
// 4. var-declaration -> `type-specifier` ID; | `type-specifier` ID [ NUM ]; // 变量声明  后面的是数组，可不实现
// 5. `type-specifier` -> int | void
// 6. fun-declaration -> `type-specifier` ID ( params ) compound-stmt // 函数声明  compound-stmt 是函数体
// 7. params -> param-list | void
// 8. param-list -> param-list, param | param // 左递归
// 9. param -> `type-specifier` ID | `type-specifier` ID [ ] // 数组可省略
// 10. compound-stmt -> { local-declarations statement-list } // 函数体
// 11. local-declarations -> local-declarations var-declaration | empty
// 12. statement-list -> statement-list statement | empty       // 和 local-declarations 一样，左递归，可以为空
// 13. statement -> expression-stmt | compound-stmt | selection-stmt | iteration-stmt | return-stmt
// 14. expression-stmt -> expression; | ;   // 表达式语句用于赋值和函数调用
// 15. selection-stmt -> `if` ( expression ) statement
//                     | `if` ( expression ) statement `else` statement
// 16. iteration-stmt -> `while` ( expression ) statement
// 17. return-stmt -> `return` ; | `return` expression;
// 18. expression -> var = expression | simple-expression
// 19. var -> ID | ID [ expression ] // 后一项可舍弃
// 20. simple-expression -> additive-expression relop additive-expression // 简单表达式由无结合的关系操作符组成
//                        | additive-expression
// 21. relop -> <= | < | > | >= | == | != 
// 22. additive-expression -> additive-expression addop term | term
// 23. addop -> + | -
// 24. term -> term mulop factor | factor
// 25. mulop -> * | /
// 26. factor -> ( expression ) | var | call | NUM // factor 是围在括号内的表达式；或一个变量，求出其变量的值；或者一个函数调用，求出函数的返回值；或者一个 NUM，其值由扫描器计算
// 27. call -> ID ( args ) // 函数调用，ID 是函数名，参数为空或以逗号分隔的表达式组成
// 28. args -> arg-list | empty
// 29. arg-list -> arg-list, expression | expression // 左递归

static TreeNode *declaration_list();

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
//         break;
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
    // program -> stmt_sequence
    t = stmt_sequence();
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
