#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "lexer.h"
#include "lexerDef.h"
#include "parser.h"
#include "parserDef.h"
#include "stack.h"

char *nonTerminalToString[NT_NOT_FOUND];
char *tokenToString[MAX_VOC_SIZE];
grmrule *Grammar[GRAMMAR_RULES_MAX];
int numrules = 0;
frstfwdRHS **First;
frstfwdRHS **Follow;
frstfwdRHS **cmpFirst;
frstfwdRHS **cmpflw;
grmrule ***parseTable;

bool grammarRead = false;
bool notInit = false;
bool tokenInit = false;
bool frstflwInit = false;
bool PTreeInit = false;
int cbbma = 0;

// Stack ADT
Stack *createNewStack()
{
    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (stack == NULL)
    {
        fprintf(stderr, "Error: Unable to allocate memory for the new stack\n");
        exit(-1);
    }
    stack->top = NULL;
    return stack;
}

void pop(Stack *st)
{
    if (!isEmpty(st))
    {
        stkNODE temp = st->top;
        st->top = st->top->next;
        free(temp);
    }
}

parsetreeNode *top(Stack *st)
{
    if (!isEmpty(st))
        return st->top->data;
    else
        return NULL;
}

bool isEmpty(Stack *stack)
{
    return (stack->top == NULL);
}

void push(Stack *stack, parsetreeNode *pointer)
{
    stkNODE stacknode = (stkNODE)malloc(sizeof(StkNode));
    if (stacknode == NULL)
    {
        fprintf(stderr, "Error: Unable to allocate memory for the new node for pushing\n");
        exit(-1);
    }
    stacknode->next = stack->top;
    stacknode->data = pointer;
    stack->top = stacknode;
}

int isInteger(const char *str) // Returns 1 if the input string is a valid integer (entirely consumed by strtol), 0 otherwise.
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }

    errno = 0;
    char *conversionEnd;
    long integerval = strtol(str, &conversionEnd, 10);

    if (errno != 0 || *conversionEnd != '\0')
    {
        return 0;
    }

    return 1;
}

int isFloat(const char *str) // Returns 1 if the input string is a valid float (fully parsed by strtod), 0 otherwise.
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }

    errno = 0;
    char *conversionEnd;
    double floatval = strtod(str, &conversionEnd);

    if (errno != 0 || *conversionEnd != '\0')
    {
        return 0;
    }

    return 1;
}

symlst *symbol_list_create() // Allocates and initializes an empty symbol list for a grammar rule's RHS.
{
    symlst *List = (symlst *)malloc(sizeof(symlst));
    if (!List)
    {
        fprintf(stderr, "Error: Unable to allocate memory for symbol List\n");
        return NULL;
    }
    List->count = 0;
    List->head = NULL;
    List->tail = NULL;
    return List;
}

void symbol_list_insert_node(symlst *symlst, symlstnode *symlistnode) // Inserts a new node at the end of the symbol list, updating tail pointer and node count.
{
    if (symlst->tail == NULL)
    {
        symlst->head = symlistnode;
    }
    else
    {
        symlst->tail->next = symlistnode;
        symlistnode->prev = symlst->tail;
    }
    symlst->tail = symlistnode;
    ++(symlst->count);
}

bool compareGramsym(const gramsym *a, const gramsym *b)
{
    if (a->isNonTerminal != b->isNonTerminal)
        return false;
    if (a->isNonTerminal)
        return (a->isTorNT.nt == b->isTorNT.nt);
    else
        return (a->isTorNT.t == b->isTorNT.t);
}

bool symlstContains(const symlst *list, const gramsym *symb)
{
    if (!list || !symb)
        return false;
    symlstnode *curr = list->head;
    while (curr)
    {
        if (compareGramsym(curr->symb, symb))
            return true;
        curr = curr->next;
    }
    return false;
}

// Allocates and initializes a new symbol list node with the provided grammar symbol.
symlstnode *symbol_list_createNode(gramsym *gramsym)
{
    symlstnode *lNode = (symlstnode *)malloc(sizeof(symlstnode));
    if (!lNode)
    {
        fprintf(stderr, "Error: Unable to allocate memory for symbol List Node\n");
        return NULL;
    }
    lNode->next = NULL;
    lNode->prev = NULL;
    lNode->symb = gramsym;
    return lNode;
}

void reverseSymlst(symlst *list)
{
    if (!list)
        return;
    symlstnode *curr = list->head;
    symlstnode *temp = NULL;
    while (curr)
    {

        temp = curr->prev;
        curr->prev = curr->next;
        curr->next = temp;
        curr = curr->prev;
    }

    if (temp)
    {

        list->tail = list->head;
        list->head = temp->prev;
    }
}

// Creates and initializes a new parse tree node with initial children capacity and default settings.
parseT *parse_tree_create()
{
    parseT *T = (parseT *)malloc(sizeof(parseT));
    if (!T)
    {
        fprintf(stderr, "Error: Unable to allocate memory for creating parse tree\n");
        return NULL;
    }
    T->root = parse_tree_node_create();
    return T;
}

parsetreeNode *parse_tree_node_create() // Creates and initializes a new parse tree node with initial children capacity and default settings.
{
    parsetreeNode *temp_node = (parsetreeNode *)malloc(sizeof(parsetreeNode));
    if (!temp_node)
    {
        fprintf(stderr, "Error: Unable to allocate memory for parse tree node\n");
        return NULL;
    }
    temp_node->children = (parsetreeNode **)malloc(CHILD_CAP * sizeof(parsetreeNode *));
    if (!(temp_node->children))
    {
        fprintf(stderr, "Error: Unable to allocate memory for parse tree node's children\n");
        return temp_node;
    }
    temp_node->capacity = CHILD_CAP;
    temp_node->size = 0;
    temp_node->symbol = NULL;
    temp_node->symtbEntry = NULL;
    temp_node->lineNumber = -1;
    for (int i = 0; i < CHILD_CAP; i++)
    {
        temp_node->children[i] = NULL;
    }
    return temp_node;
}

