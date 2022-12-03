/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"

/* states in scanner DFA */
typedef enum
{
    START,
    INASSIGN, // START + : -> INASSIGN
    // 改成 START + "/" -> INCOMMENT1 + "*" -> INCOMMENT2 + "*" -> INCOMMENT3 + "/" -> INCOMMENT4
    INCOMMENT1, // START + { -> INCOMMENT + } -> START
    INCOMMENT2,
    INCOMMENT3,
    INLE,
    INRE,
    INNE,
    INEQ,
    INNUM, // START + digit -> INNUM
    INID,  // START + letter -> INID
    INBLOCK,
    DONE
} StateType;

/* lexeme of identifier or reserved word */
// 标识符或保留字
// 先将标识符或保留字保留在该数组中，然后再在保留字表中识别出保留字
char tokenString[MAXTOKENLEN + 1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0;      /* current position in LineBuf */
static int bufsize = 0;      /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
static int getNextChar(void)
{
    if (!(linepos < bufsize))
    {
        lineno++;
        if (fgets(lineBuf, BUFLEN - 1, source))
        {
            if (EchoSource)
                fprintf(listing, "%4d: %s", lineno, lineBuf);
            bufsize = strlen(lineBuf);
            linepos = 0;
            return lineBuf[linepos++];
        }
        else
        {
            EOF_flag = TRUE;
            return EOF;
        }
    }
    else
        return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
// 会退一个位置
static void ungetNextChar(void)
{
    if (!EOF_flag)
        linepos--;
}

/* lookup table of reserved words */
// 保留字表
static struct
{
    char *str;
    TokenType tok;
} reservedWords[MAXRESERVED] = {{"if", IF}, {"int", INT}, {"else", ELSE}, {"void", VOID}, {"while", WHILE}, {"return", RETURN}, {"read", READ}, {"write", WRITE}};

/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
// 检查是否是保留字，是的话返回保留字，否则返回 id
static TokenType reservedLookup(char *s)
{
    int i;
    for (i = 0; i < MAXRESERVED; i++)
        if (!strcmp(s, reservedWords[i].str))
            return reservedWords[i].tok;
    return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the
 * next token in source file
 */
TokenType getToken(void)
{ /* index for storing into tokenString */
    int tokenStringIndex = 0;
    /* holds current token to be returned */
    TokenType currentToken;
    /* current state - always begins at START */
    StateType state = START;
    /* flag to indicate save to tokenString */
    // 指示是否将一个字符增加到 tokenString 上（标识符或保留字）    标识符长度小于 40
    int save;
    while (state != DONE)
    {
        int c = getNextChar();
        save = TRUE;
        switch (state)
        {
        case START:
            if (isdigit(c))
                state = INNUM;
            else if (isalpha(c))
                state = INID;
            else if (c == ':')
                state = INASSIGN;
            else if ((c == ' ') || (c == '\t') || (c == '\n'))
            {
                save = FALSE;
            }
            else if (c == '/')
            {
                save = FALSE;
                state = INCOMMENT1;
            }
            else if (c == '<') // 进入 INLE，可能是 LE(<=)
            {
                save = FALSE;
                state = INLE;
            }
            else if (c == '>')
            {
                save = FALSE;
                state = INRE;
            }
            else if (c == '!')
            {
                save = FALSE;
                state = INNE;
            }
            else if (c == '=')
            {
                save = FALSE;
                state = INEQ;
            }
            else // other
            {
                state = DONE;
                switch (c)
                {
                case EOF:
                    save = FALSE;
                    currentToken = ENDFILE;
                    break;
                case '=':
                    currentToken = ASSIGN; //
                    break;
                case '<':
                    currentToken = LT;
                    break;
                case '>':
                    currentToken = RT;
                    break;
                case '+':
                    currentToken = PLUS;
                    break;
                case '-':
                    currentToken = MINUS;
                    break;
                case '*':
                    currentToken = TIMES;
                    break;
                case '/':
                    currentToken = OVER;
                    break;
                case '(':
                    currentToken = LPAREN;
                    break;
                case ')':
                    currentToken = RPAREN;
                    break;
                case '{':
                    currentToken = LBRACE;
                    break;
                case '}':
                    currentToken = RBRACE;
                    break;
                case ';':
                    currentToken = SEMI;
                    break;
                case ',':
                    currentToken = COMMA;
                    break;
                default:
                    currentToken = ERROR;
                    break;
                }
            }
            break;
        case INLE:
            save = FALSE;
            state = DONE; // INLE 状态下一定会返回 LE 或 LT
            if (c == '=')
            {
                currentToken = LE;
            }
            else
            { // < 后的下一个字符不是 = 表示是 LT(<)
                currentToken = LT;
                ungetNextChar(); // 回退
            }
            break;
        case INRE:
            save = FALSE;
            state = DONE;
            if (c == '=')
            {
                currentToken = RE;
            }
            else
            {
                currentToken = RT;
                ungetNextChar();
            }
            break;
        case INNE:
            save = FALSE;
            state = DONE;
            if (c == '=')
            {
                currentToken = NE;
            }
            else // 只读取到 ！ 而没有 = ，返回 ERROR
            {
                currentToken = ERROR;
                ungetNextChar();
            }
            break;
        case INEQ:
            save = FALSE;
            state = DONE;
            if (c == '=')
            {
                currentToken = EQ;
            }
            else
            { // 只读取到 = 而没有第二个 =，返回 ASSIGN, 并回退
                currentToken = ASSIGN;
                ungetNextChar();
            }
            break;
        case INCOMMENT1:
            save = FALSE;
            if (c == EOF)
            {
                state = DONE;
                currentToken = ENDFILE;
            }
            else if (c == '*')
            {
                state = INCOMMENT2;
            }
            else
            { // INCOMMENT1 状态下没有接收到 / 则表示是除号
                ungetNextChar();
                currentToken = OVER;
            }
            break;
        case INCOMMENT2: // 已接受 /* 应能循环接受其他字符
            save = FALSE;
            if (c == EOF)
            {
                state = DONE;
                currentToken = ENDFILE;
            }
            else if (c == '*')
            {
                state = INCOMMENT3;
            }
            break; // { .. } -> /* ... */
        case INCOMMENT3:
            save = FALSE;
            if (c == '/')
            {
                // fprintf(listing, "annotation over");
                state = START;
            }
            else
            {
                ungetNextChar();
                currentToken = ERROR;
            }
            break;
        case INNUM:
            if (!isdigit(c)) // 读取到不是数字的字符后结束读取
            {                /* backup in the input */
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = NUM;
            }
            break;
        case INID:
            if (!isalpha(c)) // 读取到不是字母的字符后结束读取
            {                /* backup in the input */
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = ID;
            }
            break;
        case DONE:
        default: /* should never happen */
            fprintf(listing, "Scanner Bug: state= %d\n", state);
            state = DONE;
            currentToken = ERROR;
            break;
        }
        if ((save) && (tokenStringIndex <= MAXTOKENLEN))
            // fprintf(listing, "save: %c\n", c);
            tokenString[tokenStringIndex++] = (char)c;
        if (state == DONE)
        {
            tokenString[tokenStringIndex] = '\0';
            if (currentToken == ID) // 检查读取到的 token 是否是保留字
                currentToken = reservedLookup(tokenString);
        }
    }
    if (TraceScan)
    {
        fprintf(listing, "\t%d: ", lineno);
        
        printToken(currentToken, tokenString);
    }
    // fprintf(listing, "get token: %s", currentToken);
    return currentToken;
} /* end getToken */
