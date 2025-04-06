#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "lexerDef.h"

int state = 0;
int currentLineNumber = 1;
twinBuffer buffer = NULL;
Trie symbolTable = NULL;

FILE **fptrs;
int fptrsLen = 0;
int cnter = 0;
char parseTOutFile[FNAME_MAX] = "parse_out.txt";
char tstfile[FNAME_MAX] = "testcase.txt";
char loefile[FNAME_MAX] = "lex_error_file.txt";
char commFreeFile[FNAME_MAX] = "comment_less_file.txt";
char tokenListFile[FNAME_MAX] = "token_list_file.txt";

// function prototypes//
void setupTwinBuffer();
void releaseTwinBuffer();
void setupSymbolTable();
void freeSymbolTable();
FILE *getStream(FILE *fp);
void advanceBuffer(twinBuffer _buffer);
tokenInfo generateTokenInfo(Vocabulary tokenType, char *text, int lineNum);
void retract(twinBuffer _buffer);
void doubleRetract(twinBuffer _buffer);
char *extractLexeme(char *dest, twinBuffer _buffer);
char *updateLexemePointersAndExtract(char *Dest, twinBuffer _buffer);
int calculateLexemeLength(twinBuffer _buffer);
tokenInfo getNextToken(twinBuffer B);
void addToSymbolTable(tokenInfo tkinf, char *lexeme);
void *searchSymbolTable(char *lexeme);
void *reportVariableSizeExceeded(int lineNum, FILE *files[], int fileCount, int maxLength);
void *reportFunctionIdTooLong(int lineNum, FILE *files[], int fileCount, int maxLength);
void *reportFieldIdTooLong(int lineNum, FILE *files[], int fileCount, int maxLength);
void *reportUnknownSymbol(int lineNum, char lexeme, FILE *files[], int fileCount);
void *reportUnknownPattern(int lineNum, char *lexeme, FILE *files[], int fileCount);
linkedList *initializeLinkedList();
lln *initializeNode(tokenInfo symtbEntry);
void appendNode(linkedList *myList, lln *myNode);

//------------------//

// function definitions
linkedList *initializeLinkedList()
{
    // Allocate memory for a new linked list
    linkedList *newList = (linkedList *)malloc(sizeof(linkedList));
    if (newList == NULL)
    {
        printf("Memory allocation failed for linked list creation\n");
        return NULL;
    }
    newList->count = 0;
    newList->head = NULL;
    newList->tail = NULL;
    return newList;
}

lln *initializeNode(tokenInfo tokenData)
{
    // Allocate memory for a node that will store token details, including line number
    lln *newNode = (lln *)malloc(sizeof(lln));
    if (newNode == NULL)
    {
        printf("Memory allocation failed for linked list node\n");
        return NULL;
    }
    newNode->symTBEntry = tokenData;
    newNode->next = NULL;
    return newNode;
}

void appendNode(linkedList *list, lln *node)
{
    // Adds a new node containing token data to the linked list
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
    // Increment the node count in the list
    list->count++;
}