// Adds the child node to the parent's children array, reallocating if necessary.

void insert_node_as_child(parsetreeNode *parent, parsetreeNode *child)
{
    if (parent == NULL)
    {
        printf("Parent node is missing; cannot add child.\n");
        return;
    }
    if (child == NULL)
    {
        printf("Child node is missing; cannot add child.\n");
        return;
    }
    if (parent->size == parent->capacity)
    {
        parent->capacity *= 2;
        parsetreeNode **newChildren = (parsetreeNode **)realloc(parent->children, parent->capacity * sizeof(parsetreeNode *));
        if (newChildren == NULL)
        {
            fprintf(stderr, "Error: Memory reallocation failed for resizing node's children capacity\n");
            return;
        }
        parent->children = newChildren;
    }
    parent->children[parent->size++] = child;
}

void incrementcnt()
{
    // testing auxillary function
    cbbma += 1;
    // printf("cbbma: %d\n", cbbma);
}

// Initializes nonTerminalToString and tokenToString arrays with corresponding string representations for non-terminals and tokens.

void initializeNonTerminalToString()
{

    if (notInit)
        return;
    notInit = true;
    for (int i = 0; i < NT_NOT_FOUND; i++)
    {
        nonTerminalToString[i] = malloc(NON_TERMINAL_LENGTH);
    }
    nonTerminalToString[typeDefinitions] = "<typeDefinitions>";
    nonTerminalToString[actualOrRedefined] = "<actualOrRedefined>";
    nonTerminalToString[program] = "<program>";
    nonTerminalToString[otherFunctions] = "<otherFunctions>";
    nonTerminalToString[mainFunction] = "<mainFunction>";
    nonTerminalToString[iterativeStmt] = "<iterativeStmt>";
    nonTerminalToString[conditionalStmt] = "<conditionalStmt>";
    nonTerminalToString[elsePart] = "<elsePart>";
    nonTerminalToString[declaration] = "<declaration>";
    nonTerminalToString[singleOrRecId] = "<singleOrRecId>";
    nonTerminalToString[option_single_constructed] = "<option_single_constructed>";
    nonTerminalToString[oneExpansion] = "<oneExpansion>";
    nonTerminalToString[global_or_not] = "<global_or_not>";
    nonTerminalToString[arithmeticExpression] = "<arithmeticExpression>";
    nonTerminalToString[logicalOp] = "<logicalOp>";
    nonTerminalToString[relationalOp] = "<relationalOp>";
    nonTerminalToString[optionalReturn] = "<optionalReturn>";
    nonTerminalToString[more_ids] = "<more_ids>";
    nonTerminalToString[A] = "<A>";
    nonTerminalToString[stmt] = "<stmt>";
    nonTerminalToString[ioStmt] = "<ioStmt>";
    nonTerminalToString[funCallStmt] = "<funCallStmt>";
    nonTerminalToString[termPrime] = "<termPrime>";
    nonTerminalToString[factor] = "<factor>";
    nonTerminalToString[highPrecedenceOperators] = "<highPrecedenceOperators>";
    nonTerminalToString[lowPrecedenceOperators] = "<lowPrecedenceOperators>";
    nonTerminalToString[outputParameters] = "<outputParameters>";
    nonTerminalToString[inputParameters] = "<inputParameters>";
    nonTerminalToString[idList] = "<idList>";
    nonTerminalToString[otherStmts] = "<otherStmts>";
    nonTerminalToString[returnStmt] = "<returnStmt>";
    nonTerminalToString[assignmentStmt] = "<assignmentStmt>";
    nonTerminalToString[stmts] = "<stmts>";
    nonTerminalToString[booleanExpression] = "<booleanExpression>";
    nonTerminalToString[var] = "<var>";
    nonTerminalToString[moreExpansions] = "<moreExpansions>";
    nonTerminalToString[expPrime] = "<expPrime>";
    nonTerminalToString[term] = "<term>";
    nonTerminalToString[primitiveDatatype] = "<primitiveDatatype>";
    nonTerminalToString[constructedDatatype] = "<constructedDatatype>";
    nonTerminalToString[fieldType] = "<fieldType>";
    nonTerminalToString[fieldDefinitions] = "<fieldDefinitions>";
    nonTerminalToString[moreFields] = "<moreFields>";
    nonTerminalToString[definetypestmt] = "<definetypestmt>";
    nonTerminalToString[fieldDefinition] = "<fieldDefinition>";
    nonTerminalToString[remaining_list] = "<remaining_list>";
    nonTerminalToString[dataType] = "<dataType>";
    nonTerminalToString[typeDefinition] = "<typeDefinition>";
    nonTerminalToString[declarations] = "<declarations>";
    nonTerminalToString[function] = "<function>";
    nonTerminalToString[input_par] = "<input_par>";
    nonTerminalToString[output_par] = "<output_par>";
    nonTerminalToString[parameter_list] = "<parameter_list>";
}

