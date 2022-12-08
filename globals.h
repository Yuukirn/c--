/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 6

typedef enum
/* book-keeping tokens */
{
    ENDFILE,
    ERROR,
    /* reserved words */
    // 保留字
    IF,
    ELSE,
    INT,
    VOID,
    WHILE,
    RETURN,
    /* multicharacter tokens */
    ID,
    NUM,
    /* special symbols */
    // 特殊符号
    ASSIGN, // := -> =
    EQ, // = -> ==
    NE, // !=
    LT, // <
    LE, // <=
    RT, // >
    RE, // >=
    PLUS, // +
    MINUS, // -
    TIMES, // *
    OVER, // /
    LPAREN, // (
    RPAREN, // )
    SEMI, // ;
    COMMA, // ,
    LBRACE, // {
    RBRACE, // }
    LANNO, // /* // 这俩先保留
    RANNO, // */
} TokenType;

extern FILE *source;  /* source code text file */
extern FILE *listing; /* listing output text file */
extern FILE *code;    /* code text file for TM simulator */

// 在转换的以后步骤中出现错误时能够打印源代码行数
extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {
    StmtK, // 语句
    ExpK // 表达式
} NodeKind; // 构建语法树时标识类型

// TODO:
typedef enum {
    SelectionK,
    WhileK,
    AssignK,
    ReturnK,
    CompoundK,
    VarDeclarationK,
    FuncDeclarationK,
} StmtKind; // 语句类型

// TODO:
// 用于表达式类型检查
typedef enum {
    OpK, // operation 比较或运算
    ConstK, // 常量数值
    IdK, // 标识符
    ParamK,
    CallK, // 函数调用
//    AssignK, // var = expression
} ExpKind;

/* ExpType is used for type checking */
typedef enum {
    Void,
    Integer,
    Boolean
} ExpType;

#define MAXCHILDREN 3

typedef struct treeNode {
    struct treeNode *child[MAXCHILDREN];
    struct treeNode *sibling; // 连接同一层次的节点的链表
    int lineno;
    NodeKind nodekind;
    union {
        StmtKind stmt; // nodekind -> StmtK
        ExpKind exp; // nodekind -> ExpK
    } kind;
    union {
        TokenType op;
        int val;
        char *name;
    } attr;
    ExpType type; /* for type checking of exps */
} TreeNode;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;
#endif
