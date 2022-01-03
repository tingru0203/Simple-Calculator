# Introduction-to-programmingII_mini-project-1
Simple Calculator (output assembly code)
### Introduction

CPU: Eight 32-bit registers r0-r7. 256-byte memory.

**Input**: a list of expressions, consisting of:
* Integers
* Operators (+, -, *, /, =, &, |, ^, ++, --)
* Three initial variables x, y, z (each having an initial value)
* Some new variables

**Output**: A list of assembly code.

**Example**: 

input:
```
x=3+5
y=10
z=1
y = -z+1
z = x/4
x+y = z+100
```

output:
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
