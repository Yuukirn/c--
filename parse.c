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

static TokenType token; /* holds current token */

// 1. program -> declaration-list
// 2. declaration-list -> declaration-list declaration | declaration
// 3. declaration -> var-declaration | fun-declaration

// decl -> stmt
// param -> stmt
// 4. var-declaration -> `type-specifier` ID; // 变量声明 后面的是数组，可不实现
// 5. `type-specifier` -> int | void
// 6. fun-declaration -> `type-specifier` ID ( params ) compound-stmt //
// 函数声明  compound-stmt 是函数体
// 7. params -> param-list | void
// 8. param-list -> param-list, param | param // 左递归
// 9. param -> `type-specifier` ID
// 10. compound-stmt -> { local-declarations statement-list } // 复合语句 //
// 11. local-declarations -> local-declarations var-declaration | empty
// 12. statement-list -> statement-list statement | empty
// 13. statement -> assign-stmt | compound-stmt | selection-stmt |
// iteration-stmt | return-stmt
// 14. assign-stmt -> var = expression; | ;   // 表达式语句用于赋值和函数调用
// 15. selection-stmt -> `if` ( expression ) statement
//                     | `if` ( expression ) statement `else` statement
// 16. iteration-stmt -> `while` ( expression ) statement
// 17. return-stmt -> `return` ; | `return` expression;
// 18. expression -> simple-expression
// 19. var -> ID | ID [ expression ] // 后一项可舍弃
// 20. simple-expression -> additive-expression relop additive-expression //
// 简单表达式由无结合的关系操作符组成
//                        | additive-expression
// 21. relop -> <= | < | > | >= | == | !=
// 22. additive-expression -> additive-expression addop term | term
// 23. addop -> + | -
// 24. term -> term mulop factor | factor
// 25. mulop -> * | /
// 26. factor -> ( expression ) | var | call | NUM // factor
// 是围在括号内的表达式；或一个变量，求出其变量的值；或者一个函数调用，求出函数的返回值；或者一个
// NUM，其值由扫描器计算
// 27. call -> ID ( args ) // 函数调用，ID
// 是函数名，参数为空或以逗号分隔的表达式组成
// 28. args -> arg-list | empty
// 29. arg-list -> arg-list, expression | expression // 左递归

static TreeNode *declaration_list();

static TreeNode *declaration();

static TreeNode *param_list();

static TreeNode *param();

static TreeNode *assign_stmt();

//static TreeNode *expression();

static TreeNode *statement_list();

static TreeNode *statement();

static TreeNode *compound_stmt();

static TreeNode *selection_stmt();

static TreeNode *iteration_stmt();

static TreeNode *return_stmt();

static TreeNode *local_declarations();

static TreeNode *simple_expression();

static TreeNode *additive_expression();

static TreeNode *call();

static TreeNode *args();

static TreeNode *arg_list();

static TreeNode *term();

static TreeNode *factor();

static void syntaxError(char *message) {
    fprintf(listing, "\n>>> ");
    fprintf(listing, "Syntax error at line %d: %s", lineno, message);
    Error = TRUE;
}