void initializeTokenToString()
{
    // Initializes the tokenToString array with string representations for all tokens, if not already initialized.
    if (tokenInit)
        return;
    tokenInit = true;
    for (int i = 0; i < MAX_VOC_SIZE; i++)
    {
        tokenToString[i] = malloc(LEX_MAX);
    }
    // Assign each token name as a string (no angle brackets)
    tokenToString[TK_NUM] = "TK_NUM";
    tokenToString[TK_GT] = "TK_GT";
    tokenToString[TK_INPUT] = "TK_INPUT";
    tokenToString[TK_OR] = "TK_OR";
    tokenToString[TK_ENDRECORD] = "TK_ENDRECORD";
    tokenToString[TK_ASSIGNOP] = "TK_ASSIGNOP";
    tokenToString[TK_INT] = "TK_INT";
    tokenToString[TK_AS] = "TK_AS";
    tokenToString[TK_DOT] = "TK_DOT";
    tokenToString[TK_FIELDID] = "TK_FIELDID";
    tokenToString[TK_ENDWHILE] = "TK_ENDWHILE";
    tokenToString[TK_COMMENT] = "TK_COMMENT";
    tokenToString[TK_THEN] = "TK_THEN";
    tokenToString[TK_CL] = "TK_CL";
    tokenToString[TK_MINUS] = "TK_MINUS";
    tokenToString[TK_EQ] = "TK_EQ";
    tokenToString[TK_SQL] = "TK_SQL";
    tokenToString[TK_WRITE] = "TK_WRITE";
    tokenToString[TK_DIV] = "TK_DIV";
    tokenToString[TK_PLUS] = "TK_PLUS";
    tokenToString[TK_ELSE] = "TK_ELSE";
    tokenToString[TK_SQR] = "TK_SQR";
    tokenToString[TK_NOT] = "TK_NOT";
    tokenToString[TK_LIST] = "TK_LIST";
    tokenToString[TK_RNUM] = "TK_RNUM";
    tokenToString[TK_ENDUNION] = "TK_ENDUNION";
    tokenToString[TK_COMMA] = "TK_COMMA";
    tokenToString[TK_END] = "TK_END";
    tokenToString[TK_CALL] = "TK_CALL";
    tokenToString[TK_PARAMETER] = "TK_PARAMETER";
    tokenToString[TK_MAIN] = "TK_MAIN";
    tokenToString[TK_IF] = "TK_IF";
    tokenToString[TK_LT] = "TK_LT";
    tokenToString[TK_READ] = "TK_READ";
    tokenToString[TK_OP] = "TK_OP";
    tokenToString[TK_DOLLAR] = "TK_DOLLAR";
    tokenToString[TK_MUL] = "TK_MUL";
    tokenToString[TK_RETURN] = "TK_RETURN";
    tokenToString[TK_NE] = "TK_NE";
    tokenToString[TK_FUNID] = "TK_FUNID";
    tokenToString[TK_UNION] = "TK_UNION";
    tokenToString[TK_ENDIF] = "TK_ENDIF";
    tokenToString[TK_COLON] = "TK_COLON";
    tokenToString[TK_AND] = "TK_AND";
    tokenToString[TK_RECORD] = "TK_RECORD";
    tokenToString[TK_DEFINETYPE] = "TK_DEFINETYPE";
    tokenToString[TK_OUTPUT] = "TK_OUTPUT";
    tokenToString[TK_REAL] = "TK_REAL";
    tokenToString[TK_PARAMETERS] = "TK_PARAMETERS";
    tokenToString[TK_WITH] = "TK_WITH";
    tokenToString[TK_GE] = "TK_GE";
    tokenToString[TK_TYPE] = "TK_TYPE";
    tokenToString[TK_WHILE] = "TK_WHILE";
    tokenToString[TK_GLOBAL] = "TK_GLOBAL";
    tokenToString[TK_SEM] = "TK_SEM";
    tokenToString[TK_RUID] = "TK_RUID";
    tokenToString[EPS] = "EPS";
    tokenToString[TK_ID] = "TK_ID";
    tokenToString[TK_LE] = "TK_LE";
    tokenToString[LEXICAL_ERROR] = "LEXICAL_ERROR";
    tokenToString[ID_LENGTH_EXC] = "ID_LENGTH_EXC";
    tokenToString[FUN_LENGTH_EXC] = "FUN_LENGTH_EXC";
    tokenToString[VAR_LENGTH_EXC] = "VAR_LENGTH_EXC";
}

// Returns the non-terminal enum corresponding to the input string, or NT_NOT_FOUND if no match is found.
NonTerminal getNonTerminalFromString(char *str)
{
    for (int i = 0; i < NT_NOT_FOUND; i++)
    {
        if (!strcmp(nonTerminalToString[i], str))
            return (NonTerminal)i;
    }
    return NT_NOT_FOUND;
}

// Returns the token enum corresponding to the input string, or MAX_VOC_SIZE if no match is found.
Token getTokenFromString(char *str)
{
    // Given a string, returns the corresponding token's enum
    // printf("hello i am under the water\n");
    // printf("%s\n", str);
    // for (int i = 0; i < MAX_VOC_SIZE; i++)
    // {
    //     printf("%s\n", tokenToString[i]);
    // }
    for (int i = 0; i < MAX_VOC_SIZE; i++)
    {
        // printf("wello\n");
        if (!strcmp(tokenToString[i], str))
            return (Token)i;
    }
    return MAX_VOC_SIZE;
}

// !! use conversion function from the lexer module
grmrule *parseGrammarRule(char *line) {
    char *token_one = strtok(line, " \t\n\r");
    if (!token_one)
        return NULL;
    grmrule *rule = (grmrule *)malloc(sizeof(grmrule));
    if (!rule) {
        fprintf(stderr, "Error: Unable to allocate memory for rule\n");
        return NULL;
    }
    rule->lhs = (gramsym *)malloc(sizeof(gramsym));
    if (!rule->lhs) {
        fprintf(stderr, "Error: Unable to allocate memory for grammar symbol\n");
        free(rule);
        return NULL;
    }
    rule->lhs->isNonTerminal = true;
    rule->lhs->isTorNT.nt = getNonTerminalFromString(token_one);
    rule->rhs = symbol_list_create();
    while ((token_one = strtok(NULL, " \t\n\r"))) {
        gramsym *gs = (gramsym *)malloc(sizeof(gramsym));
        if (!gs) {
            fprintf(stderr, "Error: Unable to allocate memory for grammar symbol\n");
            break;
        }
        if (token_one[0] == '<') {
            gs->isNonTerminal = true;
            gs->isTorNT.nt = getNonTerminalFromString(token_one);
        } else {
            gs->isNonTerminal = false;
            gs->isTorNT.t = getTokenFromString(token_one);
        }
        symlstnode *node = symbol_list_createNode(gs);
        symbol_list_insert_node(rule->rhs, node);
    }
    return rule;
}

