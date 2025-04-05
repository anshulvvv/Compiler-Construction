#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer.h"
#include "parser.h"

bool canPrint = true;

void displayOptions() {
    printf("\nSelect an option:\n");
    printf("0 : Exit\n");
    printf("1 : Strip comments and generate a comment-free source file\n");
    printf("2 : Display the token list produced by the lexer on the console and store in files\n");
    printf("3 : Validate the source code's syntax—if errors exist, list them on console and in any case, show the completed or error recovered parse tree in given output file\n");
    printf("4 : Show the total execution time of the lexer and parser during syntax verification\n");
}

void runCommentRemoval() {
    printf("comment_less_file.txt created in which comments are stripped from source file\n");
    remcom(tstfile, commFreeFile);
}

void runTokenDisplay() {
    printf("Token list and lexical errors are printed on console and stored in token_list_file.txt\n");
    printf("Lexical errors are also stored in lex_error_file.txt\n");
    setupSymbolTable();
    setupTwinBuffer();
    tokenInfo node;
    fptrsLen = 2;
    fptrs = calloc(fptrsLen, sizeof(FILE*));
    fptrs[0] = fopen("token_list_file.txt", "w");
    fptrs[1] = fopen("lex_error_file.txt", "w");
    char tokenText[LEX_MAX];
    while ((node = getNextToken(buffer)) != NULL) {
        convertEnumToString(node->tokenName, tokenText);
        if (strcmp(tokenText, "LEXICAL_ERROR") != 0 && strcmp(tokenText, "ID_LENGTH_EXC") != 0 &&
            strcmp(tokenText, "FUN_LENGTH_EXC") != 0 && strcmp(tokenText, "VAR_LENGTH_EXC") != 0 && strcmp(tokenText, "TK_DOLLAR") != 0) {
            printf("Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", node->lineNumber, node->lexeme, tokenText);
            fprintf(fptrs[0], "Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", node->lineNumber, node->lexeme, tokenText);
        }
        if (node->tokenName == TK_DOLLAR)
            break;
        free(node);
    }
    for (int i = 0; i < fptrsLen; i++)
        fclose(fptrs[i]);
    free(fptrs);
    fptrsLen = 0;
    releaseTwinBuffer();
    freeSymbolTable();
}

void runTimeCalculation(char *fileA, char *fileB) {
    canPrint = false;
    clock_t start, end;
    double elapsed, seconds;
    start = clock();
    parseInputSourceCode(fileA, fileB);
    end = clock();
    elapsed = (double)(end - start);
    seconds = elapsed / CLOCKS_PER_SEC;
    printf("Total execution time for lexer and parser during syntax validation: %f seconds\n", seconds);
    canPrint = true;
}

#ifndef MAIN_FILE
#define MAIN_FILE

int main(int argc, char *argv[]) {
    strcpy(tstfile, argv[1]);
    strcpy(parseTOutFile, argv[2]);
    while (1) {
        displayOptions();
        char choice[10];
        scanf("%s", choice);
        switch (choice[0]) {
            case '0': return 0;
            case '1': runCommentRemoval(); break;
            case '2': runTokenDisplay(); break;
            case '3': parseInputSourceCode(argv[1], argv[2]); break;
            case '4': runTimeCalculation(argv[1], argv[2]); break;
            default: printf("Invalid Option Selected\n");
        }
    }
    return 0;
}

#endif
