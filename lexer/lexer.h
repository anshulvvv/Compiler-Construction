/*
    #GROUP - 16
    2021B4A71746P Sugeet Sood
    2021B5A70900P Anshul Maheshwari
    2021B5A71144P Nimish Sharma
    2021B5A70925P Dhruv Bhandari
    2021B4A70817P Prakhar Agrawal
    2021A7PS1452P Aaradhya Kulshrestha
*/

#ifndef LEXER_PROTOTYPES
#define LEXER_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "lexerDef.h"

TrieNode *initTrieNode(void);
Trie *initTrie(void);
void addKeyword(Trie *trie, const char *word, Token tokenType);
Token lookupKeyword(Trie *trie, const char *word);

SymbolTable *initSymbolTable(void);
void addSymbolEntry(SymbolTable *table, SymbolTableEntry *entry);
SymbolTableEntry *newSymbolEntry(char *lexeme, Token tokenType, double numVal);
SymbolTableEntry *findSymbolEntry(SymbolTable *table, char *lexeme);
linkedList *initTokenList(void);
tokenInfo *newTokenNode(SymbolTableEntry *entry, int lineNo);
void appendTokenNode(linkedList *list, tokenInfo *node);
linkedList *LexInput(FILE *fp, char *outp);
void removeComments(char *testFile, char *cleanFile);
void printCleanFile(char *cleanFile);
linkedList *tokenizeFile(FILE *fp);
void populateKeywordsTrie(Trie *keywordsTrie);
tokenInfo *fetchNextToken(FILE *fp, char *buffer, int *fwdPtr, int *lineNo, Trie *keywordsTrie, SymbolTable *symTable);
void extractLexeme(int beginPtr, int *fwdPtr, char *lexeme, char *buffer);
char advanceChar(FILE *fp, char *buffer, int *fwdPtr);
void initTokenStrings(void);
void displayTokens(linkedList *tokenList);

#endif