void readGrammar() {
    if (grammarRead)
        return;
    grammarRead = true;
    FILE *fp = fopen("grammar.txt", "r");
    if (!fp) {
        fprintf(stderr, "Error: Unable to open grammar file\n");
        return;
    }
    char readBuffer[RULE_LEN_MAX];
    while (fgets(readBuffer, RULE_LEN_MAX, fp)) {
        grmrule *rule = parseGrammarRule(readBuffer);
        if (rule)
            Grammar[numrules++] = rule;
    }
    fclose(fp);
}

bool updateFirstSetForRule(grmrule *rule) {
    bool modified = false;
    NonTerminal lhs = rule->lhs->isTorNT.nt;
    symlstnode *curr = rule->rhs->head;
    bool canDeriveEps = true;
    while (curr && canDeriveEps) {
        canDeriveEps = false;
        if (!curr->symb->isNonTerminal) {
            if (!exists_in_rhs(cmpFirst[lhs], curr->symb->isTorNT.t)) {
                frstfwdRHSNode *newNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                newNode->tk = curr->symb->isTorNT.t;
                newNode->next = NULL;
                insert_ff_rhs_node(cmpFirst[lhs], newNode);
                modified = true;
            }
            break;
        } else {
            for (frstfwdRHSNode *itr = cmpFirst[curr->symb->isTorNT.nt]->head; itr; itr = itr->next) {
                if (itr->tk == EPS) {
                    canDeriveEps = true;
                    continue;
                }
                if (!exists_in_rhs(cmpFirst[lhs], itr->tk)) {
                    frstfwdRHSNode *copyNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                    copyNode->tk = itr->tk;
                    copyNode->next = NULL;
                    insert_ff_rhs_node(cmpFirst[lhs], copyNode);
                    modified = true;
                }
            }
        }
        curr = curr->next;
    }
    if (!curr && canDeriveEps && !exists_in_rhs(cmpFirst[lhs], EPS)) {
        frstfwdRHSNode *epsNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
        epsNode->tk = EPS;
        epsNode->next = NULL;
        insert_ff_rhs_node(cmpFirst[lhs], epsNode);
        modified = true;
    }
    return modified;
}

void computeFirstSets() {
    bool modified = true;
    while (modified) {
        modified = false;
        for (int i = 0; i < numrules; i++) {
            if (updateFirstSetForRule(Grammar[i]))
                modified = true;
        }
    }
}

frstfwdRHS *get_first_of_rhs_from_node(symlstnode *node) {
    symlst tempList;
    tempList.head = node;
    tempList.count = 0;
    return get_first_of_rhs(&tempList);
}

bool suffix_can_derive_epsilon(symlstnode *node) {
    frstfwdRHS *firstSuffix = get_first_of_rhs_from_node(node);
    for (frstfwdRHSNode *itr = firstSuffix->head; itr; itr = itr->next) {
        if (itr->tk == EPS) {
            free_up_ff_rhs(firstSuffix);
            return true;
        }
    }
    free_up_ff_rhs(firstSuffix);
    return false;
}

bool updateFollowSetForRule(grmrule *rule) {
    bool modified = false;
    NonTerminal lhs = rule->lhs->isTorNT.nt;
    for (symlstnode *curr = rule->rhs->head; curr; curr = curr->next) {
        if (!curr->symb->isNonTerminal)
            continue;
        frstfwdRHS *firstSuffix = get_first_of_rhs_from_node(curr->next);
        for (frstfwdRHSNode *itr = firstSuffix->head; itr; itr = itr->next) {
            if (itr->tk == EPS)
                continue;
            if (!exists_in_rhs(cmpflw[curr->symb->isTorNT.nt], itr->tk)) {
                frstfwdRHSNode *newNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                newNode->tk = itr->tk;
                newNode->next = NULL;
                insert_ff_rhs_node(cmpflw[curr->symb->isTorNT.nt], newNode);
                modified = true;
            }
        }
        free_up_ff_rhs(firstSuffix);
        if (suffix_can_derive_epsilon(curr->next) || curr->next == NULL) {
            for (frstfwdRHSNode *itr = cmpflw[lhs]->head; itr; itr = itr->next) {
                if (!exists_in_rhs(cmpflw[curr->symb->isTorNT.nt], itr->tk)) {
                    frstfwdRHSNode *newNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                    newNode->tk = itr->tk;
                    newNode->next = NULL;
                    insert_ff_rhs_node(cmpflw[curr->symb->isTorNT.nt], newNode);
                    modified = true;
                }
            }
        }
    }
    return modified;
}

void computeFollowSets() {
    frstfwdRHSNode *endNode = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
    endNode->tk = TK_DOLLAR;
    endNode->next = NULL;
    insert_ff_rhs_node(cmpflw[program], endNode);
    bool modified = true;
    while (modified) {
        modified = false;
        for (int i = 0; i < numrules; i++) {
            if (updateFollowSetForRule(Grammar[i]))
                modified = true;
        }
    }
}