void freeLinkedList(linkedList *list)
{
    if (list == NULL)
    {
        return; // Nothing to free
    }

    lln *current = list->head;
    while (current != NULL)
    {
        lln *nextNode = current->next;
        free(current->symTBEntry);
        free(current);
        current = nextNode;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

char **removeComments(FILE *inputFile)
{
    char **resultLines = NULL;
    char *buffer = NULL;
    size_t bufferSize = 0;
    int bytesRead;
    int index = 0;

    while ((bytesRead = getline(&buffer, &bufferSize, inputFile)) != -1)
    {
        char *marker = strchr(buffer, '%');
        if (marker != NULL)
        {
            *marker = '\n';
            *(marker + 1) = '\0';
        }
        resultLines = realloc(resultLines, (index + 1) * sizeof(char *));
        resultLines[index] = buffer;
        buffer = NULL;
        index++;
    }
    resultLines = realloc(resultLines, (index + 1) * sizeof(char *));
    resultLines[index] = NULL;

    return resultLines;
}

char **loadAndremoveComments(char *filepath)
{
    FILE *inputFile = fopen(filepath, "r");
    if (inputFile == NULL)
    {
        printf("Error: file not found\n");
        exit(1);
    }
    char **filteredLines = removeComments(inputFile);
    fclose(inputFile);
    return filteredLines;
}

void saveWithoutComments(char **filteredLines, char *outputPath)
{
    FILE *outputFile = fopen(outputPath, "w");
    if (outputFile == NULL)
    {
        printf("Error: file could not be created\n");
        exit(1);
    }
    for (int index = 0; filteredLines[index] != NULL; index++)
    {
        fprintf(outputFile, "%s", filteredLines[index]);
        // printf("%s", filteredLines[index]);
    }
    fclose(outputFile);
}

void remcom(char *inputPath, char *outputPath)
{
    char **filteredLines = loadAndremoveComments(inputPath);
    saveWithoutComments(filteredLines, outputPath);
}

void convertEnumToString(Vocabulary value, char *output)
{
    // Iterate through the conversion array to find the matching enum value
    for (int index = 0; index < sizeof(conversion) / sizeof(conversion[0]); index++)
    {
        if (conversion[index].val == value)
        {
            strcpy(output, conversion[index].str);
            return;
        }
    }
}

const char *vocabularyToString(Vocabulary v)
{
    int convSize = sizeof(conversion) / sizeof(conversion[0]);
    for (int i = 0; i < convSize; i++)
    {
        if (conversion[i].val == v)
            return conversion[i].str;
    }
    return "UNKNOWN";
}

void printConversionMapping()
{
    int convSize = sizeof(conversion) / sizeof(conversion[0]);
    printf("Conversion Mapping:\n");
    for (int i = 0; i < convSize; i++)
    {
        printf("  %s => %d\n", conversion[i].str, conversion[i].val);
    }
}

Trie initializeTrieNode()
{
    // Allocate memory for a new trie node
    Trie newNode = (Trie)malloc(sizeof(struct trie));
    if (newNode == NULL)
    {
        return NULL;
    }
    newNode->children = NULL;
    newNode->data = NULL;
    newNode->childCount = 0;
    return newNode;
}

Trie getNextTrieNode(char character, Trie trieNode)
{ // Returns NULL if the node is not found
    for (int index = 0; index < trieNode->childCount; index++)
    {
        if (trieNode->children[index]->ch == character)
        {
            return trieNode->children[index]->ptr;
        }
    }
    return NULL;
}

Trie getOrCreateNextTrieNode(char character, Trie trieNode)
{ // Creates a new node if it doesn't exist
    for (int index = 0; index < trieNode->childCount; index++)
    {
        if (trieNode->children[index]->ch == character)
        {
            return trieNode->children[index]->ptr;
        }
    }

    trieNode->children = (TrieEdge *)realloc(trieNode->children, sizeof(TrieEdge) * ++(trieNode->childCount));
    trieNode->children[trieNode->childCount - 1] = (TrieEdge)malloc(sizeof(struct trieEdge));
    trieNode->children[trieNode->childCount - 1]->ch = character;
    trieNode->children[trieNode->childCount - 1]->ptr = initializeTrieNode();

    return trieNode->children[trieNode->childCount - 1]->ptr;
}

void storeDataInTrieNode(void *data, Trie trieNode)
{
    trieNode->data = data;
}

void *retrieveDataFromTrie(char *key, Trie rootNode)
{ // Returns NULL if the key is not found in the trie
    if (rootNode == NULL)
    {
        return NULL;
    }

    if (*key == '\0')
    {
        return rootNode->data; // If NULL is returned, the key does not exist
    }

    return retrieveDataFromTrie(key + 1, getNextTrieNode(*key, rootNode));
}

/* Recursively print a trie (symbol table) with indentation.
   The data pointer is printed as a generic pointer. */
// void printTrieRecursive(const Trie t, int level) {
//     if (t == NULL)
//         return;
//     // Indent based on tree level.
//     for (int i = 0; i < level; i++)
//         printf("  ");
//     if (t->data != NULL)
//         printf("[Data: %p]\n", t->data);
//     else
//         printf("[No Data]\n");
//     // Iterate over each trie edge (child)
//     for (int i = 0; i < t->childCount; i++) {
//         for (int j = 0; j < level; j++)
//             printf("  ");
//         printf("Edge '%c':\n", t->children[i].ch);
//         printTrieRecursive(t->children[i].ptr, level + 1);
//     }
// }

// /* Count the total number of nodes in a trie recursively */
// int countTrieNodes(const Trie t) {
//     if (t == NULL)
//         return 0;
//     int count = 1; // count current node
//     for (int i = 0; i < t->childCount; i++) {
//         count += countTrieNodes(t->children[i].ptr);
//     }
//     return count;
// }

void *storeInTrie(void *data, char *key, Trie rootNode)
{

    if (*key == '\0')
    {
        void *previousData = rootNode->data;
        rootNode->data = data;
        return previousData;
    }

    Trie nextNode = getOrCreateNextTrieNode(*key, rootNode);
    return storeInTrie(data, key + 1, nextNode);
}

Trie deleteTrieNodeRecursive(Trie node)
{
    for (int index = 0; index < node->childCount; index++)
    {
        deleteTrieNodeRecursive(node->children[index]->ptr);
        free(node->children[index]);
    }

    free(node->data);
    free(node);
}

void setupTwinBuffer() // Prepares the buffer before the next iteration
{
    buffer = (twinBuffer)malloc(sizeof(struct twinBuffer));
    buffer->forward = buffer->lexemeBegin = 0;
    buffer->fp = getStream(NULL);
    buffer->fileBufEndsHere = __INT_MAX__;
    currentLineNumber = 1;
}

void releaseTwinBuffer() // Deallocate the buffer before the next iteration
{
    fclose(buffer->fp);
    free(buffer);
    buffer = NULL;
}

void printTwinBuffer(const twinBuffer tb)
{
    if (tb == NULL)
    {
        printf("NULL twinBuffer\n");
        return;
    }
    printf("TwinBuffer State:\n");
    printf("  File pointer: %p\n", (void *)tb->fp);
    printf("  lexemeBegin: %d\n", tb->lexemeBegin);
    printf("  forward: %d\n", tb->forward);
    printf("  fileEndsAtBufferIndex: %d\n", tb->fileBufEndsHere);
    // Print a limited preview of each buffer
    printf("  First buffer: \"%.20s\"\n", tb->first);
    printf("  Second buffer: \"%.20s\"\n", tb->second);
}

void checkfn()
{
    cnter = cnter + 1;
    // printf("%d\n", i);
}

bool validateTwinBuffer(const twinBuffer tb)
{
    if (tb == NULL)
        return false;
    return (tb->lexemeBegin <= tb->forward && tb->forward <= tb->fileBufEndsHere);
}

tokenInfo generateTokenInfo(Vocabulary tokenType, char *text, int lineNum)
{
    tokenInfo token = (tokenInfo)malloc(sizeof(struct tokenInfo));
    token->tokenName = tokenType;
    if (text == NULL)
        strcpy(token->lexeme, "----");
    else
        strcpy(token->lexeme, text);
    token->lineNumber = lineNum;
    return token;
}

void setupSymbolTable()
{
    symbolTable = initializeTrieNode();

    const static struct
    {
        char *keyword;
        Vocabulary tokenType;
    } predefinedTokens[] = {
        {"with", TK_WITH},
        {"parameters", TK_PARAMETERS},
        {"end", TK_END},
        {"while", TK_WHILE},
        {"union", TK_UNION},
        {"endunion", TK_ENDUNION},
        {"definetype", TK_DEFINETYPE},
        {"as", TK_AS},
        {"type", TK_TYPE},
        {"_main", TK_MAIN},
        {"global", TK_GLOBAL},
        {"parameter", TK_PARAMETER},
        {"list", TK_LIST},
        {"input", TK_INPUT},
        {"output", TK_OUTPUT},
        {"int", TK_INT},
        {"real", TK_REAL},
        {"endwhile", TK_ENDWHILE},
        {"if", TK_IF},
        {"then", TK_THEN},
        {"endif", TK_ENDIF},
        {"read", TK_READ},
        {"write", TK_WRITE},
        {"return", TK_RETURN},
        {"call", TK_CALL},
        {"record", TK_RECORD},
        {"endrecord", TK_ENDRECORD},
        {"else", TK_ELSE},
    };

    for (int i = 0; i < sizeof(predefinedTokens) / sizeof(predefinedTokens[0]); i++)
    {
        storeInTrie(generateTokenInfo(predefinedTokens[i].tokenType, predefinedTokens[i].keyword, -1),
                    predefinedTokens[i].keyword,
                    symbolTable);
    }
}

void addToSymbolTable(tokenInfo tkinf, char *lexeme)
{
    storeInTrie((void *)generateTokenInfo(tkinf->tokenName, tkinf->lexeme, tkinf->lineNumber), lexeme, symbolTable);
}

void freeSymbolTable()
{
    deleteTrieNodeRecursive(symbolTable);
    symbolTable = NULL;
}

void *searchSymbolTable(char *lexeme)
{
    return retrieveDataFromTrie(lexeme, symbolTable);
}

FILE *getStream(FILE *fp)
{
    if (fp == NULL)
    {
        fp = fopen(tstfile, "r");
    }
    if (!feof(fp))
    {
        int tmp;
        if (buffer->forward == 0)
        {
            if ((tmp = fread(buffer->first, sizeof(char), SIZE_OF_BUFFER, fp)) < SIZE_OF_BUFFER)
            {
                buffer->fileBufEndsHere = tmp - 1;
            }
        }
        else if (buffer->forward == SIZE_OF_BUFFER)
        {
            if ((tmp = fread(buffer->second, sizeof(char), SIZE_OF_BUFFER, fp)) < SIZE_OF_BUFFER)
            {
                buffer->fileBufEndsHere = tmp - 1 + SIZE_OF_BUFFER;
            }
        }
    }
    return fp;
}

void retract(twinBuffer _buffer)
{
    _buffer->lexemeBegin = _buffer->forward;
}

void doubleRetract(twinBuffer _buffer)
{
    _buffer->lexemeBegin = _buffer->forward = (_buffer->forward + (2 * SIZE_OF_BUFFER) - 1) % (2 * SIZE_OF_BUFFER);
}

char readCharFromBuffer(int ptr, twinBuffer _buffer)
{
    if (ptr == _buffer->forward && _buffer->forward > _buffer->fileBufEndsHere)
        return '\0';
    if (ptr < 256)
    {
        return _buffer->first[ptr];
    }

    return _buffer->second[ptr - 256];
}

char *extractLexeme(char *dest, twinBuffer _buffer)
{
    // Ensure 'dest' has enough space to store the lexeme
    char *ptr = dest;
    checkfn(cnter);
    do
    {
        *ptr = readCharFromBuffer(_buffer->lexemeBegin, _buffer);
        _buffer->lexemeBegin = (_buffer->lexemeBegin + 1) % (2 * SIZE_OF_BUFFER);
        ptr++;
    } while (_buffer->lexemeBegin != _buffer->forward);

    *ptr = '\0';
    return dest;
}

void advanceBuffer(twinBuffer _buffer)
{
    _buffer->forward = (_buffer->forward + 1) % (2 * SIZE_OF_BUFFER);

    if (_buffer->forward % SIZE_OF_BUFFER == 0)
    {
        _buffer->fp = getStream(_buffer->fp);
    }
}

char *updateLexemePointersAndExtract(char *Dest, twinBuffer _buffer)
{
    advanceBuffer(_buffer);
    return extractLexeme(Dest, _buffer);
}

int calculateLexemeLength(twinBuffer _buffer)
{
    return (_buffer->forward >= _buffer->lexemeBegin)
               ? (_buffer->forward - _buffer->lexemeBegin)
               : (2 * SIZE_OF_BUFFER - _buffer->lexemeBegin + _buffer->forward);
}

void resetLexemePointers(twinBuffer _buffer)
{
    advanceBuffer(_buffer);
    _buffer->lexemeBegin = _buffer->forward;
}

void printTokenInfo(const tokenInfo t)
{
    if (t == NULL)
    {
        printf("NULL tokenInfo\n");
        return;
    }
    printf("Token: %s, Lexeme: \"%s\", Line: %d\n",
           vocabularyToString(t->tokenName), t->lexeme, t->lineNumber);
}


void reportError(int lineNum, const char *fmt, ...) {
    char errorMessage[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(errorMessage, sizeof(errorMessage), fmt, args);
    va_end(args);
    printf("Line %d\t Error: %s\n", lineNum, errorMessage);
    for (int i = 0; i < fptrsLen; i++) {
        fprintf(fptrs[i], "Line %d\t Error: %s\n", lineNum, errorMessage);
    }
}

tokenInfo getNextToken(twinBuffer B) {
    skipWhitespace(B);
    char c = readCharFromBuffer(B->forward, B);
    if (c == '\0')
        return generateTokenInfo(TK_DOLLAR, "\0", currentLineNumber);
    if (c == '<')
        return processLessThan(B);
    else if (c == '>')
        return processGreaterThan(B);
    else if (c == '+')
        return processPlus(B);
    else if (c == '-')
        return processMinus(B);
    else if (c == '*')
        return processMul(B);
    else if (c == '/')
        return processDiv(B);
    else if (c == '~')
        return processNot(B);
    else if (c == '&')
        return processAnd(B);
    else if (c == '@')
        return processOr(B);
    else if (c == '=')
        return processEquals(B);
    else if (c == '!')
        return processNotEquals(B);
    else if (isdigit(c))
        return processNumber(B);
    else if (c == '(')
        return processParenOpen(B);
    else if (c == ')')
        return processParenClose(B);
    else if (c == '[')
        return processSquareOpen(B);
    else if (c == ']')
        return processSquareClose(B);
    else if (c == '.')
        return processDot(B);
    else if (c == ':')
        return processColon(B);
    else if (c == ',')
        return processComma(B);
    else if (c == ';')
        return processSemicolon(B);
    else if (c == '%')
        return processComment(B);
    else if (c == '#')
        return processRUID(B);
    else if (c == '_' || isalpha(c))
        return processIdentifier(B);
    else {
        char tmp_lexeme[2] = {c, '\0'};
        advanceBuffer(B);
        retract(B);
        reportError(currentLineNumber, "Unrecognized symbol <%c>", c);
        return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
    }
}

void skipWhitespace(twinBuffer _buffer) {
    char c = readCharFromBuffer(_buffer->forward, _buffer);
    while (c == ' ' || c == '\t' || c == '\n') {
        if (c == '\n')
            currentLineNumber++;
        advanceBuffer(_buffer);
        c = readCharFromBuffer(_buffer->forward, _buffer);
    }
    _buffer->lexemeBegin = _buffer->forward;
}

tokenInfo processLessThan(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '=') {
        advanceBuffer(_buffer);
        retract(_buffer);
        return generateTokenInfo(TK_LE, "<=", currentLineNumber);
    } else if (next == '-') {
        advanceBuffer(_buffer);
        next = readCharFromBuffer(_buffer->forward, _buffer);
        if (next == '-') {
            advanceBuffer(_buffer);
            next = readCharFromBuffer(_buffer->forward, _buffer);
            if (next == '-') {
                advanceBuffer(_buffer);
                retract(_buffer);
                return generateTokenInfo(TK_ASSIGNOP, "<---", currentLineNumber);
            }
        }
        doubleRetract(_buffer);
        retract(_buffer);
        return generateTokenInfo(TK_LT, "<", currentLineNumber);
    } else {
        retract(_buffer);
        return generateTokenInfo(TK_LT, "<", currentLineNumber);
    }
}

tokenInfo processGreaterThan(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '=') {
        advanceBuffer(_buffer);
        retract(_buffer);
        return generateTokenInfo(TK_GE, ">=", currentLineNumber);
    } else {
        retract(_buffer);
        return generateTokenInfo(TK_GT, ">", currentLineNumber);
    }
}

tokenInfo processPlus(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_PLUS, "+", currentLineNumber);
}

