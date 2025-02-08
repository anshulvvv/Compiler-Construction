/*
    #GROUP - 16
    2021B4A71746P Sugeet Sood
    2021B5A70900P Anshul Maheshwari
    2021B5A71144P Nimish Sharma
    2021B5A70925P Dhruv Bhandari
    2021B4A70817P Prakhar Agrawal
    2021A7PS1452P Aaradhya Kulshrestha
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lexer.h"
#include "lexerDef.h"

char *tokenNames[TK_NOT_FOUND];
bool specialFlag = false;
bool tokenStringsInit = false;

TrieNode *initTrieNode(void)
{
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node)
    {
        node->isEndOfWord = 0;
        for (int i = 0; i < ALPHABET_SIZE; i++)
        {
            node->children[i] = NULL;
        }
    }
    else
    {
        printf("Error: Unable to allocate memory for Trie node.\n");
    }
    return node;
}

Trie *initTrie(void)
{
    Trie *trie = (Trie *)malloc(sizeof(Trie));
    if (trie)
    {
        trie->root = initTrieNode();
    }
    else
    {
        printf("Error: Unable to allocate memory for Trie.\n");
    }
    return trie;
}
void addKeyword(Trie *trie, const char *word, Token tokenType)
{
    TrieNode *current = trie->root;
    for (int i = 0; word[i] != '\0'; i++)
    {
        int idx = word[i] - 'a';
        if (!current->children[idx])
        {
            current->children[idx] = initTrieNode();
        }
        current = current->children[idx];
    }
    current->isEndOfWord = 1;
    current->tokenType = tokenType;
}

Token lookupKeyword(Trie *trie, const char *word)
{
    TrieNode *current = trie->root;
    for (int i = 0; word[i] != '\0'; i++)
    {
        int idx = word[i] - 'a';
        if (!current->children[idx])
            return TK_NOT_FOUND;
        current = current->children[idx];
    }
    return (current && current->isEndOfWord) ? current->tokenType : TK_NOT_FOUND;
}

SymbolTable *initSymbolTable(void)
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    if (!table)
    {
        printf("Error: Unable to allocate memory for Symbol Table structure.\n");
        return NULL;
    }
    table->table = (SymbolTableEntry **)malloc(INITIAL_SYMBOL_TABLE_CAPACITY * sizeof(SymbolTableEntry *));
    if (!table->table)
    {
        printf("Error: Unable to allocate memory for Symbol Table entries.\n");
    }
    table->capacity = INITIAL_SYMBOL_TABLE_CAPACITY;
    table->size = 0;
    return table;
}

void addSymbolEntry(SymbolTable *table, SymbolTableEntry *entry)
{
    if (table->size == table->capacity)
    {
        table->capacity *= 2;
        table->table = (SymbolTableEntry **)realloc(table->table, table->capacity * sizeof(SymbolTableEntry *));
        if (!table->table)
        {
            printf("Error: Memory reallocation failed for Symbol Table.\n");
            return;
        }
    }
    table->table[(table->size)++] = entry;
}

SymbolTableEntry *newSymbolEntry(char *lexeme, Token tokenType, double numVal)
{
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    if (!entry)
    {
        printf("Error: Unable to allocate memory for new token.\n");
        return NULL;
    }
    strcpy(entry->lexeme, lexeme);
    entry->tokenType = tokenType;
    entry->valueIfNumber = numVal;
    return entry;
}

SymbolTableEntry *findSymbolEntry(SymbolTable *table, char *lexeme)
{
    for (int i = 0; i < table->size; i++)
        if (strcmp(lexeme, table->table[i]->lexeme) == 0)
            return table->table[i];
    return NULL;
}

linkedList *initTokenList(void)
{
    linkedList *list = (linkedList *)malloc(sizeof(linkedList));
    if (!list)
    {
        printf("Error: Unable to allocate memory for token list.\n");
        return NULL;
    }
    list->count = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

tokenInfo *newTokenNode(SymbolTableEntry *entry, int lineNo)
{
    tokenInfo *node = (tokenInfo *)malloc(sizeof(tokenInfo));
    if (!node)
    {
        printf("Error: Unable to allocate memory for token node.\n");
        return NULL;
    }
    node->STE = entry;
    node->lineNumber = lineNo;
    node->next = NULL;
    return node;
}

void appendTokenNode(linkedList *list, tokenInfo *node)
{
    if (list->count == 0)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }
    list->count++;
}

char advanceChar(FILE *fp, char *buffer, int *fwdPtr)
{
    if (*fwdPtr == BUFFER_SIZE - 1 && !specialFlag)
    {
        if (feof(fp))
            buffer[BUFFER_SIZE] = '\0';
        else
        {
            int bytes = fread(buffer + BUFFER_SIZE, sizeof(char), BUFFER_SIZE, fp);
            if (bytes < BUFFER_SIZE * sizeof(char))
                buffer[BUFFER_SIZE + bytes] = '\0';
        }
    }
    else if (*fwdPtr == (2 * BUFFER_SIZE - 1) && !specialFlag)
    {
        if (feof(fp))
            buffer[0] = '\0';
        else
        {
            int bytes = fread(buffer, sizeof(char), BUFFER_SIZE, fp);
            if (bytes < BUFFER_SIZE * sizeof(char))
                buffer[bytes] = '\0';
        }
    }
    if (specialFlag)
        specialFlag = false;

    *fwdPtr = (*fwdPtr + 1) % (2 * BUFFER_SIZE);
    return buffer[*fwdPtr];
}

void extractLexeme(int beginPtr, int *fwdPtr, char *lexeme, char *buffer)
{
    if (*fwdPtr >= beginPtr)
    {
        int i;
        for (i = 0; i <= (*fwdPtr - beginPtr); i++)
            lexeme[i] = buffer[beginPtr + i];
        lexeme[i] = '\0';
    }
    else
    {
        int i;
        for (i = 0; i <= (*fwdPtr + 2 * BUFFER_SIZE - beginPtr); i++)
            lexeme[i] = buffer[(beginPtr + i) % (2 * BUFFER_SIZE)];
        lexeme[i] = '\0';
    }
}

int lexemeLength(int beginPtr, int fwdPtr)
{
    if (fwdPtr >= beginPtr)
        return (fwdPtr - beginPtr + 1);
    else
        return (2 * BUFFER_SIZE + fwdPtr - beginPtr + 1);
}

tokenInfo *fetchNextToken(FILE *fp, char *buffer, int *fwdPtr, int *lineNo, Trie *keywordsTrie, SymbolTable *symTable)
{
    tokenInfo *tokenNode;
    int beginPtr = (*fwdPtr + 1) % (2 * BUFFER_SIZE);
    int state = 0;
    char ch;
    char lexeme[BUFFER_SIZE];

    while (true)
    {
        switch (state)
        {
        case 0:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '\n')
            {
                ++(*lineNo);
                beginPtr = (beginPtr + 1) % (2 * BUFFER_SIZE);
            }
            else if (ch == ' ' || ch == '\t' || ch == '\r')
            {
                beginPtr = (beginPtr + 1) % (2 * BUFFER_SIZE);
            }
            else if (isdigit(ch))
                state = 5;
            else if (ch == '%')
                state = 65;
            else if (ch == '>')
                state = 56;
            else if (ch == '<')
                state = 53;
            else if (ch == '=')
                state = 68;
            else if (ch == '_')
                state = 69;
            else if (ch == '/')
                state = 45;
            else if (ch == '@')
                state = 73;
            else if (ch == '*')
                state = 44;
            else if (ch == '#')
                state = 75;
            else if (ch == '[')
                state = 23;
            else if (ch == ']')
                state = 24;
            else if (ch == ',')
                state = 29;
            else if (ch == ':')
                state = 31;
            else if (ch == ';')
                state = 30;
            else if (ch == '+')
                state = 42;
            else if (ch == ')')
                state = 35;
            else if (ch == '-')
                state = 43;
            else if (ch == '(')
                state = 34;
            else if (ch == '&')
                state = 76;
            else if (ch == '~')
                state = 52;
            else if (ch == '!')
                state = 78;
            else if (ch == 'a' || (ch > 'd' && ch <= 'z'))
                state = 3;
            else if (ch >= 'b' && ch <= 'd')
                state = 79;
            else if (ch == '.')
                state = 59;
            else if (ch == '\0')
            {
                lexeme[0] = ch;
                SymbolTableEntry *dollarEntry = newSymbolEntry(lexeme, DOLLAR, 0);
                tokenInfo *dollarToken = newTokenNode(dollarEntry, *lineNo);
                return dollarToken;
            }
            else
            {
                lexeme[0] = ch;
                lexeme[1] = '\0';
                SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                if (!errEntry)
                {
                    errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                    addSymbolEntry(symTable, errEntry);
                }
                tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                return errToken;
            }
            break;

        case 1:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *assignEntry = findSymbolEntry(symTable, lexeme);
                if (!assignEntry)
                {
                    assignEntry = newSymbolEntry(lexeme, ASSIGNOP, 0);
                    addSymbolEntry(symTable, assignEntry);
                }
                tokenInfo *assignToken = newTokenNode(assignEntry, *lineNo);
                return assignToken;
            }
            break;

        case 2:
            // (Comment handling code removed as in original)
            break;

        case 3:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (!islower(ch))
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *entry = findSymbolEntry(symTable, lexeme);
                    if (!entry)
                    {
                        Token tokenKind = lookupKeyword(keywordsTrie, lexeme);
                        if (tokenKind == TK_NOT_FOUND)
                        {
                            SymbolTableEntry *fieldEntry = newSymbolEntry(lexeme, FIELDID, 0);
                            addSymbolEntry(symTable, fieldEntry);
                            tokenInfo *fieldToken = newTokenNode(fieldEntry, *lineNo);
                            return fieldToken;
                        }
                        else
                        {
                            SymbolTableEntry *kwEntry = newSymbolEntry(lexeme, tokenKind, 0);
                            addSymbolEntry(symTable, kwEntry);
                            tokenInfo *kwToken = newTokenNode(kwEntry, *lineNo);
                            return kwToken;
                        }
                    }
                    else
                    {
                        tokenInfo *tokenNode = newTokenNode(entry, *lineNo);
                        return tokenNode;
                    }
                }
            }
            break;

        case 4:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch < '2' || ch > '7')
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *idEntry = findSymbolEntry(symTable, lexeme);
                    if (!idEntry)
                    {
                        idEntry = newSymbolEntry(lexeme, ID, 0);
                        addSymbolEntry(symTable, idEntry);
                    }
                    tokenInfo *idToken = newTokenNode(idEntry, *lineNo);
                    return idToken;
                }
            }
            if (lexemeLength(beginPtr, *fwdPtr) > 20)
            {
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                lexeme[20] = '.';
                lexeme[21] = '.';
                lexeme[22] = '.';
                lexeme[23] = '\0';
                {
                    SymbolTableEntry *idLenEntry = findSymbolEntry(symTable, lexeme);
                    if (!idLenEntry)
                    {
                        idLenEntry = newSymbolEntry(lexeme, ID_LENGTH_EXC, 0);
                        addSymbolEntry(symTable, idLenEntry);
                    }
                    tokenInfo *idLenToken = newTokenNode(idLenEntry, *lineNo);
                    ch = advanceChar(fp, buffer, fwdPtr);
                    for (; ch >= '2' && ch <= '7'; ch = advanceChar(fp, buffer, fwdPtr))
                        ;
                    (*fwdPtr)--;
                    if (*fwdPtr < 0)
                        *fwdPtr += 2 * BUFFER_SIZE;
                    if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                        specialFlag = true;
                    return idLenToken;
                }
            }
            break;

        case 5:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '.')
                state = 60;
            else if (!isdigit(ch))
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *numEntry = findSymbolEntry(symTable, lexeme);
                    if (numEntry)
                    {
                        tokenInfo *numToken = newTokenNode(numEntry, *lineNo);
                        return numToken;
                    }
                    double value = 0;
                    for (int i = 0; lexeme[i]; i++)
                        value = value * 10 + (lexeme[i] - '0');
                    numEntry = newSymbolEntry(lexeme, NUM, value);
                    addSymbolEntry(symTable, numEntry);
                    tokenInfo *numToken = newTokenNode(numEntry, *lineNo);
                    return numToken;
                }
            }
            break;

        case 6:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == 'E')
                state = 62;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *rnumEntry = findSymbolEntry(symTable, lexeme);
                    if (rnumEntry)
                    {
                        tokenInfo *rnumToken = newTokenNode(rnumEntry, *lineNo);
                        return rnumToken;
                    }
                    double value = 0;
                    int i;
                    for (i = 0; lexeme[i] != '.'; i++)
                        value = value * 10 + (lexeme[i] - '0');
                    value += (lexeme[i + 1] - '0') / 10.0 + (lexeme[i + 2] - '0') / 100.0;
                    rnumEntry = newSymbolEntry(lexeme, RNUM, value);
                    addSymbolEntry(symTable, rnumEntry);
                    tokenInfo *rnumToken = newTokenNode(rnumEntry, *lineNo);
                    return rnumToken;
                }
            }
            break;

        case 7:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *rnumEntry = findSymbolEntry(symTable, lexeme);
                if (rnumEntry)
                {
                    tokenInfo *rnumToken = newTokenNode(rnumEntry, *lineNo);
                    return rnumToken;
                }
                double val = 0;
                int i;
                for (i = 0; lexeme[i] != '.'; i++)
                    val = val * 10 + (lexeme[i] - '0');
                val += (lexeme[i + 1] - '0') / 10.0 + (lexeme[i + 2] - '0') / 100.0;
                int exp = 0;
                i += 4;
                if (isdigit(lexeme[i]))
                    exp = lexeme[i] * 10 + lexeme[i + 1];
                else
                    exp = lexeme[i + 1] * 10 + lexeme[i + 2];
                if (lexeme[i] == '-')
                    exp = -exp;
                val *= pow(10, exp);
                rnumEntry = newSymbolEntry(lexeme, RNUM, val);
                addSymbolEntry(symTable, rnumEntry);
                tokenInfo *rnumToken = newTokenNode(rnumEntry, *lineNo);
                return rnumToken;
            }
            break;

        case 8:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 81;
            else if (!isalpha(ch))
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *fnEntry = findSymbolEntry(symTable, lexeme);
                    if (fnEntry)
                    {
                        tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                        return fnToken;
                    }
                    fnEntry = newSymbolEntry(lexeme, FUNID, 0);
                    addSymbolEntry(symTable, fnEntry);
                    tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                    return fnToken;
                }
            }
            if (lexemeLength(beginPtr, *fwdPtr) > 30)
            {
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                lexeme[30] = '.';
                lexeme[31] = '.';
                lexeme[32] = '.';
                lexeme[33] = '\0';
                {
                    SymbolTableEntry *fnLenEntry = findSymbolEntry(symTable, lexeme);
                    if (!fnLenEntry)
                    {
                        fnLenEntry = newSymbolEntry(lexeme, FUN_LENGTH_EXC, 0);
                        addSymbolEntry(symTable, fnLenEntry);
                    }
                    tokenInfo *fnLenToken = newTokenNode(fnLenEntry, *lineNo);
                    ch = advanceChar(fp, buffer, fwdPtr);
                    for (; isalpha(ch); ch = advanceChar(fp, buffer, fwdPtr))
                        ;
                    (*fwdPtr)--;
                    if (*fwdPtr < 0)
                        *fwdPtr += 2 * BUFFER_SIZE;
                    if (isdigit(ch))
                    {
                        state = 81;
                        break;
                    }
                    return fnLenToken;
                }
            }
            break;

        case 9:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (!islower(ch))
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *ruidEntry = findSymbolEntry(symTable, lexeme);
                    if (!ruidEntry)
                    {
                        ruidEntry = newSymbolEntry(lexeme, RUID, 0);
                        addSymbolEntry(symTable, ruidEntry);
                    }
                    tokenInfo *ruidToken = newTokenNode(ruidEntry, *lineNo);
                    return ruidToken;
                }
            }
            break;

        case 19:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isalpha(ch))
                state = 8;
            else if (isdigit(ch))
                state = 81;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *mainEntry = findSymbolEntry(symTable, lexeme);
                    if (!mainEntry)
                    {
                        mainEntry = newSymbolEntry(lexeme, MAIN, 0);
                        addSymbolEntry(symTable, mainEntry);
                    }
                    tokenInfo *mainToken = newTokenNode(mainEntry, *lineNo);
                    return mainToken;
                }
            }
            break;

        case 23:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *sqlEntry = findSymbolEntry(symTable, lexeme);
                if (!sqlEntry)
                {
                    sqlEntry = newSymbolEntry(lexeme, SQL, 0);
                    addSymbolEntry(symTable, sqlEntry);
                }
                tokenInfo *sqlToken = newTokenNode(sqlEntry, *lineNo);
                return sqlToken;
            }
            break;

        case 24:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *sqrEntry = findSymbolEntry(symTable, lexeme);
                if (!sqrEntry)
                {
                    sqrEntry = newSymbolEntry(lexeme, SQR, 0);
                    addSymbolEntry(symTable, sqrEntry);
                }
                tokenInfo *sqrToken = newTokenNode(sqrEntry, *lineNo);
                return sqrToken;
            }
            break;

        case 29:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *commaEntry = findSymbolEntry(symTable, lexeme);
                if (!commaEntry)
                {
                    commaEntry = newSymbolEntry(lexeme, COMMA, 0);
                    addSymbolEntry(symTable, commaEntry);
                }
                tokenInfo *commaToken = newTokenNode(commaEntry, *lineNo);
                return commaToken;
            }
            break;

        case 30:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *semiEntry = findSymbolEntry(symTable, lexeme);
                if (!semiEntry)
                {
                    semiEntry = newSymbolEntry(lexeme, SEM, 0);
                    addSymbolEntry(symTable, semiEntry);
                }
                tokenInfo *semiToken = newTokenNode(semiEntry, *lineNo);
                return semiToken;
            }
            break;

        case 31:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *colonEntry = findSymbolEntry(symTable, lexeme);
                if (!colonEntry)
                {
                    colonEntry = newSymbolEntry(lexeme, COLON, 0);
                    addSymbolEntry(symTable, colonEntry);
                }
                tokenInfo *colonToken = newTokenNode(colonEntry, *lineNo);
                return colonToken;
            }
            break;

        case 34:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *openEntry = findSymbolEntry(symTable, lexeme);
                if (!openEntry)
                {
                    openEntry = newSymbolEntry(lexeme, OP, 0);
                    addSymbolEntry(symTable, openEntry);
                }
                tokenInfo *openToken = newTokenNode(openEntry, *lineNo);
                return openToken;
            }
            break;

        case 35:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *closeEntry = findSymbolEntry(symTable, lexeme);
                if (!closeEntry)
                {
                    closeEntry = newSymbolEntry(lexeme, CL, 0);
                    addSymbolEntry(symTable, closeEntry);
                }
                tokenInfo *closeToken = newTokenNode(closeEntry, *lineNo);
                return closeToken;
            }
            break;

        case 42:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *plusEntry = findSymbolEntry(symTable, lexeme);
                if (!plusEntry)
                {
                    plusEntry = newSymbolEntry(lexeme, PLUS, 0);
                    addSymbolEntry(symTable, plusEntry);
                }
                tokenInfo *plusToken = newTokenNode(plusEntry, *lineNo);
                return plusToken;
            }
            break;

        case 43:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *minusEntry = findSymbolEntry(symTable, lexeme);
                if (!minusEntry)
                {
                    minusEntry = newSymbolEntry(lexeme, MINUS, 0);
                    addSymbolEntry(symTable, minusEntry);
                }
                tokenInfo *minusToken = newTokenNode(minusEntry, *lineNo);
                return minusToken;
            }
            break;

        case 44:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *mulEntry = findSymbolEntry(symTable, lexeme);
                if (!mulEntry)
                {
                    mulEntry = newSymbolEntry(lexeme, MUL, 0);
                    addSymbolEntry(symTable, mulEntry);
                }
                tokenInfo *mulToken = newTokenNode(mulEntry, *lineNo);
                return mulToken;
            }
            break;

        case 45:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *divEntry = findSymbolEntry(symTable, lexeme);
                if (!divEntry)
                {
                    divEntry = newSymbolEntry(lexeme, DIV, 0);
                    addSymbolEntry(symTable, divEntry);
                }
                tokenInfo *divToken = newTokenNode(divEntry, *lineNo);
                return divToken;
            }
            break;

        case 50:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *andEntry = findSymbolEntry(symTable, lexeme);
                if (!andEntry)
                {
                    andEntry = newSymbolEntry(lexeme, AND, 0);
                    addSymbolEntry(symTable, andEntry);
                }
                tokenInfo *andToken = newTokenNode(andEntry, *lineNo);
                return andToken;
            }
            break;

        case 51:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *orEntry = findSymbolEntry(symTable, lexeme);
                if (!orEntry)
                {
                    orEntry = newSymbolEntry(lexeme, OR, 0);
                    addSymbolEntry(symTable, orEntry);
                }
                tokenInfo *orToken = newTokenNode(orEntry, *lineNo);
                return orToken;
            }
            break;

        case 52:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *notEntry = findSymbolEntry(symTable, lexeme);
                if (!notEntry)
                {
                    notEntry = newSymbolEntry(lexeme, NOT, 0);
                    addSymbolEntry(symTable, notEntry);
                }
                tokenInfo *notToken = newTokenNode(notEntry, *lineNo);
                return notToken;
            }
            break;

        case 53:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '-')
                state = 66;
            else if (ch == '=')
                state = 54;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *ltEntry = findSymbolEntry(symTable, lexeme);
                    if (!ltEntry)
                    {
                        ltEntry = newSymbolEntry(lexeme, LT, 0);
                        addSymbolEntry(symTable, ltEntry);
                    }
                    tokenInfo *ltToken = newTokenNode(ltEntry, *lineNo);
                    return ltToken;
                }
            }
            break;

        case 54:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *leEntry = findSymbolEntry(symTable, lexeme);
                if (!leEntry)
                {
                    leEntry = newSymbolEntry(lexeme, LE, 0);
                    addSymbolEntry(symTable, leEntry);
                }
                tokenInfo *leToken = newTokenNode(leEntry, *lineNo);
                return leToken;
            }
            break;

        case 55:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *eqEntry = findSymbolEntry(symTable, lexeme);
                if (!eqEntry)
                {
                    eqEntry = newSymbolEntry(lexeme, EQ, 0);
                    addSymbolEntry(symTable, eqEntry);
                }
                tokenInfo *eqToken = newTokenNode(eqEntry, *lineNo);
                return eqToken;
            }
            break;

        case 56:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '=')
                state = 57;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *gtEntry = findSymbolEntry(symTable, lexeme);
                    if (!gtEntry)
                    {
                        gtEntry = newSymbolEntry(lexeme, GT, 0);
                        addSymbolEntry(symTable, gtEntry);
                    }
                    tokenInfo *gtToken = newTokenNode(gtEntry, *lineNo);
                    return gtToken;
                }
            }
            break;

        case 57:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *geEntry = findSymbolEntry(symTable, lexeme);
                if (!geEntry)
                {
                    geEntry = newSymbolEntry(lexeme, GE, 0);
                    addSymbolEntry(symTable, geEntry);
                }
                tokenInfo *geToken = newTokenNode(geEntry, *lineNo);
                return geToken;
            }
            break;

        case 58:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *neEntry = findSymbolEntry(symTable, lexeme);
                if (!neEntry)
                {
                    neEntry = newSymbolEntry(lexeme, NE, 0);
                    addSymbolEntry(symTable, neEntry);
                }
                tokenInfo *neToken = newTokenNode(neEntry, *lineNo);
                return neToken;
            }
            break;

        case 59:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *dotEntry = findSymbolEntry(symTable, lexeme);
                if (!dotEntry)
                {
                    dotEntry = newSymbolEntry(lexeme, DOT, 0);
                    addSymbolEntry(symTable, dotEntry);
                }
                tokenInfo *dotToken = newTokenNode(dotEntry, *lineNo);
                return dotToken;
            }
            break;

        case 60:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 61;
            else
            {
                *fwdPtr -= 2;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == BUFFER_SIZE - 2 ||
                    *fwdPtr == 2 * BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 2)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *numEntry = findSymbolEntry(symTable, lexeme);
                    if (numEntry)
                    {
                        tokenInfo *numToken = newTokenNode(numEntry, *lineNo);
                        return numToken;
                    }
                    double number = 0;
                    for (int i = 0; lexeme[i]; i++)
                        number = number * 10 + (lexeme[i] - '0');
                    numEntry = newSymbolEntry(lexeme, NUM, number);
                    addSymbolEntry(symTable, numEntry);
                    tokenInfo *numToken = newTokenNode(numEntry, *lineNo);
                    return numToken;
                }
            }
            break;

        case 61:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 6;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 62:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 64;
            else if (ch == '+' || ch == '-')
                state = 63;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 63:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 64;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 64:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 7;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 65:
            extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
            {
                SymbolTableEntry *cmntEntry = findSymbolEntry(symTable, lexeme);
                if (!cmntEntry)
                {
                    cmntEntry = newSymbolEntry(lexeme, COMMENT, 0);
                    addSymbolEntry(symTable, cmntEntry);
                }
                tokenInfo *cmntToken = newTokenNode(cmntEntry, *lineNo);
                for (; (ch = advanceChar(fp, buffer, fwdPtr)) != '\n';)
                    ;
                ++(*lineNo);
                return cmntToken;
            }
            break;

        case 66:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '-')
                state = 67;
            else
            {
                *fwdPtr -= 2;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == BUFFER_SIZE - 2 ||
                    *fwdPtr == 2 * BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 2)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *ltEntry = findSymbolEntry(symTable, lexeme);
                    if (!ltEntry)
                    {
                        ltEntry = newSymbolEntry(lexeme, LT, 0);
                        addSymbolEntry(symTable, ltEntry);
                    }
                    tokenInfo *ltToken = newTokenNode(ltEntry, *lineNo);
                    return ltToken;
                }
            }
            break;

        case 67:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '-')
                state = 1;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 68:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '=')
                state = 55;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 69:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == 'm')
                state = 70;
            else if (isalpha(ch))
                state = 8;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 70:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 81;
            else if (ch == 'a')
                state = 71;
            else if (isalpha(ch))
                state = 8;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *fnEntry = findSymbolEntry(symTable, lexeme);
                    if (!fnEntry)
                    {
                        fnEntry = newSymbolEntry(lexeme, FUNID, 0);
                        addSymbolEntry(symTable, fnEntry);
                    }
                    tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                    return fnToken;
                }
            }
            break;

        case 71:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 81;
            else if (ch == 'i')
                state = 72;
            else if (isalpha(ch))
                state = 8;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *fnEntry = findSymbolEntry(symTable, lexeme);
                    if (!fnEntry)
                    {
                        fnEntry = newSymbolEntry(lexeme, FUNID, 0);
                        addSymbolEntry(symTable, fnEntry);
                    }
                    tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                    return fnToken;
                }
            }
            break;

        case 72:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (isdigit(ch))
                state = 81;
            else if (ch == 'n')
                state = 19;
            else if (isalpha(ch))
                state = 8;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *fnEntry = findSymbolEntry(symTable, lexeme);
                    if (!fnEntry)
                    {
                        fnEntry = newSymbolEntry(lexeme, FUNID, 0);
                        addSymbolEntry(symTable, fnEntry);
                    }
                    tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                    return fnToken;
                }
            }
            break;

        case 73:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '@')
                state = 74;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 74:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '@')
                state = 51;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 75:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (islower(ch))
                state = 9;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 76:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '&')
                state = 77;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 77:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '&')
                state = 50;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 78:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch == '=')
                state = 58;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *errEntry = findSymbolEntry(symTable, lexeme);
                    if (!errEntry)
                    {
                        errEntry = newSymbolEntry(lexeme, LEXICAL_ERROR, 0);
                        addSymbolEntry(symTable, errEntry);
                    }
                    tokenInfo *errToken = newTokenNode(errEntry, *lineNo);
                    return errToken;
                }
            }
            break;

        case 79:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (islower(ch))
                state = 3;
            else if (ch >= '2' && ch <= '7')
                state = 80;
            else
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *commonEntry = findSymbolEntry(symTable, lexeme);
                    if (commonEntry)
                    {
                        tokenInfo *commonToken = newTokenNode(commonEntry, *lineNo);
                        return commonToken;
                    }
                    Token tokenKind = lookupKeyword(keywordsTrie, lexeme);
                    if (tokenKind == TK_NOT_FOUND)
                    {
                        SymbolTableEntry *fieldEntry = newSymbolEntry(lexeme, FIELDID, 0);
                        addSymbolEntry(symTable, fieldEntry);
                        tokenInfo *fieldToken = newTokenNode(fieldEntry, *lineNo);
                        return fieldToken;
                    }
                    else
                    {
                        SymbolTableEntry *kwEntry = newSymbolEntry(lexeme, tokenKind, 0);
                        addSymbolEntry(symTable, kwEntry);
                        tokenInfo *kwToken = newTokenNode(kwEntry, *lineNo);
                        return kwToken;
                    }
                }
            }
            break;

        case 80:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (ch >= '2' && ch <= '7')
                state = 4;
            else if (ch < 'b' || ch > 'd')
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *commonEntry = findSymbolEntry(symTable, lexeme);
                    if (commonEntry)
                    {
                        tokenInfo *commonToken = newTokenNode(commonEntry, *lineNo);
                        return commonToken;
                    }
                    SymbolTableEntry *idEntry = newSymbolEntry(lexeme, ID, 0);
                    addSymbolEntry(symTable, idEntry);
                    tokenInfo *idToken = newTokenNode(idEntry, *lineNo);
                    return idToken;
                }
            }
            if (lexemeLength(beginPtr, *fwdPtr) > 20)
            {
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                lexeme[20] = '.';
                lexeme[21] = '.';
                lexeme[22] = '.';
                lexeme[23] = '\0';
                {
                    SymbolTableEntry *idLenEntry = findSymbolEntry(symTable, lexeme);
                    if (!idLenEntry)
                    {
                        idLenEntry = newSymbolEntry(lexeme, ID_LENGTH_EXC, 0);
                        addSymbolEntry(symTable, idLenEntry);
                    }
                    tokenInfo *idLenToken = newTokenNode(idLenEntry, *lineNo);
                    ch = advanceChar(fp, buffer, fwdPtr);
                    for (; ch >= 'b' && ch <= 'd'; ch = advanceChar(fp, buffer, fwdPtr))
                        ;
                    (*fwdPtr)--;
                    if (*fwdPtr < 0)
                        *fwdPtr += 2 * BUFFER_SIZE;
                    if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                        specialFlag = true;
                    if (ch >= '2' && ch <= '7')
                    {
                        state = 4;
                        break;
                    }
                    return idLenToken;
                }
            }
            break;

        case 81:
            ch = advanceChar(fp, buffer, fwdPtr);
            if (!isdigit(ch))
            {
                (*fwdPtr)--;
                if (*fwdPtr < 0)
                    *fwdPtr += 2 * BUFFER_SIZE;
                if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                    specialFlag = true;
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                {
                    SymbolTableEntry *fnEntry = findSymbolEntry(symTable, lexeme);
                    if (fnEntry)
                    {
                        tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                        return fnToken;
                    }
                    fnEntry = newSymbolEntry(lexeme, FUNID, 0);
                    addSymbolEntry(symTable, fnEntry);
                    tokenInfo *fnToken = newTokenNode(fnEntry, *lineNo);
                    return fnToken;
                }
            }
            if (lexemeLength(beginPtr, *fwdPtr) > 30)
            {
                extractLexeme(beginPtr, fwdPtr, lexeme, buffer);
                lexeme[30] = '.';
                lexeme[31] = '.';
                lexeme[32] = '.';
                lexeme[33] = '\0';
                {
                    SymbolTableEntry *fnLenEntry = findSymbolEntry(symTable, lexeme);
                    if (!fnLenEntry)
                    {
                        fnLenEntry = newSymbolEntry(lexeme, FUN_LENGTH_EXC, 0);
                        addSymbolEntry(symTable, fnLenEntry);
                    }
                    tokenInfo *fnLenToken = newTokenNode(fnLenEntry, *lineNo);
                    ch = advanceChar(fp, buffer, fwdPtr);
                    for (; isdigit(ch); ch = advanceChar(fp, buffer, fwdPtr))
                        ;
                    (*fwdPtr)--;
                    if (*fwdPtr < 0)
                        *fwdPtr += 2 * BUFFER_SIZE;
                    if (*fwdPtr == BUFFER_SIZE - 1 || *fwdPtr == 2 * BUFFER_SIZE - 1)
                        specialFlag = true;
                    return fnLenToken;
                }
            }
            break;

        default:
            printf("Error: Invalid state in lexer DFA.\n");
            return NULL;
        }
    }
}

void populateKeywordsTrie(Trie *keywordsTrie)
{
    addKeyword(keywordsTrie, "with", WITH);
    addKeyword(keywordsTrie, "parameters", PARAMETERS);
    addKeyword(keywordsTrie, "end", END);
    addKeyword(keywordsTrie, "while", WHILE);
    addKeyword(keywordsTrie, "union", UNION);
    addKeyword(keywordsTrie, "endunion", ENDUNION);
    addKeyword(keywordsTrie, "definetype", DEFINETYPE);
    addKeyword(keywordsTrie, "as", AS);
    addKeyword(keywordsTrie, "type", TYPE);
    addKeyword(keywordsTrie, "global", GLOBAL);
    addKeyword(keywordsTrie, "parameter", PARAMETER);
    addKeyword(keywordsTrie, "list", LIST);
    addKeyword(keywordsTrie, "input", INPUT);
    addKeyword(keywordsTrie, "output", OUTPUT);
    addKeyword(keywordsTrie, "int", INT);
    addKeyword(keywordsTrie, "real", REAL);
    addKeyword(keywordsTrie, "endwhile", ENDWHILE);
    addKeyword(keywordsTrie, "if", IF);
    addKeyword(keywordsTrie, "then", THEN);
    addKeyword(keywordsTrie, "endif", ENDIF);
    addKeyword(keywordsTrie, "read", READ);
    addKeyword(keywordsTrie, "write", WRITE);
    addKeyword(keywordsTrie, "return", RETURN);
    addKeyword(keywordsTrie, "call", CALL);
    addKeyword(keywordsTrie, "record", RECORD);
    addKeyword(keywordsTrie, "endrecord", ENDRECORD);
    addKeyword(keywordsTrie, "else", ELSE);
}

void initTokenStrings(void)
{
    if (tokenStringsInit)
        return;
    tokenStringsInit = true;
    for (int i = 0; i < TK_NOT_FOUND; i++)
    {
        tokenNames[i] = malloc(TOKEN_NAME_LENGTH);
    }
    tokenNames[ASSIGNOP] = "TK_ASSIGNOP";
    tokenNames[COMMENT] = "TK_COMMENT";
    tokenNames[FIELDID] = "TK_FIELDID";
    tokenNames[ID] = "TK_ID";
    tokenNames[NUM] = "TK_NUM";
    tokenNames[RNUM] = "TK_RNUM";
    tokenNames[FUNID] = "TK_FUNID";
    tokenNames[RUID] = "TK_RUID";
    tokenNames[WITH] = "TK_WITH";
    tokenNames[PARAMETERS] = "TK_PARAMETERS";
    tokenNames[END] = "TK_END";
    tokenNames[WHILE] = "TK_WHILE";
    tokenNames[UNION] = "TK_UNION";
    tokenNames[ENDUNION] = "TK_ENDUNION";
    tokenNames[DEFINETYPE] = "TK_DEFINETYPE";
    tokenNames[AS] = "TK_AS";
    tokenNames[TYPE] = "TK_TYPE";
    tokenNames[MAIN] = "TK_MAIN";
    tokenNames[GLOBAL] = "TK_GLOBAL";
    tokenNames[PARAMETER] = "TK_PARAMETER";
    tokenNames[LIST] = "TK_LIST";
    tokenNames[SQL] = "TK_SQL";
    tokenNames[SQR] = "TK_SQR";
    tokenNames[INPUT] = "TK_INPUT";
    tokenNames[OUTPUT] = "TK_OUTPUT";
    tokenNames[INT] = "TK_INT";
    tokenNames[REAL] = "TK_REAL";
    tokenNames[COMMA] = "TK_COMMA";
    tokenNames[SEM] = "TK_SEM";
    tokenNames[IF] = "TK_IF";
    tokenNames[COLON] = "TK_COLON";
    tokenNames[DOT] = "TK_DOT";
    tokenNames[ENDWHILE] = "TK_ENDWHILE";
    tokenNames[OP] = "TK_OP";
    tokenNames[CL] = "TK_CL";
    tokenNames[THEN] = "TK_THEN";
    tokenNames[ENDIF] = "TK_ENDIF";
    tokenNames[READ] = "TK_READ";
    tokenNames[WRITE] = "TK_WRITE";
    tokenNames[RETURN] = "TK_RETURN";
    tokenNames[PLUS] = "TK_PLUS";
    tokenNames[MINUS] = "TK_MINUS";
    tokenNames[MUL] = "TK_MUL";
    tokenNames[DIV] = "TK_DIV";
    tokenNames[CALL] = "TK_CALL";
    tokenNames[RECORD] = "TK_RECORD";
    tokenNames[ENDRECORD] = "TK_ENDRECORD";
    tokenNames[ELSE] = "TK_ELSE";
    tokenNames[AND] = "TK_AND";
    tokenNames[OR] = "TK_OR";
    tokenNames[NOT] = "TK_NOT";
    tokenNames[LT] = "TK_LT";
    tokenNames[LE] = "TK_LE";
    tokenNames[EQ] = "TK_EQ";
    tokenNames[GT] = "TK_GT";
    tokenNames[GE] = "TK_GE";
    tokenNames[NE] = "TK_NE";
    tokenNames[EPS] = "TK_EPS";
    tokenNames[DOLLAR] = "TK_DOLLAR";
    tokenNames[LEXICAL_ERROR] = "LEXICAL_ERROR";
    tokenNames[ID_LENGTH_EXC] = "IDENTIFIER_LENGTH_EXCEEDED";
    tokenNames[FUN_LENGTH_EXC] = "FUNCTION_NAME_LENGTH_EXCEEDED";
}

linkedList *tokenizeFile(FILE *fp)
{
    char twinBuffer[BUFFER_SIZE * 2];
    int fwdPtr = 2 * BUFFER_SIZE - 1;
    int lineNumber = 1;

    Trie *keywordsTrie = initTrie();
    populateKeywordsTrie(keywordsTrie);
    initTokenStrings();

    SymbolTable *symTable = initSymbolTable();

    linkedList *tokenList = initTokenList();

    while (true)
    {
        tokenInfo *tkInfo = fetchNextToken(fp, twinBuffer, &fwdPtr, &lineNumber, keywordsTrie, symTable);
        if (!tkInfo)
        {
            printf("Error: No token retrieved.\n");
            break;
        }
        appendTokenNode(tokenList, tkInfo);
        if (tkInfo->STE->tokenType == DOLLAR)
            break;
    }
    return tokenList;
}

void removeComments(char *testFile, char *cleanFile)
{
    FILE *inFile = fopen(testFile, "r");
    if (inFile == NULL)
    {
        printf("Error: Unable to open file %s.\n", testFile);
        return;
    }

    char lineBuf[4096];
    char *commentPos;

    while (fgets(lineBuf, sizeof(lineBuf), inFile) != NULL)
    {
        commentPos = strchr(lineBuf, '%');
        if (commentPos != NULL)
        {
            *commentPos = '\n';
            *(commentPos + 1) = '\0';
        }
        printf("%s", lineBuf);
    }

    fclose(inFile);
}

void displayTokens(linkedList *tokenList)
{
    tokenInfo *current = tokenList->head;
    for (int i = 0; i < tokenList->count && current; i++)
    {
        char *tokenStr;
        if (current->STE->tokenType < LEXICAL_ERROR)
            tokenStr = strdup(tokenNames[current->STE->tokenType]);
        else if (current->STE->tokenType == LEXICAL_ERROR)
            tokenStr = strdup("Unrecognized pattern");
        else if (current->STE->tokenType == ID_LENGTH_EXC)
            tokenStr = strdup("Identifier length exceeded 20");
        else if (current->STE->tokenType == FUN_LENGTH_EXC)
            tokenStr = strdup("Function name length exceeded 30");
        else
            tokenStr = strdup("");
        printf("Line No: %*d \t Lexeme: %*s \t Token: %*s \n",
               5, current->lineNumber, 35, current->STE->lexeme, 35, tokenStr);
        current = current->next;
    }
}

linkedList *LexInput(FILE *fp, char *outp)
{
    if (!fp)
    {
        printf("Error: Unable to open input file for lexer.\n");
        exit(-1);
    }

    linkedList *tokenList = tokenizeFile(fp);
    if (!tokenList)
    {
        printf("Error: Could not retrieve tokens list.\n");
        exit(-1);
    }
    return tokenList;
}
