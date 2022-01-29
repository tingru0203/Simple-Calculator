#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAXLEN 256
#define MAXREG 8
#define TBLSIZE 64 // memory
// Set PRINTERR to 1 to print error message while calling error()
#define PRINTERR 0
// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

/* lexical analyzer */
// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV, LOGICAL,
    INCDEC, ASSIGN,
    LPAREN, RPAREN
} TokenSet;
// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

TokenSet getToken(void) {
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t'); // skip space

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == lexeme[0]) { //++ or --
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        } else {
            ungetc(c, stdin);
            lexeme[1] = '\0';
            return ADDSUB;
        }
    } else if (c == '&' || c == '|' || c == '^') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return LOGICAL;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isalpha(c) || isdigit(c) || c == '_') {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}

/* parser */
// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;
// Structure of the symbol table
typedef struct {
    int change; // value assigned or not
    int val;
    char name[MAXLEN];
} Symbol;
// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;
// register
int reg[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // register available or not
// The symbol table
Symbol table[TBLSIZE];
// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val, int chg);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);

BTNode *factor(void);
BTNode *term(void);
BTNode *term_tail(BTNode *left);
BTNode *expr(void);
BTNode *expr_tail(BTNode *left);
void statement(void);
char* printAssemblyCode(BTNode *root);

// Print error message and exit the program
void err(ErrorType errorNum);

int sbcount = 0; // number of symbol in the table
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    table[0].change = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    table[1].change = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    table[2].change = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    error(NOTFOUND);
    return 0;
}

int setval(char *str, int val, int chg) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            table[i].change = chg;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    table[sbcount].change = chg;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// factor := INT | ADDSUB INT |
//		   	 ID  | ADDSUB ID  |
//		   	 INCDEC ID | ID ASSIGN expr |
//		   	 LPAREN expr RPAREN |
//		   	 ADDSUB LPAREN expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;

    if (match(INT)) { // INT
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        left = makeNode(ID, getLexeme());
        advance();
        if (!match(ASSIGN)) { // ID
            retp = left;
        } else { // ID ASSIGN expr
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = expr();
        }
    } else if (match(ADDSUB)) {
        retp = makeNode(ADDSUB, getLexeme());
        retp->left = makeNode(INT, "0");
        advance();
        if (match(INT)) { // ADDSUB INT
            retp->right = makeNode(INT, getLexeme());
            advance();
        } else if (match(ID)) { // ADDSUB ID
            retp->right = makeNode(ID, getLexeme());
            advance();
        } else if (match(LPAREN)) {
            advance();
            retp->right = expr();
            if (match(RPAREN)) // ADDSUB LPAREN expr RPAREN
                advance();
            else
                error(MISPAREN);
        } else {
            error(NOTNUMID);
        }
    } else if (match(LPAREN)) {
        advance();
        retp = expr();
        if (match(RPAREN)) // LPAREN expr RPAREN
            advance();
        else
            error(MISPAREN);
    } else if(match(INCDEC)) {
        // ID = ID ADDSUB 1
        retp = makeNode(ASSIGN, "=");
        retp->right = makeNode(ADDSUB, getLexeme());
        advance();
        if(match(ID)) { // INCDEC ID
            retp->left = makeNode(ID, getLexeme());
            retp->right->left = makeNode(ID, getLexeme());
            retp->right->right = makeNode(INT, "1");
            advance();
        } else
            error(NOTNUMID);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

// term := factor term_tail
BTNode *term(void) {
    BTNode *node = factor();
    return term_tail(node);
}

// term_tail := MULDIV factor term_tail | NiL
BTNode *term_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = factor();
        return term_tail(node);
    } else {
        return left;
    }
}

// expr := term expr_tail
BTNode *expr(void) {
    BTNode *node = term();
    return expr_tail(node);
}

// expr_tail := ADDSUB_LOGICAL term expr_tail | NiL
BTNode *expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = term();
        return expr_tail(node);
    } else if (match(LOGICAL)) {
        node = makeNode(LOGICAL, getLexeme());
        advance();
        node->left = left;
        node->right = term();
        return expr_tail(node);
    } else {
        return left;
    }
}

// statement := ENDFILE | END | expr END
void statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        retp = expr();
        if (match(END)) {
            // find ASSIGN
            while(retp!=NULL && retp->data != ASSIGN) {
                retp = retp->right;
            }
            if(retp != NULL) {
                char regIdx[5];
                strcpy(regIdx, printAssemblyCode(retp));
                if(regIdx[0]=='r')
                    reg[regIdx[1]-'0'] = 1; // release register
                freeTree(retp);
            }
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    printf("EXIT 1\n");
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    exit(0);
}