tokenInfo processMinus(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_MINUS, "-", currentLineNumber);
}

tokenInfo processMul(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_MUL, "*", currentLineNumber);
}

tokenInfo processDiv(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_DIV, "/", currentLineNumber);
}

tokenInfo processNot(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_NOT, "~", currentLineNumber);
}

tokenInfo processAnd(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '&') {
        advanceBuffer(_buffer);
        next = readCharFromBuffer(_buffer->forward, _buffer);
        if (next == '&') {
            advanceBuffer(_buffer);
            retract(_buffer);
            return generateTokenInfo(TK_AND, "&&&", currentLineNumber);
        }
    }
    char tmp[LEX_MAX];
    extractLexeme(tmp, _buffer);
    reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp);
    return generateTokenInfo(LEXICAL_ERROR, tmp, currentLineNumber);
}

tokenInfo processOr(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '@') {
        advanceBuffer(_buffer);
        next = readCharFromBuffer(_buffer->forward, _buffer);
        if (next == '@') {
            advanceBuffer(_buffer);
            retract(_buffer);
            return generateTokenInfo(TK_OR, "@@@", currentLineNumber);
        }
    }
    char tmp[LEX_MAX];
    extractLexeme(tmp, _buffer);
    reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp);
    return generateTokenInfo(LEXICAL_ERROR, tmp, currentLineNumber);
}

