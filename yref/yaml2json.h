#ifndef YAML2JSON_H
#define YAML2JSON_H

#include <yaml.h>
#include "cJSON.h"

int yaml_parse_string(const char *input, yaml_document_t * document);
cJSON * yaml2json(yaml_document_t * document);
char* yaml2json_string(char *content);

#endif
