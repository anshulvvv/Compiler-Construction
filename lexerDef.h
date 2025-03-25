#ifndef LEXERDEF_H
#define LEXERDEF_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define LENGTH_MAX_VAR_ID 20
#define SIZE_OF_BUFFER 50
#define LENGTH_MAX_FIELD_ID 20
#define TOK_NAME_LEN 50
#define COUNT_VOC ((int)MAX_VOC_SIZE)
#define TERM_CNT ((int)MAX_VOC_SIZE)
#define LEN_FUNID_MAX 30
#define FNAME_MAX 50
#define LEX_MAX LEN_FUNID_MAX

typedef enum vocabulary Vocabulary;
typedef struct trie *Trie;
typedef struct tokenInfo *tokenInfo;
typedef struct twinBuffer *twinBuffer;
typedef struct trieEdge *TrieEdge;

extern int state;
extern bool canPrint;
extern char parseTOutFile[FNAME_MAX];
extern char tokenListFile[FNAME_MAX];
extern int currentLineNumber;
extern twinBuffer buffer;
extern Trie symbolTable;
extern char commFreeFile[FNAME_MAX];
extern char loefile[FNAME_MAX];
extern FILE **fptrs;
extern int fptrsLen;
extern char tstfile[FNAME_MAX];

enum vocabulary
{
    TK_NUM,
    TK_GT,
    TK_ENDWHILE,
    TK_ELSE,
    TK_GE,
    TK_FUNID,
    TK_UNION,
    TK_LIST,
    TK_RNUM,
    TK_REAL,
    TK_NOT,
    TK_ENDIF,
    TK_COLON,
    EPS,
    TK_SQR,
    TK_THEN,
    TK_ASSIGNOP,
    TK_LT,
    TK_READ,
    TK_AND,
    TK_RECORD,
    TK_END,
    TK_OUTPUT,
    TK_COMMENT,
    TK_MINUS,
    TK_EQ,
    TK_PARAMETER,
    TK_TYPE,
    TK_WHILE,
    TK_GLOBAL,
    TK_SEM,
    TK_RUID,
    TK_OP,
    TK_ENDUNION,
    TK_DEFINETYPE,
    TK_PLUS,
    TK_MUL,
    TK_RETURN,
    TK_LE,
    TK_COMMA,
    TK_CL,
    TK_DOLLAR,
    TK_NE,
    TK_MAIN,
    TK_IF,
    TK_DOT,
    TK_FIELDID,
    TK_INT,
    TK_AS,
    TK_INPUT,
    TK_OR,
    TK_ID,
    TK_ENDRECORD,
    TK_WITH,
    TK_PARAMETERS,
    TK_SQL,
    TK_WRITE,
    TK_DIV,
    TK_CALL,
    LEXICAL_ERROR,
    ID_LENGTH_EXC,
    FUN_LENGTH_EXC,
    VAR_LENGTH_EXC,
    MAX_VOC_SIZE
};

const static struct
{
    Vocabulary val;
    const char *str;
} conversion[] = {
    {TK_DIV, "TK_DIV"},
    {TK_PLUS, "TK_PLUS"},
    {TK_NUM, "TK_NUM"},
    {TK_GT, "TK_GT"},
    {TK_DEFINETYPE, "TK_DEFINETYPE"},
    {TK_OUTPUT, "TK_OUTPUT"},
    {TK_PARAMETER, "TK_PARAMETER"},
    {TK_TYPE, "TK_TYPE"},
    {TK_IF, "TK_IF"},
    {TK_DOT, "TK_DOT"},
    {TK_FIELDID, "TK_FIELDID"},
    {TK_INT, "TK_INT"},
    {TK_AS, "TK_AS"},
    {TK_INPUT, "TK_INPUT"},
    {TK_OP, "TK_OP"},
    {TK_ENDUNION, "TK_ENDUNION"},
    {TK_RECORD, "TK_RECORD"},
    {TK_END, "TK_END"},
    {TK_CALL, "TK_CALL"},
    {TK_MINUS, "TK_MINUS"},
    {TK_EQ, "TK_EQ"},
    {TK_MAIN, "TK_MAIN"},
    {VAR_LENGTH_EXC, "VAR_LENGTH_EXC"},
    {TK_COMMA, "TK_COMMA"},
    {TK_MUL, "TK_MUL"},
    {TK_RETURN, "TK_RETURN"},
    {TK_REAL, "TK_REAL"},
    {TK_PARAMETERS, "TK_PARAMETERS"},
    {TK_WITH, "TK_WITH"},
    {TK_COLON, "TK_COLON"},
    {EPS, "EPS"},
    {TK_ID, "TK_ID"},
    {TK_LE, "TK_LE"},
    {TK_ENDWHILE, "TK_ENDWHILE"},
    {TK_ELSE, "TK_ELSE"},
    {TK_SQR, "TK_SQR"},
    {FUN_LENGTH_EXC, "FUN_LENGTH_EXC"},
    {TK_SQL, "TK_SQL"},
    {TK_WRITE, "TK_WRITE"},
    {TK_SEM, "TK_SEM"},
    {TK_RUID, "TK_RUID"},
    {TK_COMMENT, "TK_COMMENT"},
    {TK_THEN, "TK_THEN"},
    {TK_ASSIGNOP, "TK_ASSIGNOP"},
    {TK_WHILE, "TK_WHILE"},
    {TK_GLOBAL, "TK_GLOBAL"},
    {TK_OR, "TK_OR"},
    {TK_ENDRECORD, "TK_ENDRECORD"},
    {TK_NE, "TK_NE"},
    {TK_NOT, "TK_NOT"},
    {TK_ENDIF, "TK_ENDIF"},
    {LEXICAL_ERROR, "LEXICAL_ERROR"},
    {ID_LENGTH_EXC, "ID_LENGTH_EXC"},
    {TK_FUNID, "TK_FUNID"},
    {TK_UNION, "TK_UNION"},
    {TK_LIST, "TK_LIST"},
    {TK_RNUM, "TK_RNUM"},
    {TK_CL, "TK_CL"},
    {TK_DOLLAR, "TK_DOLLAR"},
    {TK_GE, "TK_GE"},
    {TK_LT, "TK_LT"},
    {TK_READ, "TK_READ"},
    {TK_AND, "TK_AND"},
};

struct trieEdge
{
    char ch;
    Trie ptr;
};

struct trie
{
    int childCount;
    void *data;
    TrieEdge *children;
};

struct twinBuffer
{
    char first[SIZE_OF_BUFFER];
    char second[SIZE_OF_BUFFER];
    FILE *fp;
    int lexemeBegin;
    int forward;
    int fileBufEndsHere;
};

struct tokenInfo
{
    char lexeme[LEX_MAX];
    int lineNumber;
    Vocabulary tokenName;
};

typedef struct llnode
{
    tokenInfo symTBEntry;
    struct llnode *next;
} lln;

typedef struct linkedList
{
    int count;
    lln *head, *tail;
} linkedList;

#endif