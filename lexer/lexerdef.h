/*
    #GROUP - 16
    2021B4A71746P Sugeet Sood
    2021B5A70900P Anshul Maheshwari
    2021B5A71144P Nimish Sharma
    2021B5A70925P Dhruv Bhandari
    2021B4A70817P Prakhar Agrawal
    2021A7PS1452P Aaradhya Kulshrestha
*/

#ifndef LEXER_DEF
#define LEXER_DEF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALPHABET_SIZE 26
#define INITIAL_SYMBOL_TABLE_CAPACITY 10
#define BUFFER_SIZE 256
#define TOKEN_NAME_LENGTH 50

typedef enum Token
{
    ASSIGNOP,
    COMMENT,
    FIELDID,
    ID,
    NUM,
    RNUM,
    FUNID,
    RUID,
    WITH,
    PARAMETERS,
    END,
    WHILE,
    UNION,
    ENDUNION,
    DEFINETYPE,
    AS,
    TYPE,
    MAIN,
    GLOBAL,
    PARAMETER,
    LIST,
    SQL,
    SQR,
    INPUT,
    OUTPUT,
    INT,
    REAL,
    COMMA,
    SEM,
    COLON,
    DOT,
    ENDWHILE,
    OP,
    CL,
    IF,
    THEN,
    ENDIF,
    READ,
    WRITE,
    RETURN,
    PLUS,
    MINUS,
    MUL,
    DIV,
    CALL,
    RECORD,
    ENDRECORD,
    ELSE,
    AND,
    OR,
    NOT,
    LT,
    LE,
    EQ,
    GT,
    GE,
    NE,
    EPS,
    DOLLAR, // End of File marker
    LEXICAL_ERROR,
    ID_LENGTH_EXC,
    FUN_LENGTH_EXC,
    TK_NOT_FOUND
} Token;

extern char *tokenNames[TK_NOT_FOUND];
extern bool tokenStringsInit;
extern bool specialFlag;

typedef struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    int isEndOfWord;
    Token tokenType;
} TrieNode;

typedef struct Trie
{
    TrieNode *root;
} Trie;

typedef struct SymbolTableEntry
{
    char lexeme[BUFFER_SIZE];
    Token tokenType;
    double valueIfNumber;
} SymbolTableEntry;

typedef struct SymbolTable
{
    int capacity, size;
    SymbolTableEntry **table;
} SymbolTable;

typedef struct llnode
{
    SymbolTableEntry *STE;
    int lineNumber;
    struct llnode *next;
} tokenInfo;

typedef struct linkedList
{
    int count;
    tokenInfo *head, *tail;
} linkedList;

#endif