void computeFirstAndFollow() {
    if (frstflwInit)
        return;
    frstflwInit = true;
    cmpFirst = (frstfwdRHS **)malloc(NT_NOT_FOUND * sizeof(frstfwdRHS *));
    cmpflw   = (frstfwdRHS **)malloc(NT_NOT_FOUND * sizeof(frstfwdRHS *));
    if (!cmpFirst || !cmpflw) {
        printf("Could not allocate memory for FIRST/FOLLOW arrays\n");
        return;
    }
    for (int i = 0; i < NT_NOT_FOUND; i++) {
        cmpFirst[i] = (frstfwdRHS *)malloc(sizeof(frstfwdRHS));
        cmpFirst[i]->head = NULL;
        cmpFirst[i]->tail = NULL;
        cmpflw[i] = (frstfwdRHS *)malloc(sizeof(frstfwdRHS));
        cmpflw[i]->head = NULL;
        cmpflw[i]->tail = NULL;
    }
    computeFirstSets();
    computeFollowSets();
}
// Prints all grammar rules stored in the Grammar array to the console.
void printGrammar()
{
    FILE *filepointer = fopen("readgrm.txt", "w");
    fflush(filepointer);
    // printf("Displaying grammar %d\n", numrules);

    for (int i = 0; i < numrules; i++)
    {
        grmrule *rule = Grammar[i];
        fprintf(filepointer, "%d. %s: ", i + 1, nonTerminalToString[rule->lhs->isTorNT.nt]);

        symlstnode *node = rule->rhs->head;
        for (int j = 0; j < rule->rhs->count; j++)
        {
            if (node->symb->isNonTerminal)
            {
                fprintf(filepointer, "%s\t", nonTerminalToString[node->symb->isTorNT.nt]);
            }
            else
            {
                fprintf(filepointer, "%s\t", tokenToString[node->symb->isTorNT.t]);
            }
            node = node->next;
        }
        fprintf(filepointer, "\n");
    }
}