tokenInfo processEquals(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '=') {
        advanceBuffer(_buffer);
        retract(_buffer);
        return generateTokenInfo(TK_EQ, "==", currentLineNumber);
    }
    char tmp[LEX_MAX];
    extractLexeme(tmp, _buffer);
    reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp);
    return generateTokenInfo(LEXICAL_ERROR, tmp, currentLineNumber);
}

tokenInfo processNotEquals(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char next = readCharFromBuffer(_buffer->forward, _buffer);
    if (next == '=') {
        advanceBuffer(_buffer);
        retract(_buffer);
        return generateTokenInfo(TK_NE, "!=", currentLineNumber);
    }
    char tmp[LEX_MAX];
    extractLexeme(tmp, _buffer);
    reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp);
    return generateTokenInfo(LEXICAL_ERROR, tmp, currentLineNumber);
}

tokenInfo processNumber(twinBuffer _buffer) {
    char tmp_lexeme[LEX_MAX];
    while (isdigit(readCharFromBuffer(_buffer->forward, _buffer)))
        advanceBuffer(_buffer);
    if (readCharFromBuffer(_buffer->forward, _buffer) == '.') {
        advanceBuffer(_buffer);
        if (!isdigit(readCharFromBuffer(_buffer->forward, _buffer))) {
            extractLexeme(tmp_lexeme, _buffer);
            reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp_lexeme);
            return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
        }
        while (isdigit(readCharFromBuffer(_buffer->forward, _buffer)))
            advanceBuffer(_buffer);
        extractLexeme(tmp_lexeme, _buffer);
        return generateTokenInfo(TK_RNUM, tmp_lexeme, currentLineNumber);
    } else {
        extractLexeme(tmp_lexeme, _buffer);
        return generateTokenInfo(TK_NUM, tmp_lexeme, currentLineNumber);
    }
}

