#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <yaml.h>
#include "cJSON.h"  // Assuming this is for JSON creation, modify include paths as necessary.

static cJSON * yaml2json_node(yaml_document_t * document, yaml_node_t * node);

// Function to parse YAML from a string input
int yaml_parse_string(const char *input, yaml_document_t * document) {
    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char *)input, strlen(input));
    if (yaml_parser_load(&parser, document)) { return 0; }
    yaml_parser_delete(&parser);
    return -1;
}

cJSON * yaml2json(yaml_document_t * document) {
  yaml_node_t * node;
  if (node = yaml_document_get_root_node(document), !node) { return NULL; }
  return yaml2json_node(document, node);
}


char* yaml2json_string(char* content) {
  yaml_document_t document;
  cJSON *object;
  char *json;

  if (yaml_parse_string(content, &document)) {
      yaml_document_delete(&document);
      return NULL;
  }
  if ((object = yaml2json(&document)) == NULL) {
      yaml_document_delete(&document);
      return NULL;
  }

  // Clean up and output the JSON
  yaml_document_delete(&document);
  json = cJSON_Print(object);
  cJSON_Delete(object);
  return json;
}

cJSON * yaml2json_node(yaml_document_t * document, yaml_node_t * node) {
    yaml_node_t * key;
    yaml_node_t * value;
    yaml_node_item_t * item_i;
    yaml_node_pair_t * pair_i;
    double number;
    char * scalar;
    char * end;
    cJSON * object;

    switch (node->type) {
    case YAML_NO_NODE:
        object = cJSON_CreateObject();
        break;
    case YAML_SCALAR_NODE:
        scalar = (char *)node->data.scalar.value;
        number = strtod(scalar, &end);
        object = (end == scalar || *end) ? cJSON_CreateString(scalar) : cJSON_CreateNumber(number);
        break;
    case YAML_SEQUENCE_NODE:
        object = cJSON_CreateArray();
        for (item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; ++item_i) {
            cJSON_AddItemToArray(object, yaml2json_node(document, yaml_document_get_node(document, *item_i)));
        }
        break;
    case YAML_MAPPING_NODE:
        object = cJSON_CreateObject();
        for (pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; ++pair_i) {
            key   = yaml_document_get_node(document, pair_i->key);
            value = yaml_document_get_node(document, pair_i->value);
            if (key->type != YAML_SCALAR_NODE) { continue; }
            cJSON_AddItemToObject(object, (char *)key->data.scalar.value, yaml2json_node(document, value));
        }
        break;

    default:
        // mwarn("Unknown node type (line %lu).", node->start_mark.line);
        object = NULL;
    }

    return object;
}