bool removeTokenFromFrstfwdRHS(frstfwdRHS *list, Token tk)
{
    if (!list)
        return false;
    frstfwdRHSNode *curr = list->head;
    frstfwdRHSNode *prev = NULL;
    while (curr)
    {
        if (curr->tk == tk)
        {

            if (prev)
                prev->next = curr->next;
            else
                list->head = curr->next;
            if (curr == list->tail)
                list->tail = prev;
            free(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void mergeFrstfwdRHS(frstfwdRHS *target, const frstfwdRHS *source)
{
    if (!target || !source)
        return;
    frstfwdRHSNode *curr = source->head;
    while (curr)
    {
        // addTokenToFrstfwdRHS(target, curr->tk);
        curr = curr->next;
    }
}

// Inserts a node into the frstfwdRHS list (first/follow set) and updates head and tail pointers.
void insert_ff_rhs_node(frstfwdRHS *list, frstfwdRHSNode *node)
{
    if (list->tail == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }
}

// Returns true if the token tmpkey is found in the frstfwdRHS list, false otherwise.
bool exists_in_rhs(frstfwdRHS *list, Token _key)
{
    if (!list)
        return false;
    frstfwdRHSNode *curr_node = list->head;
    for (; curr_node; curr_node = curr_node->next)
    {
        if (curr_node->tk == _key)
            return true;
    }
    return false;
}

// Computes and returns the FIRST set for the given grammar rule's right-hand side.
frstfwdRHS *get_first_of_rhs(symlst *grammar_rhs)
{
    frstfwdRHS *rf = (frstfwdRHS *)malloc(sizeof(frstfwdRHS));
    rf->head = NULL;
    rf->tail = NULL;

    symlstnode *head_r = grammar_rhs->head;
    bool hasEps = true;
    for (; head_r && hasEps; head_r = head_r->next)
    {
        frstfwdRHSNode *temp;
        hasEps = false;
        if (!(head_r->symb->isNonTerminal))
        {
            if (!exists_in_rhs(rf, head_r->symb->isTorNT.t))
            {
                frstfwdRHSNode *temp1 = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                temp1->next = NULL;
                temp1->tk = head_r->symb->isTorNT.t;
                insert_ff_rhs_node(rf, temp1);
            }
            break;
        }
        for (temp = cmpFirst[head_r->symb->isTorNT.nt]->head; temp; temp = temp->next)
        {
            if (temp->tk == EPS)
            {
                hasEps = true;
                continue;
            }
            if (!exists_in_rhs(rf, temp->tk))
            {
                frstfwdRHSNode *temp_copy = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
                temp_copy->next = NULL;
                temp_copy->tk = temp->tk;
                insert_ff_rhs_node(rf, temp_copy);
            }
        }
    }
    if (head_r == NULL && hasEps)
    {
        frstfwdRHSNode *temp2 = (frstfwdRHSNode *)malloc(sizeof(frstfwdRHSNode));
        temp2->next = NULL;
        temp2->tk = EPS;
        if (!exists_in_rhs(rf, EPS))
            insert_ff_rhs_node(rf, temp2);
    }
    return rf;
}

// Frees all nodes in the frstfwdRHS list to release allocated memory.
void free_up_ff_rhs(frstfwdRHS *list_r)
{
    if (!list_r)
    {
        return;
    }
    frstfwdRHSNode *temp1 = list_r->head;
    while (temp1 != NULL)
    {
        frstfwdRHSNode *temp2 = temp1;
        temp1 = temp1->next;
        free(temp2);
    }
}

// Writes computed FIRST and FOLLOW sets for each non-terminal to their respective files.
void printComputedFirstAndFollow()
{
    FILE *file_follow = fopen("computedfollow.txt", "w");
    for (int i = 0; i < NT_NOT_FOUND; i++)
    {
        fprintf(file_follow, "%s:\t", nonTerminalToString[i]);
        frstfwdRHSNode *temp_node1 = cmpflw[i]->head;
        while (temp_node1)
        {
            fprintf(file_follow, "%s\t", tokenToString[temp_node1->tk]);
            temp_node1 = temp_node1->next;
        }
        fprintf(file_follow, "\n");
    }

    FILE *file_first = fopen("computedfirst.txt", "w");
    for (int i = 0; i < NT_NOT_FOUND; i++)
    {
        fprintf(file_first, "%s:\t", nonTerminalToString[i]);
        frstfwdRHSNode *temp_node = cmpFirst[i]->head;
        while (temp_node)
        {
            fprintf(file_first, "%s\t", tokenToString[temp_node->tk]);
            temp_node = temp_node->next;
        }
        fprintf(file_first, "\n");
    }

    fclose(file_first);
    fclose(file_follow);
}

void add_rules_to_parse_table() // Populates the parse table with grammar rules using their computed FIRST and FOLLOW sets.
{
    for (int gri = 0; gri < numrules; ++gri)
    {
        grmrule *temp_rule = Grammar[gri];
        frstfwdRHS *first_Rhs = get_first_of_rhs(temp_rule->rhs);
        for (frstfwdRHSNode *rhs1 = first_Rhs->head; rhs1 != NULL; rhs1 = rhs1->next)
        {
            if (parseTable[temp_rule->lhs->isTorNT.nt][rhs1->tk] != NULL)
            {
                printf("\nWarning: Duplicate entry in parse table detected; the existing rule will be replaced\n");
            }
            parseTable[temp_rule->lhs->isTorNT.nt][rhs1->tk] = temp_rule;
        }
        if (exists_in_rhs(first_Rhs, EPS))
        {
            for (frstfwdRHSNode *rhs2 = cmpflw[temp_rule->lhs->isTorNT.nt]->head; rhs2 != NULL; rhs2 = rhs2->next)
            {
                if (parseTable[temp_rule->lhs->isTorNT.nt][rhs2->tk] != NULL)
                {
                    printf("\nWarning: Duplicate entry in parse table detected; the existing rule will be replaced\n");
                }
                parseTable[temp_rule->lhs->isTorNT.nt][rhs2->tk] = temp_rule;
            }
        }
        free_up_ff_rhs(first_Rhs);
    }
}

void printRule(grmrule *rule, FILE *fp)
{
    fprintf(fp, "Lhs: %s\n", nonTerminalToString[rule->lhs->isTorNT.nt]);
    fprintf(fp, "Rhs: ");
    symlstnode *tmp;
    for (tmp = rule->rhs->head; tmp; tmp = tmp->next)
    {
        fprintf(fp, "%s\t", tmp->symb->isNonTerminal ? nonTerminalToString[tmp->symb->isTorNT.nt] : tokenToString[tmp->symb->isTorNT.t]);
    }
    fprintf(fp, "\n");
}

void initializeParseTable() // Allocates, initializes, and populates the parse table structure for grammar rules.
{
    if (PTreeInit)
    {
        return;
    }
    PTreeInit = true;
    parseTable = (grmrule ***)malloc(NT_NOT_FOUND * sizeof(grmrule **));
    if (!parseTable)
    {
        fprintf(stderr, "Error: Unable to allocate memory for parseTable rows\n");
        return;
    }
    for (int nonter_i = 0; nonter_i < NT_NOT_FOUND; nonter_i++)
    {
        parseTable[nonter_i] = (grmrule **)malloc(MAX_VOC_SIZE * sizeof(grmrule *));
        if (!parseTable[nonter_i])
        {
            fprintf(stderr, "Error: Unable to allocate memory for parseTable columns\n");
            return;
        }
        for (int ter_j = 0; ter_j < MAX_VOC_SIZE; ter_j++)
        {
            parseTable[nonter_i][ter_j] = NULL;
        }
    }
    add_rules_to_parse_table();
}

// Prints the complete parse table to "parseTable.txt", showing each grammar rule for every non-terminal and terminal.
void print_parse_table()
{
    FILE *outFile = fopen("parseTable.txt", "w");
    if (!outFile)
    {
        printf("Unable to open file to write the parse table.\n");
        return;
    }
    for (int nonTermIdx = 0; nonTermIdx < NT_NOT_FOUND; nonTermIdx++)
    {
        fprintf(outFile, "------------------------------Non Terminal %d -----------------------------\n", nonTermIdx);
        for (int termIdx = 0; termIdx < MAX_VOC_SIZE; termIdx++)
        {
            fprintf(outFile, "******Terminal %d ******\n", termIdx);
            grmrule *ruleEntry = parseTable[nonTermIdx][termIdx];
            if (ruleEntry)
            {
                printRule(ruleEntry, outFile);
            }
            else
            {
                fprintf(outFile, "Error\n");
            }
        }
    }
    fclose(outFile);
}

bool shouldSkipToken(tokenInfo tInfo) {
    return (tInfo->tokenName == TK_COMMENT || tInfo->tokenName >= LEXICAL_ERROR);
}

void handleSkippedToken(lln *currentToken, FILE *fp, bool *hasSyntaxError) {
    if (currentToken->symTBEntry->tokenName >= LEXICAL_ERROR)
        *hasSyntaxError = true;
}

void handleTerminalMatch(Stack *stack, parsetreeNode *node, lln **currentToken, FILE *fp) {
    node->lineNumber = (*currentToken)->symTBEntry->lineNumber;
    node->symtbEntry = (*currentToken)->symTBEntry;
    pop(stack);
    *currentToken = (*currentToken)->next;
}

void handleNonTerminalExpansion(Stack *stack, parsetreeNode *node, lln *currentToken) {
    grmrule *rule = parseTable[node->symbol->isTorNT.nt][currentToken->symTBEntry->tokenName];
    pop(stack);
    node->lineNumber = currentToken->symTBEntry->lineNumber;
    for (symlstnode *iter = rule->rhs->head; iter; iter = iter->next) {
        parsetreeNode *child = parse_tree_node_create();
        child->symbol = (gramsym *)malloc(sizeof(gramsym));
        child->symbol->isNonTerminal = iter->symb->isNonTerminal;
        if (child->symbol->isNonTerminal)
            child->symbol->isTorNT.nt = iter->symb->isTorNT.nt;
        else
            child->symbol->isTorNT.t = iter->symb->isTorNT.t;
        insert_node_as_child(node, child);
    }
    for (int i = node->size - 1; i >= 0; i--) {
        push(stack, node->children[i]);
    }
}

void reportTerminalMismatch(FILE *fp, parsetreeNode *node, lln *currentToken) {
    fprintf(fp, "Line %5d \tError: Terminal token %s (lexeme \"%s\") does not match expected token %s\n",
            currentToken->symTBEntry->lineNumber,
            tokenToString[currentToken->symTBEntry->tokenName],
            currentToken->symTBEntry->lexeme,
            tokenToString[node->symbol->isTorNT.t]);
}

void reportNonTerminalError(FILE *fp, parsetreeNode *node, lln *currentToken) {
    fprintf(fp, "Line %5d \tError: Invalid token %s (lexeme \"%s\") for non-terminal %s\n",
            currentToken->symTBEntry->lineNumber,
            tokenToString[currentToken->symTBEntry->tokenName],
            currentToken->symTBEntry->lexeme,
            nonTerminalToString[node->symbol->isTorNT.nt]);
}

void finalizeParsing(Stack *stack, lln *currentToken, FILE *fp, int lastLine, bool *hasSyntaxError) {
    while (!isEmpty(stack)) {
        parsetreeNode *node = top(stack);
        fprintf(fp, "Line %5d \tError: Unmatched non-terminal %s\n",
                lastLine, nonTerminalToString[node->symbol->isTorNT.nt]);
        pop(stack);
    }
    while (currentToken && currentToken->symTBEntry->tokenName != TK_DOLLAR) {
        fprintf(fp, "Line %5d \tError: Extra token %s (lexeme \"%s\") encountered\n",
                currentToken->symTBEntry->lineNumber,
                tokenToString[currentToken->symTBEntry->tokenName],
                currentToken->symTBEntry->lexeme);
        currentToken = currentToken->next;
        *hasSyntaxError = true;
    }
}

parseT *parseTokens(linkedList *tokenList, bool *hasSyntaxError, FILE *fp) {
    if (!tokenList) {
        fprintf(fp, "Tokens list is null. Parsing failed.\n");
        return NULL;
    }
    lln *currentToken = tokenList->head;
    parseT *parseTree = parse_tree_create();
    parsetreeNode *root = parseTree->root;
    gramsym *startSymbol = (gramsym *)malloc(sizeof(gramsym));
    startSymbol->isNonTerminal = true;
    startSymbol->isTorNT.nt = program;
    root->symbol = startSymbol;
    Stack *stack = createNewStack();
    push(stack, root);
    while (!isEmpty(stack) && currentToken) {
        parsetreeNode *node = top(stack);
        if (shouldSkipToken(currentToken->symTBEntry)) {
            handleSkippedToken(currentToken, fp, hasSyntaxError);
            currentToken = currentToken->next;
            continue;
        }
        if (!node->symbol->isNonTerminal && node->symbol->isTorNT.t == EPS) {
            node->lineNumber = currentToken->symTBEntry->lineNumber;
            tokenInfo tInfo = (tokenInfo)malloc(sizeof(struct tokenInfo));
            strcpy(tInfo->lexeme, "EPSILON");
            tInfo->tokenName = EPS;
            node->symtbEntry = tInfo;
            pop(stack);
            continue;
        }
        if (!node->symbol->isNonTerminal) {
            if (node->symbol->isTorNT.t == currentToken->symTBEntry->tokenName) {
                handleTerminalMatch(stack, node, &currentToken, fp);
            } else {
                *hasSyntaxError = true;
                reportTerminalMismatch(fp, node, currentToken);
                pop(stack);
            }
        }
        else if (parseTable[node->symbol->isTorNT.nt][currentToken->symTBEntry->tokenName] == NULL) {
            *hasSyntaxError = true;
            reportNonTerminalError(fp, node, currentToken);
            if (exists_in_rhs(cmpflw[node->symbol->isTorNT.nt], currentToken->symTBEntry->tokenName))
                pop(stack);
            else
                currentToken = currentToken->next;
        }
        else {
            handleNonTerminalExpansion(stack, node, currentToken);
        }
    }
    finalizeParsing(stack, currentToken, fp, currentToken ? currentToken->symTBEntry->lineNumber : 1, hasSyntaxError);
    return parseTree;
}





void print_tree_node(parsetreeNode *node, parsetreeNode *parent, FILE *file)
{
    // Prints the details of a specific tree node on a line in the specified file using the given format

    fprintf(file, "\n");
    fprintf(file, "%*s ", 32, !node->symbol->isNonTerminal ? node->symtbEntry->lexeme : "-----");
    fprintf(file, "%*d ", 12, node->lineNumber);
    fprintf(file, "%*s ", 16, node->symbol->isNonTerminal ? "-----" : tokenToString[node->symtbEntry->tokenName]);
    if (!(node->symbol->isNonTerminal) && (node->symtbEntry->tokenName == TK_NUM || node->symtbEntry->tokenName == TK_RNUM))
    {
        if (node->symtbEntry->tokenName == TK_NUM)
            fprintf(file, "%*d ", 20, atoi(node->symtbEntry->lexeme));
        else
        {
            char *endPtr;
            fprintf(file, "%20.2lf ", strtod(node->symtbEntry->lexeme, &endPtr));
        }
    }
    else
    {
        fprintf(file, "%*s ", 20, "Not number ");
    }
    fprintf(file, "%*s ", 30, parent ? nonTerminalToString[parent->symbol->isTorNT.nt] : "ROOT");
    fprintf(file, "%*s ", 12, node->symbol->isNonTerminal ? "NO" : "YES");
    fprintf(file, "%*s ", 30, node->symbol->isNonTerminal ? nonTerminalToString[node->symbol->isTorNT.nt] : "-----");
    fprintf(file, "\n");
}

void printShortTreeNode(parsetreeNode *currentNode, parsetreeNode *parentNode, FILE *outFile)
{
    if (!currentNode)
    {
        fprintf(outFile, "\n[NULL node encountered]\n");
        return;
    }

    fprintf(outFile, "\n");

    // Print lexeme if this node is a terminal and the lexeme exists; otherwise, print a placeholder.
    if (currentNode->symbol && !currentNode->symbol->isNonTerminal && currentNode->symtbEntry && currentNode->symtbEntry->lexeme)
    {
        fprintf(outFile, "%32s ", currentNode->symtbEntry->lexeme);
    }
    else
    {
        fprintf(outFile, "%32s ", "-----");
    }

    fprintf(outFile, "%12d ", currentNode->lineNumber);

    // For non-terminals, print a placeholder; for terminals, print the token name if available.
    if (currentNode->symbol && currentNode->symbol->isNonTerminal)
    {
        fprintf(outFile, "%16s ", "-----");
    }
    else if (currentNode->symtbEntry)
    {
        fprintf(outFile, "%16s ", tokenToString[currentNode->symtbEntry->tokenName]);
    }
    else
    {
        fprintf(outFile, "%16s ", "NULL");
    }

    // If the node represents a number token, print its numeric value.
    if (currentNode->symbol && !currentNode->symbol->isNonTerminal && currentNode->symtbEntry &&
        (currentNode->symtbEntry->tokenName == TK_NUM || currentNode->symtbEntry->tokenName == TK_RNUM))
    {
        if (currentNode->symtbEntry->tokenName == TK_NUM)
        {
            if (currentNode->symtbEntry->lexeme)
                fprintf(outFile, "%20d ", atoi(currentNode->symtbEntry->lexeme));
            else
                fprintf(outFile, "%20s ", "NULL");
        }
        else
        {
            char *endPointer = NULL;
            if (currentNode->symtbEntry->lexeme)
                fprintf(outFile, "%20.2lf ", strtod(currentNode->symtbEntry->lexeme, &endPointer));
            else
                fprintf(outFile, "%20s ", "NULL");
        }
    }
    else
    {
        fprintf(outFile, "%20s ", "Not number ");
    }

    // Print the parent's non-terminal details if available, otherwise mark as ROOT.
    if (parentNode && parentNode->symbol)
        fprintf(outFile, "%30s ", nonTerminalToString[parentNode->symbol->isTorNT.nt]);
    else
        fprintf(outFile, "%30s ", "ROOT");

    // Print whether this node is a terminal and the corresponding symbol details.
    if (currentNode->symbol)
    {
        if (currentNode->symbol->isNonTerminal)
        {
            fprintf(outFile, "%12s ", "NO");
            fprintf(outFile, "%30s ", nonTerminalToString[currentNode->symbol->isTorNT.nt]);
        }
        else
        {
            fprintf(outFile, "%12s ", "YES");
            fprintf(outFile, "%30s ", "-----");
        }
    }
    else
    {
        fprintf(outFile, "%12s ", "NULL");
        fprintf(outFile, "%30s ", "NULL");
    }

    fprintf(outFile, "\n");
}

void inorderTraverse(parsetreeNode *node, parsetreeNode *parent, FILE *file)
{
    // Traverses the parse tree in-order and prints node details

    if (!node)
        return;

    if (node->size)
        inorderTraverse(node->children[0], node, file);
    printShortTreeNode(node, parent, file);
    for (int childIndex = 1; childIndex < node->size; ++childIndex)
        inorderTraverse(node->children[childIndex], node, file);
}

void printParseTree(parseT *parseTree, char *outputFilename)
{
    FILE *file = fopen(outputFilename, "w");
    if (!file)
    {
        printf("Could not open file for printing parse tree\n");
        return;
    }
    if (!parseTree)
    {
        printf("Given parse tree is null. Could not print\n");
        return;
    }
    if (canPrint)
        printf("Printing Parse Tree in the specified file...\n");
    fprintf(file, "%*s %*s %*s %*s %*s %*s %*s\n\n",
            32, "lexeme",
            12, "lineNumber",
            16, "tokenName",
            20, "valueIfNumber",
            30, "parentNodeSymbol",
            12, "isLeafNode",
            30, "nodeSymbol");
    inorderTraverse(parseTree->root, NULL, file);
    fclose(file);
    if (canPrint)
        printf("Printing parse tree completed...\n");
}

void parseInputSourceCode(char *inpFile, char *opFile)
{

    FILE *ifp = fopen(inpFile, "r");
    if (!ifp)
    {
        printf("Could not open file input file for parsing\n");
        return;
    }
    printf("All the errors while parsing the code are dispayed on console and printed in ParserErrors.txt\n");
    printf("The parse tree is printed in the file %s\n", opFile);
    linkedList *tokensFromLexer = processLexicalTokens(ifp, opFile);
    printf("Lexical analysis completed successfully\n");
    // fclose(ifp);
    initializeTokenToString();
    initializeNonTerminalToString();
    readGrammar();
    printf("Grammar read successfully\n");
    // printGrammar();
    computeFirstAndFollow();
    printf("First and Follow sets computed successfully\n");
    printComputedFirstAndFollow();
    initializeParseTable();
    printf("Parse table filled successfully\n");
    // print_parse_table();

    FILE *fpout = fopen("ParserErrors.txt", "w");
    if (!fpout)
    {
        printf("Could not open file for parser output\n");
        return;
    }
    bool hasSyntaxError = false;
    parseT *parseTree = parseTokens(tokensFromLexer, &hasSyntaxError, fpout);
    fclose(fpout);
    if (!hasSyntaxError)
        printParseTree(parseTree, opFile);
    else
    {
        FILE *foptp = fopen(opFile, "w");
        if (!foptp)
        {
            printf("Could not open file for printing parse tree\n");
            return;
        }
        printf("Though the code has syntactic errors, the partial parse tree obtained by error recovery is printed\n");
        printParseTree(parseTree, opFile);
        // fprintf(foptp, "There were syntax errors in the input file. Not printing the parse tree!\nCheck the console for error details.");
        // fclose(foptp);
    }
    freeLinkedList(tokensFromLexer);
}