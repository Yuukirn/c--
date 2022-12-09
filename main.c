/****************************************************/
/* File: main.c                                     */
/* Main program for TINY compiler                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"

/* set NO_PARSE to TRUE to get a scanner-only compiler */
#define NO_PARSE TRUE
/* set NO_ANALYZE to TRUE to get a parser-only compiler */
#define NO_ANALYZE FALSE

/* set NO_CODE to TRUE to get a compiler that does not
 * generate code
 */
#define NO_CODE FALSE

#include "util.h"

#if NO_PARSE
#include "scan.h"
#else

#include "parse.h"

#if !NO_ANALYZE
#include "analyze.h"
#if !NO_CODE
#include "cgen.h"
#endif
#endif
#endif

/* allocate global variables */
int lineno = 0; //行号
FILE *source; // *tny
FILE *listing; // stdout
FILE *code; // *.tm

/* allocate and set tracing flags */
int EchoSource = TRUE;
int TraceScan = TRUE;
int TraceParse = TRUE;
int TraceAnalyze = TRUE;
int TraceCode = TRUE;

int Error = FALSE;

int main(int argc, char *argv[]) {
    TreeNode *syntaxTree;
    char pgm[120]; /* source code file name */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    strcpy(pgm, argv[1]);
    if (strchr(pgm, '.') == NULL) // 添加后缀
        strcat(pgm, ".tny");
    source = fopen(pgm, "r");
    if (source == NULL) {
        fprintf(stderr, "File %s not found\n", pgm);
        exit(1);
    }
    listing = stdout; /* send listing to screen */
    fprintf(listing, "\nTINY COMPILATION: %s\n", pgm);
#if NO_PARSE
    while (getToken() != ENDFILE);
#else
    syntaxTree = parse(); // 解析得到语法树
    if (TraceParse) {
        fprintf(listing, "\nSyntax tree:\n");
        printTree(syntaxTree);
    }
#if !NO_ANALYZE
    if (!Error)
    {
        if (TraceAnalyze)
            fprintf(listing, "\nBuilding Symbol Table...\n");
        buildSymtab(syntaxTree); //根据语法树得到符号表
        if (TraceAnalyze)
            fprintf(listing, "\nChecking Types...\n");
        typeCheck(syntaxTree); // 类型检查
        if (TraceAnalyze)
            fprintf(listing, "\nType Checking Finished\n");
    }
#if !NO_CODE
    if (!Error)
    {
        char *codefile;
        // size_t strcspn(const char *str1, const char *str2)
        // 检索 str1 开头连续有几个字符都不含 str2 中的字符
        // 此处用于求后缀前的文件名的长度
        int fnlen = strcspn(pgm, ".");
        codefile = (char *)calloc(fnlen + 4, sizeof(char));
        strncpy(codefile, pgm, fnlen);
        strcat(codefile, ".tm");
        code = fopen(codefile, "w");
        if (code == NULL)
        {
            printf("Unable to open %s\n", codefile);
            exit(1);
        }
        codeGen(syntaxTree, codefile); // 生成 .tm 文件
        fclose(code);
    }
#endif
#endif
#endif
    fclose(source);
    return 0;
}
