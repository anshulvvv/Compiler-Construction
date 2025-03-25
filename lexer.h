#ifndef LEXER_H
#define LEXER_H

#include "lexerDef.h"

extern FILE *getStream(FILE *fp);
extern tokenInfo getNextToken(twinBuffer B);
extern void remcom(char *tstfile, char *cleanFile);
extern void setupTwinBuffer();
extern void releaseTwinBuffer();
extern void setupSymbolTable();
extern void freeSymbolTable();
extern tokenInfo generateTokenInfo(Vocabulary v, char *lexeme, int lineNumber);
extern void convertEnumToString(Vocabulary v, char *dest);
extern linkedList *extractTokens(FILE *fp);
extern void freeLinkedList(linkedList *list);
extern linkedList *processLexicalTokens(FILE *fp, char *outp);

#endif