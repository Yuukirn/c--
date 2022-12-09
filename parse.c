/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "parse.h"
#include "globals.h"
#include "scan.h"
#include "util.h"
#include <stdlib.h>

static TokenType token; /* holds current token */

static TreeNode *declaration_list();

static TreeNode *declaration();

static TreeNode *param_list();

static TreeNode *param();

static TreeNode *assign_stmt();

static TreeNode *expression();

static TreeNode *statement_list();

static TreeNode *statement();

static TreeNode *compound_stmt();

static TreeNode *selection_stmt();

static TreeNode *iteration_stmt();

static TreeNode *return_stmt();

static TreeNode *expression_stmt();

static TreeNode *local_declarations();

static TreeNode *simple_expression();

static TreeNode *additive_expression();

static TreeNode *call();

static TreeNode *args();

static TreeNode *arg_list();

static TreeNode *term();

static TreeNode *factor();

static void syntaxError(char *message)
{
    fprintf(listing, "\n>>> ");
    fprintf(listing, "Syntax error at line %d: %s", lineno, message);
    Error = TRUE;
    printToken(token, tokenString);
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
        fprintf(listing, "\texpected token -> ");
        printToken(expected, tokenString);
        fprintf(listing, "      ");
    }
}

// program -> declaration_list
// declaration_list -> declaration_list declaration | declaration
TreeNode *declaration_list()
{
    TreeNode *t = declaration();
    TreeNode *p = t;
    while ((token != ENDFILE) && (token != EOF) && (token != RBRACE))
    {
        TreeNode *q;
        q = declaration();
        if (q != NULL)
        {
            if (t == NULL)
            {
                t = p = q; // TODO: ?
            }
            else
            {
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
TreeNode *declaration()
{
    TreeNode *t = newNullStmtNode();
    switch (token)
    {
    case VOID:
        t->type = Void;
        break;
    case INT:
        t->type = Integer;
        break;
    }
    match(token); // type-specifier
    if (t != NULL && token == ID)
    {
        t->attr.name = copyString(tokenString);
    }
    match(ID); // check ID 后，当前 token 为 ; 或 (
    if (token == SEMI)
    { // var-declaration
        t->kind.stmt = VarDeclarationK;
        match(SEMI);
    }
    else if (token == LPAREN)
    { // func-declaration
        t->kind.stmt = FuncDeclarationK;
        match(LPAREN);
        if (token == VOID || token == RPAREN)
        {
            t->child[0] = newExpNode(ParamK);
            t->child[0]->type = Void;
            match(token);
        }
        else
        {
            t->child[0] = param_list();
            match(RPAREN);
        }
        t->child[1] = compound_stmt();
    }
    return t;
}

// param-list -> param-list, param | param
TreeNode *param_list()
{
    TreeNode *t = param();
    TreeNode *p = t;
    while (token != RPAREN)
    { // 读取到 ) 说明到了 ( params ) 的尾
        match(COMMA);
        TreeNode *q;
        q = param();
        if (q != NULL)
        {
            if (t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// param -> `type-specifier` ID
TreeNode *param()
{
    TreeNode *t = newExpNode(ParamK);
    if (token == INT)
    {
        t->type = Integer;
    }
    else if (token == VOID)
    {
        t->type = Void;
    }
    match(token);
    t->attr.name = copyString(tokenString);
    match(ID);
    return t;
}

// statement_list -> statement_list statement | empty
TreeNode *statement_list()
{
    TreeNode *t = statement();
    TreeNode *p = t;
    while (token != RBRACE)
    {
        TreeNode *q = statement();
        if (q != NULL)
        {
            if (t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// statement -> expression-stmt | compound-stmt | selection-stmt | iteration-stmt | return-stmt
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
    case RETURN:
        t = return_stmt();
        break;
    case LBRACE:
        t = compound_stmt();
        break;
        // ( expression ) | var | call | num | id
    case ID:
        // t = assign_stmt();
        t = expression_stmt();
        break;
    case RBRACE:
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
    } /* end case */
    return t;
}

// expression_stmt -> expression; | ;
// expression -> var = expression | simple-expression;
TreeNode *expression_stmt() {
    TreeNode *t = NULL;
    if (token != ';') {
        t = expression();
    }
    match(SEMI);
    return t;
}

// assign-stmt -> var = expression; | ;
// expression -> simple-expression
TreeNode *assign_stmt()
{
    TreeNode *t = newStmtNode(AssignK);
    if ((t != NULL) && token == ID)
    {
        t->attr.name = copyString(tokenString);
    }
    match(ID);
    match(ASSIGN);
    if (t != NULL)
    {
        t->child[0] = simple_expression();
    }
    match(SEMI);
    return t;
}

// iteration-stmt -> `while` ( expression ) statement
TreeNode *iteration_stmt()
{
    TreeNode *t = newStmtNode(WhileK);
    match(WHILE);
    match(LPAREN);
    if (t != NULL)
    {
        t->child[0] = simple_expression();
    }
    match(RPAREN);
    if (t != NULL)
    {
        t->child[1] = statement();
    }
    return t;
}

// selection-stmt -> `if` ( expression ) statement
//                     | `if` ( expression ) statement `else` statement
TreeNode *selection_stmt()
{
    TreeNode *t = newStmtNode(SelectionK);
    match(IF);
    if (t != NULL)
    {
        match(LPAREN);
        t->child[0] = simple_expression();
        match(RPAREN);
    }
    if (t != NULL)
    {
        t->child[1] = statement();
    }
    if (token == ELSE)
    {
        match(ELSE);
        t->child[2] = statement();
    }
    return t;
}

// compound-stmt -> { local-declarations statement-list }
TreeNode *compound_stmt()
{
    TreeNode *t = newStmtNode(CompoundK);
    match(LBRACE);
    if (t != NULL)
    {
        t->child[0] = local_declarations();
        t->child[1] = statement_list();
    }
    match(RBRACE);

    return t;
}

// return-stmt -> `return` ; | `return` expression;
TreeNode *return_stmt()
{
    TreeNode *t = newStmtNode(ReturnK);
    match(RETURN);
    if (token == SEMI)
    {
        t->type = Void;
    }
    else
    {
        t->type = Integer; // 返回值只可能是 void 或 int
        t->child[0] = simple_expression();
    }
    match(SEMI);
    return t;
}

// local_declaration -> local-declarations var-declaration | empty
// var-declaration -> `type-specifier` ID;
TreeNode *local_declarations()
{
    TreeNode *t = NULL;
    TreeNode *p = t;
    debug("local declaration\n");
    if (token != INT && token != VOID)
    { // empty
        return t;
    }
    while (token == INT || token == VOID)
    {
        // debug("i: %d\n", i++);
        TreeNode *q = newStmtNode(VarDeclarationK);
        if (token == INT) {
            q->type = Integer;
        } else {
            q->type = Void;
        }
        match(token);
        if (q != NULL && token == ID)
        {
            q->attr.name = copyString(tokenString);
        }
        match(ID);
        match(SEMI);

        // 连接
        if (t == NULL)
        {
            t = p = q;
        }
        else
        {
            p->sibling = q;
            p = q;
        }
    }
    return t;
}

// expression -> var = expression | simple-expression
TreeNode *expression() {
    TreeNode *t;
    char *s = (char *) malloc(sizeof(char) * 40);
    if (token == ID) {
        s = copyString(tokenString);
        match(ID);
    }
    
    if (token == ASSIGN) { // var = expression
        t = newStmtNode(AssignK);
        t->attr.name = copyString(s);
        match(ASSIGN);
        t->child[0] = simple_expression();
    } else { // simple-expression
        t = newExpNode(CallK);
        t->attr.name = copyString(s);
        match(LPAREN);
        t->child[0] = args();
        match(RPAREN);
    }

    free(s);
    return t;
}

// simple_expression -> additive-expression relop additive-expression
//                    | additive-expression
// relop -> <= | < | > | >= | == | !=
TreeNode *simple_expression()
{
    debug("simple-expression\n");
    TreeNode *t = NULL;
    TreeNode *q = additive_expression();
    if ((token == LE) || (token == LT) || (token == RT) || (token == RE) || (token == EQ) || (token == NE))
    {
        t = newExpNode(OpK);
        t->child[0] = q;
        t->attr.op = token;
        match(token);
        t->child[1] = additive_expression();
    } else {
        t = q;
    }
    return t;
}

// c--: additive_expression -> additive_expression addop term | term
TreeNode *additive_expression()
{
    debug("additive-expression\n");
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
// term -> term mulop factor | factor // 同 c--
TreeNode *term()
{
    debug("term\n");
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

// c--: factor -> ( expression ) | NUM | call | var
// call -> ID ( args )
TreeNode *factor()
{
    debug("factor\n");
    TreeNode *t = NULL;
    switch (token)
    {
    case NUM: {// NUM
        t = newExpNode(ConstK);
        if ((t != NULL) && (token == NUM))
            t->attr.val = atoi(tokenString);
        match(NUM);
        break;
        }
    case ID: { // var or call
        t = newNullExpNode();
        if ((t != NULL) && (token == ID)) // var
            t->attr.name = copyString(tokenString);
        match(ID);
        if (token == LPAREN)
        { // call
            t->kind.exp = CallK;
            match(LPAREN);
            t->child[0] = args();
            match(RPAREN);
        }
        else
        {
            t->kind.exp = IdK;
        }
        break;
        }
    case LPAREN:{ // (expression)
        match(LPAREN);
        t = simple_expression();
        match(RPAREN);
        break;
        }
    default: {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
        break;
        }
    }
    return t;
}

// call -> ID ( args )
// args -> arg-list | empty
// arg-list -> arg-list, expression | expression
TreeNode *call()
{
    TreeNode *t = newExpNode(CallK);
    if (t != NULL && token == ID)
    {
        t->attr.name = copyString(tokenString);
        match(ID);
        match(LPAREN);
        t->child[0] = args();
        match(RPAREN);
    }
    return t;
}

TreeNode *args()
{
    TreeNode *t = NULL;
    if (token == RPAREN)
    {
        return t;
    }
    t = arg_list();
    return t;
}

// arg-list -> arg-list, expression | expression
TreeNode *arg_list()
{
    TreeNode *t = simple_expression();
    TreeNode *p = t;
    while (token != RPAREN)
    {
        match(COMMA);
        TreeNode *q;
        q = simple_expression();
        if (q != NULL)
        {
            if (t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
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
    // program -> declaration_list
    t = declaration_list();
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