/* code generator */
char ret[50];

// int to string
void itos(int n, char* s) {
    int num = n, cnt = 0, neg = 0;
    if(n < 0) {
        neg = 1;
        s[0] = '-';
        n *= -1;
    }
    do {
        num /= 10;
        cnt++;
    } while(num != 0);
    for(int j = cnt-1; j >= 0; j--) {
        s[j+neg] = n%10+'0';
        n /= 10;
    }
    s[cnt+neg]='\0';
}

// find the variable defined in the table
int findVariable(BTNode* root) {
    for(int i = 0; i < sbcount; i++) {
        if(strcmp(root->lexeme, table[i].name) == 0)
            return i;
    }
    err(NOTFOUND);
}

// find an available register
int findRegister(BTNode* root) {
    for(int i = 3; i < MAXREG; i++) {
        if(reg[i]) {
            reg[i] = 0;
            return i;
        }
    }
    err(RUNOUT);
}

void printOP(BTNode* root, int r1, int r2) {
    switch(root->lexeme[0]) {
        case '+':
            printf("ADD r%d r%d\n", r1, r2);
            break;
        case '-':
            printf("SUB r%d r%d\n", r1, r2);
            break;
        case '*':
            printf("MUL r%d r%d\n", r1, r2);
            break;
        case '/':
            printf("DIV r%d r%d\n", r1, r2);
            break;
        case '&':
            printf("AND r%d r%d\n", r1, r2);
            break;
        case '|':
            printf("OR r%d r%d\n", r1, r2);
            break;
        case '^':
            printf("XOR r%d r%d\n", r1, r2);
            break;
    }
}

void INT_INT(BTNode* root, char* left, char* right) {
    // root->data: from OP to INT
    root->data = INT;

    // root->lexeme = INT OP INT
    switch(root->lexeme[0]) {
        case '+':
            itos(atoi(left) + atoi(right), root->lexeme);
            break;
        case '-':
            itos(atoi(left) - atoi(right), root->lexeme);
            break;
        case '*':
            itos(atoi(left) * atoi(right), root->lexeme);
            break;
        case '/':
            if (right[0]=='0')
                err(DIVZERO);
            itos(atoi(left) / atoi(right), root->lexeme);
            break;
        case '&':
            itos(atoi(left) & atoi(right), root->lexeme);
            break;
        case '|':
            itos(atoi(left) | atoi(right), root->lexeme);
            break;
        case '^':
            itos(atoi(left) ^ atoi(right), root->lexeme);
            break;
    }
}

int p = 0;

