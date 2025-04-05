#ifndef PARSER_DEF
#define PARSER_DEF

#include "lexerDef.h"

#define NON_TERMINAL_LENGTH 30
#define GRAMMAR_RULES_MAX 150
#define RULE_LEN_MAX 512
#define CHILD_CAP 10

extern bool grammarRead;
extern bool notInit;
extern bool frstflwInit;
extern bool PTreeInit;
extern int numrules;

typedef enum vocabulary Token;

typedef enum NonTerminal
{
    relationalOp,
    booleanExpression,
    returnStmt,
    var,
    output_par,
    typeDefinitions,
    stmts,
    definetypestmt,
    elsePart,
    oneExpansion,
    outputParameters,
    factor,
    program,
    conditionalStmt,
    constructedDatatype,
    funCallStmt,
    input_par,
    A,
    logicalOp,
    declarations,
    arithmeticExpression,
    dataType,
    fieldDefinition,
    function,
    more_ids,
    assignmentStmt,
    singleOrRecId,
    fieldDefinitions,
    actualOrRedefined,
    parameter_list,
    option_single_constructed,
    ioStmt,
    stmt,
    termPrime,
    optionalReturn,
    idList,
    global_or_not,
    expPrime,
    primitiveDatatype,
    moreExpansions,
    mainFunction,
    fieldType,
    otherFunctions,
    otherStmts,
    lowPrecedenceOperators,
    inputParameters,
    highPrecedenceOperators,
    declaration,
    moreFields,
    typeDefinition,
    remaining_list,
    iterativeStmt,
    term,
    NT_NOT_FOUND
} NonTerminal;

typedef struct gramsym
{
    bool isNonTerminal;
    union
    {
        NonTerminal nt;
        Token t;
    } isTorNT;
} gramsym;

typedef struct symlstnode
{
    gramsym *symb;
    struct symlstnode *prev;
    struct symlstnode *next;
} symlstnode;

typedef struct symlst
{
    symlstnode *head;
    symlstnode *tail;
    int count;
} symlst;

typedef struct grmrule
{
    gramsym *lhs;
    symlst *rhs;
} grmrule;

typedef struct frstfwdRHSNode
{
    Token tk;
    struct frstfwdRHSNode *next;
} frstfwdRHSNode;

typedef struct frstfwdRHS
{
    frstfwdRHSNode *head;
    frstfwdRHSNode *tail;
} frstfwdRHS;

extern grmrule ***parseTable;
extern grmrule *Grammar[GRAMMAR_RULES_MAX];
extern frstfwdRHS **Follow;
extern frstfwdRHS **cmpFirst;
extern frstfwdRHS **cmpflw;
extern frstfwdRHS **First;

typedef struct parsetreeNode
{
    gramsym *symbol;
    tokenInfo symtbEntry;
    struct parsetreeNode **children;
    int capacity, size, lineNumber;
} parsetreeNode;

typedef struct parseT
{
    parsetreeNode *root;
} parseT;

#endif