// 检查当前读取的 token 是否是 expected，是则继续读取下一个放入全局变量 token 中
static void match(TokenType expected) {
    if (token == expected)
        token = getToken();
    else {
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
    switch (token) {
        case VOID:
            t->type = Void;
            break;
        case INT:
            t->type = Integer;
            break;
    }
    match(token); // type-specifier
    if (t != NULL && token == ID) {
        t->attr.name = copyString(tokenString);
    }
    match(ID);          // check ID 后，当前 token 为 ; 或 (
    if (token == ';') { // var-declaration
        t->kind.stmt = VarDeclarationK;
        match(';');
    } else if (token == '(') { // func-declaration
        t->kind.stmt = FuncDeclarationK;
        match('(');
        if (token == VOID) {
            t->child[0] = newExpNode(ParamK);
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
    while (token != ')') { // 读取到 ) 说明到了 ( params ) 的尾
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
    TreeNode *t = newExpNode(ParamK);
    if (token == INT) {
        t->type = Integer;
    } else if (token == VOID) {
        t->type = Void;
    }
    match(token);
    t->attr.name = copyString(tokenString);
    return t;
}

// statement_list -> statement_list statement | empty
TreeNode *statement_list() {
    // TODO: 暂不支持 empty
    TreeNode *t = statement();
    TreeNode *p = t;
    while (token != '}') { // statement_list 只用在 compound-stmt 中
        TreeNode *q = statement();
        if (q != NULL) {
            if (t == NULL) {
                t = p = q;
            } else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

// statement -> expression-stmt | compound-stmt | selection-stmt | iteration-stmt | return-stmt
TreeNode *statement() {
    TreeNode *t = NULL;
    switch (token) {
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
            t = assign_stmt();
            break;
        default:
            syntaxError("unexpected token -> ");
            printToken(token, tokenString);
            token = getToken();
            break;
    } /* end case */
    return t;
}


//TreeNode *expression_stmt() {
//    TreeNode *t = NULL;
//    if (token != ';') {
//        t = expression();
//    }
//    match(SEMI);
//    return t;
//}

// assign-stmt -> var = expression; | ;
// expression -> simple-expression
TreeNode *assign_stmt() {
    TreeNode *t = newStmtNode(AssignK);
    if ((t != NULL) && token == ID) {
        t->attr.name = copyString(tokenString);
    }
    match(ID);
    match(ASSIGN);
    if (t != NULL) {
        t->child[0] = simple_expression();
    }
    return t;
}

// iteration-stmt -> `while` ( expression ) statement
// TODO: statement ---> compound-stmt ? statement 包含了 compound-stmt
TreeNode *iteration_stmt() {
    TreeNode *t = newStmtNode(WhileK);
    match(WHILE);
    match('(');
    if (t != NULL) {
        t->child[0] = simple_expression();
    }
    match(')');
    if (t != NULL) {
        t->child[1] = statement();
    }
    return t;
}

// 15. selection-stmt -> `if` ( expression ) statement
//                     | `if` ( expression ) statement `else` statement
TreeNode *selection_stmt() {
    TreeNode *t = newStmtNode(SelectionK);
    match(IF);
    if (t != NULL) {
        match(LPAREN);
        t->child[0] = simple_expression();
        match(RPAREN);
    }
    if (t != NULL) {
        t->child[1] = statement(); // TODO ?
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
    if (t != NULL) {
        t->child[0] = local_declarations();
        t->child[1] = statement_list();
    }
    match('}');

    return t;
}

// return-stmt -> `return` ; | `return` expression;
TreeNode *return_stmt() {
    TreeNode *t = newStmtNode(ReturnK);
    match(RETURN);
    if (token == ';') {
        t->type = Void;
    } else {
        t->type = Integer; // 返回值只可能是 void 或 int
        t->child[0] = simple_expression();
    }
}

// local_declarations -> local-declarations var-declaration | empty
// var-declaration -> `type-specifier` ID;
TreeNode *local_declarations() { // TODO:
    TreeNode *t = NULL;
    TreeNode *p = t;
    if (token != INT && token != VOID) { // empty
        return t;
    }
    while (token == INT || token == VOID) {
        // 建立节点
        TreeNode *q = newStmtNode(VarDeclarationK);
        match(token);
        if (t != NULL && token == ID) {
            t->attr.name = copyString(tokenString);
        }
        match(ID);
        match(';');

        // 连接
        if (t == NULL) {
            t = p = q;
        } else {
            p->sibling = q;
            p = q;
        }
    }
    return t;
}

// expression -> simple-expression
// simple-expression -> additive-expression relop additive-expression
//                    | additive-expression
// ID | ( expression ), ID, NUM
//TreeNode *expression() {
//    // TODO
//    TreeNode *t = NULL;
//    switch (token) {
//        case LPAREN:
//        case NUM: { // simple-expression
//            t = simple_expression();
//        }
//        case ID: {
//            char *id = copyString(tokenString);
//            match(ID);
//            if (token == '=') { // var = expression
//                // TODO: !!! 这里逻辑有问题，再想想如何分辨两种情况 添加 assign_expression -> var = expression ?
//                t = newExpNode(AssignK);
//                t->attr.name = copyString(id);
//            } else { // TODO: 其他ID开头的
//
//            }
//        }
//    }
//    return t;
//}

// simple_expression -> additive-expression relop additive-expression
//                    | additive-expression
// relop -> <= | < | > | >= | == | !=
TreeNode *simple_expression() {
    TreeNode *t = newExpNode(OpK);
    if (t != NULL) {
        t->child[0] = additive_expression();
    }
    if ((token == LE) || (token == LT) || (token == RT) || (token == RE) || (token == EQ) || (token == NE)) {
        t->attr.op = token;
        match(token);
        if (t != NULL) {
            t->child[1] = additive_expression();
        }
    }
    return t;
}

// c--: additive_expression -> additive_expression addop term | term
TreeNode *additive_expression() {
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
    return t;
}

// call -> ID ( args )
// args -> arg-list | empty
// arg-list -> arg-list, expression | expression
TreeNode *call() {
    TreeNode *t = newExpNode(CallK);
    if (t != NULL && token == ID) {
        t->attr.name = copyString(tokenString);
        match(ID);
        match('(');
        t->child[0] = args();
        match(')');
    }
    return t;
}

TreeNode *args() {
    TreeNode *t = NULL;
    if (token == ')') {
        return t;
    }
    t = arg_list();
    return t;
}

// arg-list -> arg-list, expression | expression
TreeNode *arg_list() {
    TreeNode *t = simple_expression();
    TreeNode *p = t;
    while (token != ')') {
        match(COMMA);
        TreeNode *q;
        q = simple_expression();
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

// term -> factor { mulop factor }      // mulop -> * | /
// term -> term mulop factor | factor // 同 c--
TreeNode *term() {
    TreeNode *t = factor();
    while ((token == TIMES) || (token == OVER)) {
        TreeNode *p = newExpNode(OpK);
        if (p != NULL) {
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
TreeNode *factor() {
    TreeNode *t = NULL;
    switch (token) {
        case NUM: // NUM
            t = newExpNode(ConstK);
            if ((t != NULL) && (token == NUM))
                t->attr.val = atoi(tokenString);
            match(NUM);
            break;
        case ID: // var or call
            t = newNullExpNode();
            if ((t != NULL) && (token == ID)) // var
                t->attr.name = copyString(tokenString);
            match(ID);
            if (token == '(') { // call
                t->kind.exp = CallK;
                match(LPAREN);
                t->child[0] = args();
                match(RPAREN);
            } else {
                t->kind.exp = IdK;
            }
            break;
        case LPAREN: // (expression)
            match(LPAREN);
            t = simple_expression();
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
TreeNode *parse(void) {
    TreeNode *t;
    token = getToken();
    // program -> stmt_sequence
    t = declaration_list(); // TODO:
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
