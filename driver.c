#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer.h"
#include "parser.h"
bool canPrint = true;

void print_menu()
{
    printf("\nSelect an option:\n");
    printf("0 : Exit\n");
    printf("1 : Strip comments and generate a comment-free source file\n");
    printf("2 : Display the token list produced by the lexer on the console and store in files\n");
    printf("3 : Validate the source code's syntax—if errors exist, list them on console and in any case, show the completed or error recovered parse tree in given output file\n");
    printf("4 : Show the total execution time of the lexer and parser during syntax verification\n");
}

void case_clean_comments()
{
    printf("comment_less_file.txt created in which comments are stripped from source file\n");
    remcom(tstfile, commFreeFile);
}

void case_print_token_list()
{
    printf("Token list and lexical errors are printed on console and stored in token_list_file.txt\n");
    printf("Lexical errors are also stored in lex_error_file.txt\n");
    setupSymbolTable();
    setupTwinBuffer();
    tokenInfo currentToken;
    fptrsLen = 2;
    fptrs = calloc(fptrsLen, sizeof(FILE *));

    fptrs[0] = fopen("token_list_file.txt", "w");
    fptrs[1] = fopen("lex_error_file.txt", "w");

    char tokenStr[LEX_MAX];
    while ((currentToken = getNextToken(buffer)) != NULL)
    {
        convertEnumToString(currentToken->tokenName, tokenStr);
        if (strcmp(tokenStr, "LEXICAL_ERROR") != 0 && strcmp(tokenStr, "ID_LENGTH_EXC") != 0 &&
            strcmp(tokenStr, "FUN_LENGTH_EXC") != 0 && strcmp(tokenStr, "VAR_LENGTH_EXC") != 0 && strcmp(tokenStr, "TK_DOLLAR") != 0)
        {
            printf("Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", currentToken->lineNumber, currentToken->lexeme, tokenStr);
            fprintf(fptrs[0], "Line no. %d\t\t\t Lexeme %s\t\t\t Token %s\n", currentToken->lineNumber, currentToken->lexeme, tokenStr);
        }
        if (currentToken->tokenName == TK_DOLLAR)
            break;
        free(currentToken);
    }

    for (int i = 0; i < fptrsLen; i++)
        fclose(fptrs[i]);
    free(fptrs);
    fptrsLen = 0;

    releaseTwinBuffer();
    freeSymbolTable();
}

void case_calculate_time(char *argv1, char *argv2)
{
    canPrint = false;
    clock_t s_time, e_time;
    double tct, tctis;
    s_time = clock();
    parseInputSourceCode(argv1, argv2);
    e_time = clock();
    tct = (double)(e_time - s_time);
    tctis = tct / CLOCKS_PER_SEC;
    printf("Total execution time for lexer and parser during syntax validation: %f seconds\n", tctis);
    canPrint = true;
}

#ifndef MAIN_FILE
#define MAIN_FILE

int main(int argc, char *argv[])
{

    strcpy(tstfile, argv[1]);
    strcpy(parseTOutFile, argv[2]);
    while (1)
    {
        print_menu();

        char op[10];
        scanf("%s", op);

        switch (op[0])
        {
        case '0':
            return 0;
        case '1':
            case_clean_comments();
            break;
        case '2':
            case_print_token_list();
            break;
        case '3':
            parseInputSourceCode(argv[1], argv[2]);
            break;
        case '4':
            case_calculate_time(argv[1], argv[2]);
            break;
        default:
            printf("Invalid Option Selected\n");
        }
    }
    return 0;
}

#endif