tokenInfo processParenOpen(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_OP, "(", currentLineNumber);
}

tokenInfo processParenClose(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_CL, ")", currentLineNumber);
}

tokenInfo processSquareOpen(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_SQL, "[", currentLineNumber);
}

tokenInfo processSquareClose(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_SQR, "]", currentLineNumber);
}

tokenInfo processDot(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_DOT, ".", currentLineNumber);
}

tokenInfo processColon(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_COLON, ":", currentLineNumber);
}

tokenInfo processComma(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_COMMA, ",", currentLineNumber);
}

tokenInfo processSemicolon(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    retract(_buffer);
    return generateTokenInfo(TK_SEM, ";", currentLineNumber);
}

tokenInfo processComment(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    char c = readCharFromBuffer(_buffer->forward, _buffer);
    while (c != '\n' && c != '\0') {
        advanceBuffer(_buffer);
        c = readCharFromBuffer(_buffer->forward, _buffer);
    }
    retract(_buffer);
    return generateTokenInfo(TK_COMMENT, "%", currentLineNumber++);
}

tokenInfo processRUID(twinBuffer _buffer) {
    advanceBuffer(_buffer);
    while (islower(readCharFromBuffer(_buffer->forward, _buffer)))
        advanceBuffer(_buffer);
    char tmp_lexeme[LEX_MAX];
    extractLexeme(tmp_lexeme, _buffer);
    return generateTokenInfo(TK_RUID, tmp_lexeme, currentLineNumber);
}

