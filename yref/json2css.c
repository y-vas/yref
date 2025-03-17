#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cJSON.h"

bool has_wrapps(char* cnt){
  return (
    cnt[strlen(cnt) - 1] == '\'' && cnt[0] == '\''
  ) || (
    cnt[strlen(cnt) - 1] == '"' && cnt[0] == '"'
  );
}

void strip(char *str) {
    size_t len = strlen(str);

    if (len == 0) return;
    if (str[0] == '\'' || str[0] == '"') {
        memmove(str, str + 1, len);
        len--;
    }
    if (len > 0 && (str[len - 1] == '\'' || str[len - 1] == '"')) {
        str[len - 1] = '\0';
    }
}

void json2css(char *cnt , int max_size) {
		cJSON *json = cJSON_Parse(cnt);

    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return;
    }

		memset(cnt, 0, max_size );

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
			strcat(cnt, item->string);
			strcat(cnt, ":");

      if (cJSON_IsString(item)) {

				if (strstr(item->valuestring, "#") != NULL && has_wrapps(item->valuestring)) {
					strip(item->valuestring);
					strcat(cnt, item->valuestring);
				} else {
					strcat(cnt, item->valuestring);
				}

      } else if (cJSON_IsNumber(item)) {
				char str[50];
				snprintf(str, sizeof(str), "%f", item->valuedouble);
				strcat(cnt, str);
      } else if (cJSON_IsBool(item)) {
				strcat(cnt, cJSON_IsTrue(item) ? "true" : "false");
      } else {
				strcat(cnt, "none");
      }

			strcat(cnt, ";\n");
    }

    cJSON_Delete(json);
}
