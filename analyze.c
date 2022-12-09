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
        case VarDeclarationK:
            if (st_lookup(t->attr.name) == -1) {
            /* not yet in table, so treat as new definition */
                st_insert(t->attr.name, t->lineno, location++, VAR);
            } else {
            /* already in table, so ignore location,
                   add line number of use only */
                st_insert(t->attr.name, t->lineno, 0, VAR);
            }
            break;
        case FuncDeclarationK:
            if (st_lookup(t->attr.name) == -1) {
                st_insert(t->attr.name, t->lineno, location++, FUNC);
            } else {
                st_insert(t->attr.name, t->lineno, 0, FUNC);
            }
            break;
        case ReturnK:
            break;
        default:
            break;
        }
        break;
    case ExpK:
        switch (t->kind.exp)
        {
        case IdK:
            if (st_lookup(t->attr.name) == -1) {
                /* not yet in table, so treat as new definition */
                st_insert(t->attr.name, t->lineno, location++, VAR);
                }
            else {
                /* already in table, so ignore location,
                   add line number of use only */
                st_insert(t->attr.name, t->lineno, 0, VAR);
                }
            break;
        case CallK:
            if (st_lookup(t->attr.name) == -1) {
                st_insert(t->attr.name, t->lineno, location++, FUNC);
            } else {
                st_insert(t->attr.name, t->lineno, 0, FUNC);
            }
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
        case OpK: // 运算符，两边都必须是 int（包括 id）
            if ((t->child[0]->type != Integer) ||
                (t->child[1]->type != Integer))
                typeError(t, "Op applied to non-integer");
            if ((t->attr.op == EQ) || (t->attr.op == LT))
                t->type = Boolean;
            else
                t->type = Integer;
            break;
        case ConstK:
        case IdK: // id 视为 int，便于检查
            t->type = Integer;
            break;
        default:
            break;
        }
        break;
    case StmtK:
        switch (t->kind.stmt)
        {
        case SelectionK:
            if (t->child[0]->type == Integer)
                typeError(t->child[0], "if test is not Boolean");
            break;
        case AssignK:
            if (t->child[0]->type != Integer && t->child[0]->kind.stmt != CallK)
                typeError(t->child[0], "assignment of non-integer or non-call value");
            break;
        case WhileK:
            if (t->child[0]->type == Integer)
                typeError(t->child[1], "while test is not Boolean");
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
