#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "parserDef.h"

symlstnode *symbol_list_createNode(gramsym *gramsym);
symlst *symbol_list_create();
void symbol_list_insert_node(symlst *symlst, symlstnode *symlistnode);
bool compareGramsym(const gramsym *a, const gramsym *b);
bool symlstContains(const symlst *list, const gramsym *symb);
parsetreeNode *parse_tree_node_create();
parseT *parse_tree_create();
void insert_node_as_child(parsetreeNode *parent, parsetreeNode *child);
void initializeNonTerminalToString();
void initializeTokenToString();
Token getTokenFromString(char *str);
NonTerminal getNonTerminalFromString(char *str);
void readGrammar();
void printGrammar();
void insert_ff_rhs_node(frstfwdRHS *list, frstfwdRHSNode *node);
bool removeTokenFromFrstfwdRHS(frstfwdRHS *list, Token tk);
void mergeFrstfwdRHS(frstfwdRHS *target, const frstfwdRHS *source);
void printComputedFirstAndFollow();
bool exists_in_rhs(frstfwdRHS *list, Token _key);
frstfwdRHS *get_first_of_rhs(symlst *grammar_rhs);
void free_up_ff_rhs(frstfwdRHS *list_r);
void printRule(grmrule *rule, FILE *fp);
void initializeParseTable();
void add_rules_to_parse_table();
void print_parse_table();
parseT *parseTokens(linkedList *tokenList, bool *hasSyntaxError, FILE *fp);
void computeFirstSets();
void computeFollowSets();
void computeFirstAndFollow();
void print_tree_node(parsetreeNode *node, parsetreeNode *parent, FILE *file);
void printShortTreeNode(parsetreeNode *currentNode, parsetreeNode *parentNode, FILE *outFile);
void printParseTree(parseT *parseTree, char *outputFilename);
void inorderTraverse(parsetreeNode *node, parsetreeNode *parent, FILE *file);
extern void parseInputSourceCode(char *inpFile, char *opFile);

#endif