// return INT or register
char* printAssemblyCode(BTNode *root) {
    // first, move x, y, z to r0, r1, r2 (can direct use r0, r1, r2 later)
    if(p == 0) {
        printf("MOV r0 [0]\nMOV r1 [4]\nMOV r2 [8]\n");
        p++;
    }

    char left[50], right[50];
    int i, j;

    switch (root->data) {
        case ID:
            i = findVariable(root);
            if(table[i].change) { // ID's value is known
                // root: from ID to INT
                root->data = INT; // change root->data
                itos(table[i].val, root->lexeme); // change root->lexeme
                // return INT
                strcpy(ret, root->lexeme);
            }
            else if(i <= 2) { // ID's value is unknown & ID is x, y, z
                // return reg(root)
                ret[0] = 'r';
                ret[1] = i+'0';
                ret[2] = '\0';
            }
            else { // ID's value is unknown & ID is other variable
                // find a new register to store ID
                j = findRegister(root);
                // MOV reg(new) mem(root)
                printf("MOV r%d [%d]\n", j, i*4);
                // return r(new)
                ret[0] = 'r';
                ret[1] = j+'0';
                ret[2] = '\0';
            }
            break;

        case INT: // return INT
            strcpy(ret, root->lexeme);
            break;

        case ASSIGN: // return register
            strcpy(right, printAssemblyCode(root->right));

            // set left to 0
            setval(root->left->lexeme, 0, 0);
            i = findVariable(root->left);
            // change root from ASSIGN to ID(left)
            root->data = ID;
            strcpy(root->lexeme, root->left->lexeme);

            if(right[0] == 'r') { // right is not INT (unknown)
                if(root->right->data == ID) {
                    j = findVariable(root->right);
                    if(table[j].change) {
                        // change root's value
                        setval(root->lexeme, table[j].val, 1);
                    }
                }
                root->left = root->right = NULL;
                if(i <= 2) // MOV reg(left) reg(right)
                    printf("MOV r%d %s\n", i, right);
                else // MOV mem(left) reg(right)
                    printf("MOV [%d] %s\n", i*4, right);
                // return reg(right)
                strcpy(ret, right);
            }
            else { // right is INT (known)
                // change root's value
                setval(root->lexeme, atoi(right), 1);
                if(i <= 2) { // left is x, y, z
                    // MOV reg(left) reg(right)
                    printf("MOV r%d %s\n", i, right);
                    // return reg(left)
                    ret[0] = 'r';
                    ret[1] = i+'0';
                    ret[2] = '\0';
                }
                else  { // left is other variable
                    j = findRegister(root);
                    // MOV reg(new) INT(right)
                    printf("MOV r%d %s\n", j, right);
                    // MOV mem(left) reg(new)
                    printf("MOV [%d] r%d\n", i*4, j);
                    // return reg(new)
                    ret[0] = 'r';
                    ret[1] = j+'0';
                    ret[2] = '\0';
                }
            }
            break;

        case ADDSUB:
        case MULDIV:
        case LOGICAL:
            if(root->left->data==ID && root->right->data!=ID) { // right subtree may be big, so handle right subtree first
                strcpy(right, printAssemblyCode(root->right));
                strcpy(left, printAssemblyCode(root->left));
            }
            else {
                strcpy(left, printAssemblyCode(root->left));
                strcpy(right, printAssemblyCode(root->right));
            }

            /* INT OP INT */
            if(root->left->data==INT && root->right->data==INT) {
                // calculate INT OP INT
                INT_INT(root, left, right);
                // return INT
                strcpy(ret, root->lexeme);
            }
            /* INT OP reg */
            else if(root->left->data==INT) { // find a new register to store the result and release the right register
                i = findRegister(root);
                // MOV reg(new) INT(left)
                printf("MOV r%d %s\n", i, left);
                // OP reg(new) reg(right)
                printOP(root, i, right[1]-'0');
                // reg(right) = 1
                reg[right[1]-'0'] = 1;
                // return r(new)
                ret[0] = 'r';
                ret[1] = i+'0';
                ret[2] = '\0';
            }
            /* reg OP INT */
            else if(root->right->data==INT) {
                if (root->lexeme[0] == '/' && right[0]=='0') // check divide zero
                    err(DIVZERO);
                i = findRegister(root); // find a new register for right
                // MOV reg(new) INT(right)
                printf("MOV r%d %s\n", i, right);
                if(left[1]=='0' || left[1]=='1' || left[1]=='2') { // left is x, y, z: find a new register to store the result
                    j = findRegister(root);
                    // MOV reg(new) reg(left)
                    printf("MOV r%d %s\n", j, left);
                    // OP reg(new) reg(new)
                    printOP(root, j, i);
                    // return r(new)
                    ret[0] = 'r';
                    ret[1] = j+'0';
                    ret[2] = '\0';
                }
                else { // left is other variable
                    // OP reg(left) reg(new)
                    printOP(root, left[1]-'0', i);
                    // return r(left)
                    strcpy(ret, left);
                }
                // reg(new) = 1
                reg[i] = 1;
            }
            /* reg OP reg */
            else {
                if(left[1]=='0' || left[1]=='1' || left[1]=='2') { // left is x, y, z: find a new register to store the result
                    i = findRegister(root);
                    // MOV reg(new) reg(left)
                    printf("MOV r%d %s\n", i, left);
                    // OP reg(new) reg(right)
                    printOP(root, i, right[1]-'0');
                    // return r(new)
                    ret[0] = 'r';
                    ret[1] = i+'0';
                    ret[2] = '\0';
                }
                else { // left is other variable
                    // OP reg(left) reg(right)
                    printOP(root, left[1]-'0', right[1]-'0');
                    // return r(left)
                    strcpy(ret, left);
                }
                // reg(right) = 1
                reg[right[1]-'0'] = 1;
            }
            break;
        default:
            err(UNDEFINED);
    }
    return ret;
}

int main() {
    freopen("input.txt", "w", stdout);
    initTable();
    while (1) {
        statement();
    }
    return 0;
}
