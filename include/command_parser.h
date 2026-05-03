#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

char** parse_command(const char *command, int *argc);
void free_parsed_command(char **args);

#endif