tokenInfo processIdentifier(twinBuffer _buffer) {
    char tmp_lexeme[LEX_MAX];
    char c = readCharFromBuffer(_buffer->forward, _buffer);
    if (c == '_') {
        advanceBuffer(_buffer);
        c = readCharFromBuffer(_buffer->forward, _buffer);
        if (!isalpha(c)) {
            extractLexeme(tmp_lexeme, _buffer);
            reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp_lexeme);
            return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
        }
        while (isalpha(readCharFromBuffer(_buffer->forward, _buffer)) ||
               isdigit(readCharFromBuffer(_buffer->forward, _buffer)))
            advanceBuffer(_buffer);
        if (calculateLexemeLength(_buffer) > LEN_FUNID_MAX) {
            retract(_buffer);
            reportError(currentLineNumber, "Function name exceeds the allowed length of %d characters.", LEN_FUNID_MAX);
            return generateTokenInfo(FUN_LENGTH_EXC, tmp_lexeme, currentLineNumber);
        }
        extractLexeme(tmp_lexeme, _buffer);
        tokenInfo existing = searchSymbolTable(tmp_lexeme);
        if (existing)
            return generateTokenInfo(existing->tokenName, tmp_lexeme, currentLineNumber);
        else {
            tokenInfo newToken = generateTokenInfo(TK_FUNID, tmp_lexeme, currentLineNumber);
            addToSymbolTable(newToken, tmp_lexeme);
            return newToken;
        }
    } else if (isalpha(c)) {
        if (c == 'a' || (c >= 'e' && c <= 'z')) {
            while (islower(readCharFromBuffer(_buffer->forward, _buffer)))
                advanceBuffer(_buffer);
            if (calculateLexemeLength(_buffer) > LENGTH_MAX_FIELD_ID) {
                retract(_buffer);
                reportError(currentLineNumber, "Record/Union name exceeds the allowed length of %d characters.", LENGTH_MAX_FIELD_ID);
                return generateTokenInfo(ID_LENGTH_EXC, tmp_lexeme, currentLineNumber);
            }
            extractLexeme(tmp_lexeme, _buffer);
            tokenInfo existing = searchSymbolTable(tmp_lexeme);
            if (existing)
                return generateTokenInfo(existing->tokenName, tmp_lexeme, currentLineNumber);
            else {
                tokenInfo newToken = generateTokenInfo(TK_FIELDID, tmp_lexeme, currentLineNumber);
                addToSymbolTable(newToken, tmp_lexeme);
                return newToken;
            }
        } else if (c >= 'b' && c <= 'd') {
            advanceBuffer(_buffer);
            c = readCharFromBuffer(_buffer->forward, _buffer);
            if (islower(c)) {
                while (islower(readCharFromBuffer(_buffer->forward, _buffer)))
                    advanceBuffer(_buffer);
                if (calculateLexemeLength(_buffer) > LENGTH_MAX_FIELD_ID) {
                    retract(_buffer);
                    reportError(currentLineNumber, "Record/Union name exceeds the allowed length of %d characters.", LENGTH_MAX_FIELD_ID);
                    return generateTokenInfo(ID_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                }
                extractLexeme(tmp_lexeme, _buffer);
                tokenInfo existing = searchSymbolTable(tmp_lexeme);
                if (existing)
                    return generateTokenInfo(existing->tokenName, tmp_lexeme, currentLineNumber);
                else {
                    tokenInfo newToken = generateTokenInfo(TK_FIELDID, tmp_lexeme, currentLineNumber);
                    addToSymbolTable(newToken, tmp_lexeme);
                    return newToken;
                }
            } else if (c >= '2' && c <= '7') {
                while ((readCharFromBuffer(_buffer->forward, _buffer) >= '2' &&
                        readCharFromBuffer(_buffer->forward, _buffer) <= '7') ||
                       (readCharFromBuffer(_buffer->forward, _buffer) >= 'b' &&
                        readCharFromBuffer(_buffer->forward, _buffer) <= 'd'))
                    advanceBuffer(_buffer);
                if (calculateLexemeLength(_buffer) > LENGTH_MAX_VAR_ID) {
                    retract(_buffer);
                    reportError(currentLineNumber, "Variable name exceeds the allowed length of %d characters.", LENGTH_MAX_VAR_ID);
                    return generateTokenInfo(VAR_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                }
                extractLexeme(tmp_lexeme, _buffer);
                tokenInfo existing = searchSymbolTable(tmp_lexeme);
                if (existing)
                    return generateTokenInfo(existing->tokenName, tmp_lexeme, currentLineNumber);
                else {
                    tokenInfo newToken = generateTokenInfo(TK_ID, tmp_lexeme, currentLineNumber);
                    addToSymbolTable(newToken, tmp_lexeme);
                    return newToken;
                }
            } else {
                while (islower(readCharFromBuffer(_buffer->forward, _buffer)))
                    advanceBuffer(_buffer);
                if (calculateLexemeLength(_buffer) > LENGTH_MAX_FIELD_ID) {
                    retract(_buffer);
                    reportError(currentLineNumber, "Record/Union name exceeds the allowed length of %d characters.", LENGTH_MAX_FIELD_ID);
                    return generateTokenInfo(ID_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                }
                extractLexeme(tmp_lexeme, _buffer);
                tokenInfo existing = searchSymbolTable(tmp_lexeme);
                if (existing)
                    return generateTokenInfo(existing->tokenName, tmp_lexeme, currentLineNumber);
                else {
                    tokenInfo newToken = generateTokenInfo(TK_FIELDID, tmp_lexeme, currentLineNumber);
                    addToSymbolTable(newToken, tmp_lexeme);
                    return newToken;
                }
            }
        }
    }
    advanceBuffer(_buffer);
    extractLexeme(tmp_lexeme, _buffer);
    reportError(currentLineNumber, "Unrecognized pattern <%s>", tmp_lexeme);
    return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
}

linkedList *extractTokens(FILE *sourceFile) {
    setupSymbolTable();
    setupTwinBuffer();
    tokenInfo tokenDetails;
    fptrsLen = 2;
    fptrs = calloc(fptrsLen, sizeof(FILE *));
    fptrs[0] = fopen(tokenListFile, "w");
    fptrs[1] = fopen(loefile, "w");
    linkedList *tokenizedList = initializeLinkedList();
    char tokenBuffer[LEX_MAX];
    printf("Lexical Errors are also printed on console\n");
    while ((tokenDetails = getNextToken(buffer)) != NULL) {
        convertEnumToString(tokenDetails->tokenName, tokenBuffer);
        if (strcmp(tokenBuffer, "LEXICAL_ERROR") != 0 &&
            strcmp(tokenBuffer, "ID_LENGTH_EXC") != 0 &&
            strcmp(tokenBuffer, "FUN_LENGTH_EXC") != 0 &&
            strcmp(tokenBuffer, "VAR_LENGTH_EXC") != 0 &&
            strcmp(tokenBuffer, "TK_DOLLAR") != 0)
        {
            fprintf(fptrs[0], "Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n",
                    tokenDetails->lineNumber, tokenDetails->lexeme, tokenBuffer);
        }
        lln *newNode = initializeNode(tokenDetails);
        appendNode(tokenizedList, newNode);
        if (tokenDetails->tokenName == TK_DOLLAR)
            break;
    }
    for (int idx = 0; idx < fptrsLen; idx++)
        fclose(fptrs[idx]);
    free(fptrs);
    fptrsLen = 0;
    releaseTwinBuffer();
    freeSymbolTable();
    return tokenizedList;
}



linkedList *processLexicalTokens(FILE *inputFile, char *outputPath)
{
    // Wrapper function that invokes extractTokens to process and return tokenized output
    // This function is intended for use by the driver

    if (!inputFile)
    {
        printf("Error: Unable to open input file for lexical analysis\n");
        exit(-1);
    }

    linkedList *tokenList = extractTokens(inputFile);
    if (!tokenList)
    {
        printf("Error: Failed to generate token list\n");
        exit(-1);
    }

    return tokenList;
}

void printLinkedListReverseRecursive(const lln *node)
{
    if (node == NULL)
        return;
    printLinkedListReverseRecursive(node->next);
    printTokenInfo(node->symTBEntry);
}

void printLinkedListReverse(const linkedList *list)
{
    if (list == NULL)
    {
        printf("NULL linkedList\n");
        return;
    }
    printf("Linked List (Reverse Order):\n");
    printLinkedListReverseRecursive(list->head);
}

int countLinkedListTokens(const linkedList *list)
{
    if (list == NULL)
        return 0;
    return list->count;
}
