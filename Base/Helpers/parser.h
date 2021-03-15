#ifndef _PARSER_H
#define _PARSER_H
#include "../Structs/tokenlist.h"

tokenlist *new_tokenlist();
void add_token(tokenlist *tokens, char *item);
char *get_input();
tokenlist *get_tokens(char *input);
void free_tokens(tokenlist *tokens);

#endif