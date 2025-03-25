#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void *reportFieldIdTooLong(int lineNum, FILE *files[], int fileCount, int maxLength)
{
    printf("Line %d\t Error: Record/Union name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    for (int index = 0; index < fileCount; index++)
    {
        fprintf(files[index], "Line %d\t Error: Record/Union name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    }
    return NULL;
}

void *reportUnknownSymbol(int lineNum, char lexeme, FILE *files[], int fileCount)
{
    printf("Line %d\t Error: Unrecognized symbol <%c>\n", lineNum, lexeme);
    for (int index = 0; index < fileCount; index++)
    {
        fprintf(files[index], "Line %d\t Error: Unrecognized symbol <%c>\n", lineNum, lexeme);
    }
    // printf(files[index], "Line %d\t Error: Unrecognized symbol <%c>\n", lineNum, lexeme);
    return NULL;
}

void *reportFunctionIdTooLong(int lineNum, FILE *files[], int fileCount, int maxLength)
{
    printf("Line %d\t Error: Function name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    for (int index = 0; index < fileCount; index++)
    {
        fprintf(files[index], "Line %d\t Error: Function name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
        // printf(files[index], "Line %d\t Error: Function name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    }
    return NULL;
}

void *reportUnknownPattern(int lineNum, char *lexeme, FILE *files[], int fileCount)
{
    printf("Line %d\t Error: Unrecognized pattern <%s>\n", lineNum, lexeme);
    for (int index = 0; index < fileCount; index++)
    {
        fprintf(files[index], "Line %d\t Error: Unrecognized pattern <%s>\n", lineNum, lexeme);
    }
    // printf(files[index], "Line %d\t Error: Unrecognized pattern <%s>\n", lineNum, lexeme);
    return NULL;
}

void *reportVariableSizeExceeded(int lineNum, FILE *files[], int fileCount, int maxLength)
{
    printf("Line %d\t Error: Variable name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    for (int index = 0; index < fileCount; index++)
    {
        fprintf(files[index], "Line %d\t Error: Variable name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
        // printf(files[index], "Line %d\t Error: Variable name exceeds the allowed length of %d characters.\n", lineNum, maxLength);
    }
    return NULL;
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

tokenInfo getNextToken(twinBuffer B)
{
    char c;
    char tmp_lexeme[LEX_MAX];
    int diff_tmp;
    tokenInfo tmp;
    while (1)
    {
        switch (state)
        {
        case 0:
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '\0')
                return generateTokenInfo(TK_DOLLAR, "\0", currentLineNumber);
            else if (c == '\n')
                state = 15;
            else if (c == ' ' || c == '\t')
                state = 16;
            else if (c == '<')
                state = 1;
            else if (c == '>')
                state = 7;
            else if (c == '+')
                state = 10;
            else if (c == '-')
                state = 11;
            else if (c == '*')
                state = 12;
            else if (c == '/')
                state = 13;
            else if (c == '~')
                state = 14;
            else if (c == '&')
                state = 18;
            else if (c == '@')
                state = 21;
            else if (c == '=')
                state = 24;
            else if (c == '!')
                state = 26;
            else if (c >= '0' && c <= '9')
                state = 28;
            else if (c == '(')
                state = 37;
            else if (c == ')')
                state = 38;
            else if (c == '[')
                state = 39;
            else if (c == ']')
                state = 40;
            else if (c == '.')
                state = 41;
            else if (c == ':')
                state = 42;
            else if (c == ',')
                state = 43;
            else if (c == '_')
                state = 44;
            else if (c == ';')
                state = 48;
            else if (c == '%')
                state = 49;
            else if (c == '#')
                state = 51;
            else if (c == 'a' || (c >= 'e' && c <= 'z'))
                state = 54;
            else if (c >= 'b' && c <= 'd')
                state = 56;

            // else if
            else
            {
                state = 0;
                resetLexemePointers(buffer);
                // strcpy(tmp_lexeme, c);
                reportUnknownSymbol(currentLineNumber, c, fptrs, fptrsLen);
                char tmp[2] = {c, '\0'};
                return generateTokenInfo(LEXICAL_ERROR, tmp, currentLineNumber);
                break;
            }
            break;

        case 1:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '=')
                state = 2;
            else if (c == '-')
                state = 3;
            else
                state = 6;
            break;

        case 2:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "<=");
            return generateTokenInfo(TK_LE, tmp_lexeme, currentLineNumber);

        case 3:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '-')
                state = 4;
            else
            {
                doubleRetract(buffer);
                state = 0;
                strcpy(tmp_lexeme, "<");
                return generateTokenInfo(TK_LT, tmp_lexeme, currentLineNumber);
            }
            break;

        case 4:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '-')
                state = 5;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 5:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "<---");
            return generateTokenInfo(TK_ASSIGNOP, tmp_lexeme, currentLineNumber);

        case 6:
            retract(buffer);
            state = 0;
            strcpy(tmp_lexeme, "<");
            return generateTokenInfo(TK_LT, tmp_lexeme, currentLineNumber);

        case 7:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '=')
                state = 8;
            else
                state = 9;
            break;

        case 8:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ">=");
            return generateTokenInfo(TK_GE, tmp_lexeme, currentLineNumber);

        case 9:
            retract(buffer);
            state = 0;
            strcpy(tmp_lexeme, ">");
            return generateTokenInfo(TK_GT, tmp_lexeme, currentLineNumber);

        case 10:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "+");
            return generateTokenInfo(TK_PLUS, tmp_lexeme, currentLineNumber);

        case 11:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "-");
            return generateTokenInfo(TK_MINUS, tmp_lexeme, currentLineNumber);

        case 12:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "*");
            return generateTokenInfo(TK_MUL, tmp_lexeme, currentLineNumber);

        case 13:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "/");
            return generateTokenInfo(TK_DIV, tmp_lexeme, currentLineNumber);

        case 14:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "~");
            return generateTokenInfo(TK_NOT, tmp_lexeme, currentLineNumber);

        case 15:
            currentLineNumber++;
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '\n')
                state = 15;
            else if (c == ' ' || c == '\t')
                state = 16;
            else
                state = 17;
            break;

        case 16:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '\n')
                state = 15;
            else if (c == ' ' || c == '\t')
                state = 16;
            else
                state = 17;
            break;

        case 17:
            retract(buffer);
            state = 0;
            break;

        case 18:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '&')
                state = 19;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 19:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '&')
                state = 20;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 20:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "&&&");
            return generateTokenInfo(TK_AND, tmp_lexeme, currentLineNumber);

        case 21:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '@')
                state = 22;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 22:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '@')
                state = 23;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 23:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "@@@");
            return generateTokenInfo(TK_OR, tmp_lexeme, currentLineNumber);

        case 24:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '=')
                state = 25;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 25:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "==");
            return generateTokenInfo(TK_EQ, tmp_lexeme, currentLineNumber);

        case 26:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '=')
                state = 27;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 27:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "!=");
            return generateTokenInfo(TK_NE, tmp_lexeme, currentLineNumber);

        case 28:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
                state = 28;
            else if (c == '.')
                state = 29;
            else
                state = 36;
            break;

        case 29:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
                state = 30;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 30:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
                state = 31;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 31:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == 'E')
                state = 32;
            else
                state = 35;
            break;

        case 32:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '+' || c == '-')
                state = 33;
            else if (c >= '0' && c <= '9')
                state = 34;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 33:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
                state = 34;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 34:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
            {
                advanceBuffer(buffer);
                state = 35;
            }
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 35:
            extractLexeme(tmp_lexeme, buffer);
            state = 0;
            return generateTokenInfo(TK_RNUM, tmp_lexeme, currentLineNumber);

        case 36:
            extractLexeme(tmp_lexeme, buffer);
            state = 0;
            return generateTokenInfo(TK_NUM, tmp_lexeme, currentLineNumber);

        case 37:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "(");
            return generateTokenInfo(TK_OP, tmp_lexeme, currentLineNumber);

        case 38:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ")");
            return generateTokenInfo(TK_CL, tmp_lexeme, currentLineNumber);

        case 39:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "[");
            return generateTokenInfo(TK_SQL, tmp_lexeme, currentLineNumber);

        case 40:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, "]");
            return generateTokenInfo(TK_SQR, tmp_lexeme, currentLineNumber);

        case 41:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ".");
            return generateTokenInfo(TK_DOT, tmp_lexeme, currentLineNumber);

        case 42:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ":");
            return generateTokenInfo(TK_COLON, tmp_lexeme, currentLineNumber);

        case 43:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ",");
            return generateTokenInfo(TK_COMMA, tmp_lexeme, currentLineNumber);

        case 44:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                state = 45;
            else
            {
                checkfn();
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 45:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                state = 45;
            else if (c >= '0' && c <= '9')
                state = 46;
            else
                state = 47;
            break;

        case 46:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '0' && c <= '9')
                state = 46;
            else
                state = 47;
            break;

        case 47:
            state = 0;
            diff_tmp = calculateLexemeLength(buffer);
            if ((diff_tmp) > LEN_FUNID_MAX)
            {
                retract(buffer);
                reportFunctionIdTooLong(currentLineNumber, fptrs, fptrsLen, LEN_FUNID_MAX);
                return generateTokenInfo(FUN_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                break;
            }
            extractLexeme(tmp_lexeme, buffer);
            if (tmp = searchSymbolTable(tmp_lexeme))
                return generateTokenInfo(tmp->tokenName, tmp_lexeme, currentLineNumber);
            else
            {
                tmp = generateTokenInfo(TK_FUNID, tmp_lexeme, currentLineNumber);
                addToSymbolTable(tmp, tmp_lexeme);
                return tmp;
            }

        case 48:
            resetLexemePointers(buffer);
            state = 0;
            strcpy(tmp_lexeme, ";");
            return generateTokenInfo(TK_SEM, tmp_lexeme, currentLineNumber);

        case 49:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c == '\n')
                state = 50;
            else
                state = 49;
            break;

        case 50:
            resetLexemePointers(buffer);
            state = 0;
            return generateTokenInfo(TK_COMMENT, "%", currentLineNumber++);

        case 51:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= 'a' && c <= 'z')
                state = 52;
            else
            {
                state = 0;
                extractLexeme(tmp_lexeme, buffer);
                reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
                return generateTokenInfo(LEXICAL_ERROR, tmp_lexeme, currentLineNumber);
                break;
            }
            break;

        case 52:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= 'a' && c <= 'z')
                state = 52;
            else
                state = 53;
            break;

        case 53:
            extractLexeme(tmp_lexeme, buffer);
            state = 0;
            return generateTokenInfo(TK_RUID, tmp_lexeme, currentLineNumber);

        case 54:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= 'a' && c <= 'z')
                state = 54;
            else
                state = 55;
            break;

        case 55:
            state = 0;
            diff_tmp = calculateLexemeLength(buffer);
            if ((diff_tmp) > LENGTH_MAX_FIELD_ID)
            {
                retract(buffer);
                reportFieldIdTooLong(currentLineNumber, fptrs, fptrsLen, LENGTH_MAX_FIELD_ID);
                return generateTokenInfo(ID_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                break;
            }
            extractLexeme(tmp_lexeme, buffer);
            if (tmp = searchSymbolTable(tmp_lexeme))
                return generateTokenInfo(tmp->tokenName, tmp_lexeme, currentLineNumber);
            else
            {
                tmp = generateTokenInfo(TK_FIELDID, tmp_lexeme, currentLineNumber);
                addToSymbolTable(tmp, tmp_lexeme);
                return tmp;
            }

        case 56:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= 'a' && c <= 'z')
                state = 54;
            else if (c >= '2' && c <= '7')
                state = 57;
            else
                state = 55;
            // else {
            //     state=0;
            //     extractLexeme(tmp_lexeme,buffer);
            //     reportUnknownPattern(currentLineNumber, tmp_lexeme, fptrs, fptrsLen);
            //     break;
            // }
            break;

        case 57:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= 98 && c <= 100)
                state = 57;
            else if (c >= '2' && c <= '7')
                state = 58;
            else
                state = 59;
            break;

        case 58:
            advanceBuffer(buffer);
            c = readCharFromBuffer(buffer->forward, buffer);
            if (c >= '2' && c <= '7')
                state = 58;
            else
                state = 59;
            break;

        case 59:
            state = 0;
            diff_tmp = calculateLexemeLength(buffer);
            if ((diff_tmp) > LENGTH_MAX_VAR_ID)
            {
                retract(buffer);
                reportVariableSizeExceeded(currentLineNumber, fptrs, fptrsLen, LENGTH_MAX_VAR_ID);
                return generateTokenInfo(VAR_LENGTH_EXC, tmp_lexeme, currentLineNumber);
                break;
            }
            extractLexeme(tmp_lexeme, buffer);
            if (tmp = searchSymbolTable(tmp_lexeme))
                return generateTokenInfo(tmp->tokenName, tmp_lexeme, currentLineNumber);
            else
            {
                tmp = generateTokenInfo(TK_ID, tmp_lexeme, currentLineNumber);
                addToSymbolTable(tmp, tmp_lexeme);
                return tmp;
            }
        }
    }
}

