# RPGLang's Expanded Backus-Naur Form

```
Grammar ::= FunctionDeclaration+, EOF

FunctionDeclaration ::= Type, Identifier, "(", ParameterList?, ")", Statement
ParameterList ::= Parameter, (";", Parameter)*
Parameter ::= NonVoidType, Identifier

Statement ::= StatementBlock
            | If
            | While
            | Until
            | Expression, ";"
            | VariableDeclaration, ";"
            | Assignment, ";"
            | Continue, ";"
            | Break, ";"
            | Return, ";"
            | ";"

StatementBlock ::= "{", Statement*, "}" 

If ::= "if", ConditionalBody, ("else", Statement)?
While ::= "while", ConditionalBody
Until ::= "until", ConditionalBody
ConditionalBody ::= "(", Expression, ")", Statement

Expression ::= And, ("or", And)*
And ::= Equality, ("and", Equality)*
Equality ::= Relation, (("worthy" | "duel!<>"), Relation)*
Relation ::= Shift, (("duel!<" | "duel!>"), Shift)*
Shift ::= Addition, (("push>" | "<push"), Addition)*
Addition ::= Term, (("unite" | "hit"), Term)*            (* "+" | "-" *)
Term ::= Unary, (("empower" | "shatter"), Unary)*        (* "*" | "/" *)
Unary ::= ("shadow" | "not")*, Primary
Primary ::= Number
          | Identifier
          | FunctionCall
          | "(", Expression, ")"

FunctionCall ::= Identifier, "(", ArgumentList?,")"
ArgumentList ::= Expression, (";", Expression)*

Number ::= "0", (RomanDigit)+

VariableDeclaration ::= NonVoidType, Identifier, AssignmentBody?
Assignment ::= Identifier, AssignmentBody
AssignmentBody ::= "mirror", Expression      (* = *)
Identifier ::= Character, (Character | "0")*
Type       ::= "void" | NonVoidType
NonVoidType ::= "prim" | "primordial" | "frac" | "fractured" | "loc" | "location"

Return ::= "complete", Expression?
Continue ::= "rollback"
Break ::= "skip"

Character ::= Letter
            | NonZeroDecimalDigit
            | SpecialCharacter
Letter ::= LowercaseLetter
         | UppercaseLetter
LowercaseLetter ::= [a-z]
UppercaseLetter ::= [A-Z]
NonZeroDecimalDigit ::= [1-9]

(* TODO: finish this list, it isn't exhaustive *)
SpecialCharacter ::= "+" | "-" | "*" | "/" | "_" | "."

RomanDigit ::= "I" | "V" | "X" | "L" | "C"
```
