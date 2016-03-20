# Othello

French version in C with socket between two localhost software launched

## Compilation

```gcc -o othello_GUI othello_GUI.c $(pkg-config --cflags --libs gtk+-3.0)```
> en utilisateur normal

## Indentation

```indent --no-blank-lines-after-declarations --no-blank-lines-after-procedures --break-before-boolean-operator --braces-after-if-line --braces-on-struct-decl-line --brace-indent0 --dont-cuddle-else --continuation-indentation4 --case-indentation0  --format-first-column-comments --honour-newlines --indent-level4 --parameter-indentation4 --line-length80 --continue-at-parentheses --no-space-after-function-call-names --no-space-after-parentheses --dont-break-procedure-type --space-after-for --space-after-if --space-after-while --leave-optional-blank-lines --dont-space-special-semicolon --tab-size4 othello_GUI.c```