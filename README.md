# Introduction-to-programmingII_mini-project-1
Simple Calculator (output assembly code)
## Spec

CPU: Eight 32-bit registers r0-r7. 256-byte memory.

**Input**: a list of expressions, consisting of:
* Integers
* Operators (+, -, *, /, =, &, |, ^, ++, --)
* Three initial variables x, y, z (each having an initial value)
* Some new variables

**Grammar**: parse the input according to the grammar below
```
statement  :=  ENDFILE | END | expr END
expr       :=  term expr_tail
expr_tail  :=  ADDSUB_LOGICAL term expr_tail | NiL
term       :=  factor term_tail
term_tail  :=  MULDIV factor term_tail | NiL
factor     :=  INT | ADDSUB INT |
               ID  | ADDSUB ID  |
               INCDEC ID | ID ASSIGN expr |
               LPAREN expr RPAREN |
               ADDSUB LPAREN expr RPAREN
```

**Output**: A list of assembly code.
* Store the answer of the variables x, y, z in registers r0, r1, and r2 respectively.
* If the expression is legal, print "EXIT 0" on the last line.
* If the expression is illegal, the final output should be "EXIT 1".

Sample input:
```
x=3+5
y=10
z=1
y = -z+1
z = x/4
x+y = z+100
```

Sample output:
```
MOV r0 [0]
MOV r1 [4]
MOV r2 [8]
MOV r0 8
MOV r1 10
MOV r2 1
MOV r1 0
MOV r2 2
MOV r1 102
EXIT 0
```

## Implementation
#### Lexical analyzer

#### Parser

#### Code generator
