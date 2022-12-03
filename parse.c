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

// 1. program -> declaration-list
// 2. declaration-list -> declaration-list declaration | declaration
// 3. declaration -> var-declaration | fun-declaration

// decl -> stmt
// param -> stmt
// 4. var-declaration -> `type-specifier` ID; // 变量声明  后面的是数组，可不实现
// 5. `type-specifier` -> int | void
// 6. fun-declaration -> `type-specifier` ID ( params ) compound-stmt // 函数声明  compound-stmt 是函数体
// 7. params -> param-list | void
// 8. param-list -> param-list, param | param // 左递归
// 9. param -> `type-specifier` ID
// 10. compound-stmt -> { local-declarations statement-list } // 复合语句 // TODO:大括号体现在 compound-stmt 中
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
static TreeNode *declaration();
static TreeNode *param_list();
static TreeNode *param();
static TreeNode *expression_stmt();
static TreeNode *compound_stmt();
static TreeNode *selection_stmt();
static TreeNode *iteration_stmt();
static TreeNode *return_stmt();
static TreeNode *local_declarations();
static TreeNode *statement_list();
static TreeNode *additive_expression();
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
// program -> declaration_list 程序由一堆声明组成
// declaration_list -> declaration_list declaration | declaration
TreeNode *declaration_list() {
    TreeNode *t = declaration();
    TreeNode *p = t;
    while (token != ENDFILE) {
        TreeNode *q;
        q = declaration();
        if (q != NULL) {
            if (t == NULL) {
                t = p = q; // TODO: ?
            } else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// declaration -> var-declaration | fun-declaration
// var-declaration -> `type-specifier` ID;
// fun-declaration -> `type-specifier` ID ( params ) compound-stmt
TreeNode *declaration() {
    TreeNode *t = newNullStmtNode();
    t->type = Integer;
    match(ID); // check ID 后，当前 token 为 ; 或 (
    t->attr.name = copyString(tokenString);
    if (token == ';') { // var-declaration
        t->kind.stmt = VarDeclarationK;
        match(';');
    } else if (token == '(') { // func-declaration
        t->kind.stmt = FuncDeclarationK;
        match('(');
        if (token == VOID) {
            t->child[0] = newStmtNode(ParamK);
            t->child[0]->type = Void;
        } else {
            t->child[0] = param_list();
        }
        // t->child[0] = params(); // TODO
        match(')');
        t->child[1] = compound_stmt();
    }
    return t;
}

// param-list -> param-list, param | param
TreeNode *param_list() {
    TreeNode *t = param();
    TreeNode *p = t;
    while ( token != ')' ) { // 读取到 ) 说明到了 ( params ) 的尾
        match(COMMA);
        TreeNode *q;
        q = param();
        if (q != NULL) {
            if (t != NULL) {
                t = p = q;
            } else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// param -> `type-specifier` ID
TreeNode *param() {
    TreeNode *t = newStmtNode(ParamK);
    if (token == INT) {
        t->type = Integer;
    } else if (token == VOID) {
        t->type = Void;
    }
    match(token);
    t->attr.name = copyString(tokenString);
    return t;
}

// statement -> if_stmt | repeat_stmt | assign-stmt | read-stmt | write-stmt
TreeNode *statement()
{
    TreeNode *t = NULL;
    switch (token)
    {
    case IF:
        t = selection_stmt();
        break;
    case WHILE:
        t = iteration_stmt();
        break;
    case ID:
        t = assign_stmt(); // TODO
        break;
    case RETURN:
        t = return_stmt();
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    } /* end case */
    return t;
}

// 15. selection-stmt -> `if` ( expression ) statement
//                     | `if` ( expression ) statement `else` statement
TreeNode *selection_stmt(void)
{
    TreeNode *t = newStmtNode(SelectionK);
    match(IF);
    if (t != NULL)
    {
        match(LPAREN);
        t->child[0] = expression();
        match(RPAREN);
    }
    if (t != NULL) {
        t->child[1] = statement();
    }
    if (token == ELSE) {
        // TODO: statement -> compound=stmt?
    }
    return t;
}

// compound-stmt -> { local-declarations statement-list }
// 两个子节点
TreeNode *compound_stmt() {
    TreeNode *t = newStmtNode(CompoundK);
    match('{');
    t->child[0] = local_declarations();
    t->child[1] = statement_list();
    match('}');

    return t;
}

// local_declarations -> local-declarations var-declaration | empty
TreeNode *local_declarations() {
    TreeNode *t;
    if (token != INT && token != VOID) { // empty
        return t;
    }
    while (token == INT || token == VOID) {
        t = newStmtNode();
    }
}


// expression -> var = expression | simple-expression
TreeNode *expression(void) {

}

// c--: additive_expression -> additive_expression addop term | term
// 同 tiny 的 simple_exp
TreeNode *additive_expression(void)
{
    TreeNode *t = term();
    while ((token == PLUS) || (token == MINUS)) {
        TreeNode *p = newExpNode(OpK);
        if (p != NULL) {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->child[1] = term();
        }
    }
}

// term -> factor { mulop factor }      // mulop -> * | /
// term -> term mulop factor | factor // 同 c--
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

// TODO
// factor -> (exp) | number | identifier
// c--: factor -> ( expression ) | NUM | call | var
TreeNode *factor(void)
{
    TreeNode *t = NULL;
    switch (token)
    {
    case NUM: // NUM
        t = newExpNode(ConstK);
        if ((t != NULL) && (token == NUM))
            t->attr.val = atoi(tokenString);
        match(NUM);
        break;
    case ID: // var == id
        t = newExpNode(IdK);
        if ((t != NULL) && (token == ID))
            t->attr.name = copyString(tokenString);
        match(ID);
        break;
    case LPAREN: // (expression)
        match(LPAREN);
        t = expression();
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