linkedList *extractTokens(FILE *sourceFile)
{
    setupSymbolTable();
    setupTwinBuffer();
    tokenInfo tokenDetails;
    fptrsLen = 2;
    fptrs = calloc(fptrsLen, sizeof(FILE *));

    fptrs[0] = fopen(tokenListFile, "w");
    fptrs[1] = fopen(loefile, "w");
    linkedList *tokenizedList = initializeLinkedList();
    char tokenBuffer[LEX_MAX];
    printf("Lexical Errors are also printed on console again\n");
    while (tokenDetails = getNextToken(buffer))
    {

        convertEnumToString(tokenDetails->tokenName, tokenBuffer);
        if (strcmp(tokenBuffer, "LEXICAL_ERROR") != 0 && strcmp(tokenBuffer, "ID_LENGTH_EXC") != 0 &&
            strcmp(tokenBuffer, "FUN_LENGTH_EXC") != 0 && strcmp(tokenBuffer, "VAR_LENGTH_EXC") != 0 && strcmp(tokenBuffer, "TK_DOLLAR") != 0)
        {
            // printf("Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", tokenDetails->lineNumber, tokenDetails->lexeme, tokenBuffer);
            fprintf(fptrs[0], "Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", tokenDetails->lineNumber, tokenDetails->lexeme, tokenBuffer);
        }
        lln *newNode = initializeNode(tokenDetails);
        appendNode(tokenizedList, newNode);
        if (tokenDetails->tokenName == TK_DOLLAR)
            break;
        // free(tokenDetails);
    }

    for (int idx = 0; idx < fptrsLen; idx++)
        fclose(fptrs[idx]);
    free(fptrs);
    fptrsLen = 0;
    // printf("Lexer completed successfully\n");
    releaseTwinBuffer();
    freeSymbolTable();
    // printf("Lexer completed successfully\n");

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