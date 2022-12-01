/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 0;

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
// 前序用于构建语法树，后序用于检查类型
static void traverse(TreeNode *t,
                     void (*preProc)(TreeNode *),
                     void (*postProc)(TreeNode *))
{
    if (t != NULL)
    {
        preProc(t);
        {
            int i;
            for (i = 0; i < MAXCHILDREN; i++)
                traverse(t->child[i], preProc, postProc);
        }
        postProc(t);
        traverse(t->sibling, preProc, postProc);
    }
}

/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
// 用于在 traverse() 中禁用另一个遍历
static void nullProc(TreeNode *t)
{
    if (t == NULL)
        return;
    else
        return;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
// 将 t 中的标识符插入到符号表中
static void insertNode(TreeNode *t)
{
    switch (t->nodekind)
    {
    case StmtK:
        switch (t->kind.stmt)
        {
        case AssignK:
        case ReadK:
            // 在符号表中查找是否有名为 attr.name 的变量
            if (st_lookup(t->attr.name) == -1)
                // 若没有，则认为是新变量
                /* not yet in table, so treat as new definition */
                st_insert(t->attr.name, t->lineno, location++);
            else
                /* already in table, so ignore location,
                   add line number of use only */
                st_insert(t->attr.name, t->lineno, 0);
            break;
        default:
            break;
        }
        break;
    case ExpK:
        switch (t->kind.exp)
        {
        case IdK:
            if (st_lookup(t->attr.name) == -1)
                /* not yet in table, so treat as new definition */
                st_insert(t->attr.name, t->lineno, location++);
            else
                /* already in table, so ignore location,
                   add line number of use only */
                st_insert(t->attr.name, t->lineno, 0);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
// 前序遍历语法树来构造符号表
void buildSymtab(TreeNode *syntaxTree)
{
    traverse(syntaxTree, insertNode, nullProc);
    if (TraceAnalyze)
    {
        fprintf(listing, "\nSymbol table:\n\n");
        printSymTab(listing);
    }
}

static void typeError(TreeNode *t, char *message)
{
    fprintf(listing, "Type error at line %d: %s\n", t->lineno, message);
    Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t)
{
    switch (t->nodekind)
    {
    case ExpK:
        switch (t->kind.exp)
        {
        case OpK: // 运算符，两边都必须是整数
            if ((t->child[0]->type != Integer) ||
                (t->child[1]->type != Integer))
                typeError(t, "Op applied to non-integer");
            if ((t->attr.op == EQ) || (t->attr.op == LT))
                t->type = Boolean; // 比较运算符，此时父节点的类型是比较的结果
            else
                t->type = Integer;
            break;
        case ConstK:
        case IdK: // TODO
            t->type = Integer;
            break;
        default:
            break;
        }
        break;
    case StmtK:
        switch (t->kind.stmt)
        {
        case IfK:
            if (t->child[0]->type == Integer)
                typeError(t->child[0], "if test is not Boolean");
            break;
        case AssignK:
            if (t->child[0]->type != Integer)
                typeError(t->child[0], "assignment of non-integer value");
            break;
        case WriteK:
            if (t->child[0]->type != Integer)
                typeError(t->child[0], "write of non-integer value");
            break;
        case RepeatK:
            if (t->child[1]->type == Integer)
                typeError(t->child[1], "repeat test is not Boolean");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
// 类型检查
void typeCheck(TreeNode *syntaxTree)
{
    traverse(syntaxTree, nullProc, checkNode);
}
