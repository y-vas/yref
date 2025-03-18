#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "refer.h"
#include "entries.h"

#include "yaml2json.h"
#include "json2yaml.h"
#include "json2css.h"
#include "cJSON.h"


bool is_json_list(char* cnt){
  return cnt[strlen(cnt) - 1] == ']' && cnt[0] == '[';
}
bool is_json_dict(char* cnt){
  return cnt[strlen(cnt) - 1] == '}' && cnt[0] == '{';
}

bool is_json(char* cnt){
  return is_json_dict(cnt) || is_json_list(cnt);
}

bool is_number(char *str) {
    if (*str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}


char *str_replace(char *orig, char *rep, char *with) {
    char *result;
    char *ins;
    char *tmp;
    int len_rep;
    int len_with;
    int len_front;
    int count;

    if (!orig || !rep)
        return NULL;

    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // Avoid infinite loop if 'rep' is an empty string

    if (!with)
        with = "";

    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}


bool attr_val(char* input, char* key, char* output) {
  char *key_start = strstr(input, key); // Find the key in the input string

  if (key_start) {
      char *equal_sign = strchr(key_start, '='); // Find the '=' after the key

      if (equal_sign) {
          // Find the starting quote, can be either single or double
          char *quote_start = strchr(equal_sign, '\'');
          if (!quote_start) {
              quote_start = strchr(equal_sign, '\"');
          }

          if (quote_start) {
              char *quote_end = quote_start + 1; // Start searching for the end of the quote
              bool escape = false; // To track escaped characters

              // Traverse until we find the matching closing quote
              while (*quote_end) {
                  if (*quote_end == '\\' && !escape) {
                      escape = true; // Next character will be escaped
                  } else if ((*quote_end == '\'' || *quote_end == '\"') && !escape) {
                      break;
                  } else {
                      escape = false; // Reset escape if not an escape character
                  }
                  quote_end++;
              }

              if (quote_end) {
                  size_t length = quote_end - quote_start - 1; // Calculate the length of the value
                  strncpy(output, quote_start + 1, length); // Copy the value between the quotes
                  output[length] = '\0'; // Null-terminate
                  return true; // found
              }
          }
      }

      char utf8_char[12];
      sprintf(utf8_char, "%d", 1);
      strcat(output, utf8_char);
      return true;
  }

  return false;
}

char* _get_content_from_json(char* content,char* path) {
    if (content == NULL) {
      return NULL;
    }

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        return NULL;
    }

    const char *token = path;
    cJSON *current = root;

    while (token != NULL && *token != '\0') {
        const char *next_token = strchr(token, '/');
        size_t token_length = (next_token != NULL) ? (size_t)(next_token - token) : strlen(token);

        char key[token_length + 1];
        strncpy(key, token, token_length);
        key[token_length] = '\0';  // Null-terminate the string

        if (isdigit(key[0])) {
            int index = atoi(key);
            if (!cJSON_IsArray(current)) {
                cJSON_Delete(root);
                return NULL;
            }
            current = cJSON_GetArrayItem(current, index);
            if (current == NULL) {
                cJSON_Delete(root);
                return NULL;
            }
        } else {
            if (!cJSON_IsObject(current)) {
                cJSON_Delete(root);
                return NULL;
            }
            current = cJSON_GetObjectItemCaseSensitive(current, key);
            if (current == NULL) {
                cJSON_Delete(root);
                return NULL;
            }
        }

        // Move to the next part of the path
        token = (next_token != NULL) ? next_token + 1 : NULL;
    }

    if (cJSON_IsString(current)) {
        const char *result = cJSON_GetStringValue(current);
        if (result) {
            char *final_result = strdup(result);
            cJSON_Delete(root);  // Clean up JSON object before returning
            return final_result;
        }
    } else if (cJSON_IsNumber(current)) {
        char *final_result = malloc(20); // Enough for typical int/float
        snprintf(final_result, 20, "%g", current->valuedouble);
        cJSON_Delete(root);
        return final_result;
    } else if (cJSON_IsObject(current) || cJSON_IsArray(current)) {
        char *json_result = cJSON_PrintUnformatted(current);  // Allocates memory for the result
        if (json_result) {
            cJSON_Delete(root);
            return json_result;  // Must be freed by the caller
        }
    }

    cJSON_Delete(root);
    return NULL;
}


void _tag_name_pos(char* content,int start, int end, int *nstart, int *nstart_ends){
  int  i = start + 1;
  *nstart = start + 1;

  while (i < end) {
    char c = content[i];

    if (c == '/') { // for </tag >
      i++; continue;
    }

    if (
        isspace(c) ||
        c == '\''  ||
        c == '='   ||
        c == '<'   ||
        c == '>'   ||
        c == '\"'
      ) {
      *nstart_ends = i;
      break;
    }

    i++;
  }
}


void _xml_tags(char* content,char* entries, char* output,char* separator){
  char utf8_char[12];
  char *ptr = entries;  // Pointer to traverse the string
  char *delimiter_pos;

  while ((delimiter_pos = strchr(ptr, ';')) != NULL) {
    *delimiter_pos = '\0';  // Temporarily null-terminate the token

    int start, start_ends,start_binary,start_ends_binary ,nl;
    sscanf(ptr, "%d,%d,%d,%d,%d", &start, &start_ends,&start_binary,&start_ends_binary, &nl);

    if ( // skip non valid [ ender value </name> ]
        (start_ends_binary - start_binary) <= 3  ||
        content[start_binary + 1] == '/' ||
        content[start_binary + 2] == '>'
    ) {
      *delimiter_pos = ';';
      ptr = delimiter_pos + 1;
      continue;
    }


    int nbstart      = start_binary;
    int nbstart_ends = start_ends_binary;

    _tag_name_pos(
        content,
        start_binary,
        start_ends_binary,
        &nbstart,
        &nbstart_ends
    );

    if ( // is simple
      content[start_ends_binary - 1] == '>' &&
      content[start_ends_binary - 2] == '/'
    ) {

      strcat(output, ptr);
      strcat(output, ",");

      // name entries
      sprintf(utf8_char, "%d", nbstart); strcat(output, utf8_char);
      strcat(output, ",");
      sprintf(utf8_char, "%d", nbstart_ends); strcat(output, utf8_char);
      strcat(output, ",");

      // colse tag not defined
      sprintf(utf8_char, "%d", -1); strcat(output, utf8_char);
      strcat(output, ",");
      sprintf(utf8_char, "%d", -1); strcat(output, utf8_char);
      strcat(output, separator );

      ptr = delimiter_pos + 1;
      continue;
    }

    char *last_delimiter_pos = delimiter_pos;
    char *next_ptr           = last_delimiter_pos + 1;  // Pointer to the next token
    char *next_delimiter_pos;


    int  count = 0;
    bool has_ends = false;

    while ((next_delimiter_pos = strchr(next_ptr, ';')) != NULL) {
        *next_delimiter_pos = '\0';

        int close, close_ends,close_binary,close_ends_binary;
        sscanf(next_ptr, "%d,%d,%d,%d", &close, &close_ends,&close_binary,&close_ends_binary);


        // skip non valid
        if (
          (close_ends_binary - close_binary) <= 3 ||
          (
            content[close_ends_binary - 1] == '>' &&
            content[close_ends_binary - 2] == '/'
          )
        ) {
          *next_delimiter_pos = ';';
          next_ptr = next_delimiter_pos + 1;
          continue;
        }


        int nbclose      = close_binary;
        int nbclose_ends = close_ends_binary;

        _tag_name_pos(
            content,
            close_binary,
            close_ends_binary,
            &nbclose,
            &nbclose_ends
        );

        nbclose++;
        int start_len = nbstart_ends - nbstart;
        int close_len = nbclose_ends - nbclose;

        // printf("---------------------------------------------------------------- \n");
        // printf("[%d] == [%d] \n",start_len,close_len);
        // printf("[%s] == [%s] \n",content + nbstart,content + nbclose);
        // printf("same %d \n",strncmp(content + nbstart, content + nbclose , start_len) == 0);
        // printf("---------------------------------------------------------------- \n");
        if (!(
          start_len == close_len && strncmp(content + nbstart, content + nbclose , start_len) == 0
        )) {
          *next_delimiter_pos = ';';
          next_ptr = next_delimiter_pos + 1;
          continue;
        }

        // printf("is ender \n");

        bool isender = (
          content[close_binary    ] == '<' &&
          content[close_binary + 1] == '/'
        );

        if (!isender) { count++; }
        if (isender && count != 0) { count--; }

        if (isender && count == 0) {
            has_ends = true;

            strcat(output, ptr);
            strcat(output, ",");

            // name entries
            sprintf(utf8_char, "%d", nbstart); strcat(output, utf8_char);
            strcat(output, ",");
            sprintf(utf8_char, "%d", nbstart_ends); strcat(output, utf8_char);
            strcat(output, ",");

            // colse tag
            sprintf(utf8_char, "%d", close_binary); strcat(output, utf8_char);
            strcat(output, ",");
            sprintf(utf8_char, "%d", close_ends_binary); strcat(output, utf8_char);

            strcat(output,separator);

            *next_delimiter_pos = ';';
            next_ptr = next_delimiter_pos + 1;
            break;
        }


        *next_delimiter_pos = ';';
        next_ptr = next_delimiter_pos + 1;
    }

    if (!has_ends) {
      // <ref ...>
      // printf("s | %s ", substring);
      // start tag

      strcat(output, ptr);
      strcat(output, ",");

      // name entries //
      sprintf(utf8_char, "%d", nbstart); strcat(output, utf8_char);
      strcat(output, ",");
      sprintf(utf8_char, "%d", nbstart_ends); strcat(output, utf8_char);
      strcat(output, ",");

      // colse tag not defined //
      sprintf(utf8_char, "%d", -1); strcat(output, utf8_char);
      strcat(output, ",");
      sprintf(utf8_char, "%d", -1); strcat(output, utf8_char);
      strcat(output, separator );
    }

    // Restore the original delimiter position
    *last_delimiter_pos = ';';
    ptr = last_delimiter_pos + 1;
  }
}



RefLink* _setup_link(RefBuffer* data, char* alias, char* name, int lang, char* content , bool is_source ){
  struct RefLink* link = malloc(sizeof( struct RefLink ));

  // store for later use
  strncpy(link->alias, alias, SPACE_ALIAS_LENGTH - 1);
  link->alias[SPACE_ALIAS_LENGTH - 1] = '\0';
  strncpy(link->name, name, USER_NAME_LENGTH     - 1);
  link->name[USER_NAME_LENGTH    - 1] = '\0';
  strncpy(link->content, content, MAX_SIZE       - 1);
  link->content[MAX_SIZE         - 1] = '\0';
  strncpy(link->render , content, MAX_SIZE       - 1);
  link->render[ MAX_SIZE         - 1] = '\0';


  link->lang      = lang      ;
  link->is_source = is_source ;
  link->has_refs  = strstr(content, "<ref");

  // append
  RefLink** nLinks = (RefLink**)realloc(data->links, (data->count + 1) * sizeof(RefLink*));

  data->links = nLinks;
  data->links[data->count++] = link;

  return link;
}


bool _extract_content(
  char    * from,
  RefTag  * tag ,
  RefLink * ref ,
  RefLink * origin
){
  char* result = _get_content_from_json(from, tag->path);
  if (result == NULL) { return true; }

  memset( from, 0, MAX_SIZE );

  // if "r/r3/" => {"r3":"test"}
  // if "r/r3" => test
  bool _is_json = is_json(result);

  if (tag->end_slash) {

    if((
      origin->lang == TYPE_YAML ||
      ref->lang    == TYPE_YAML
      ) && !_is_json
    ){

      if ( tag->item_list) {
        strcat( from , "- " );
      }

      strcat( from , result );

     return false;
    }

    strcat( from , "{\"" );
    strcat( from , tag->outerkey   );
    strcat( from , "\":" );

    if (_is_json) {
      strcat( from , result );
    } else {
      strcat( from , "\"" );
      strcat( from , result );
      strcat( from , "\"" );
    }

    strcat( from , "}" );
  } else {
    strcat( from , result );
  }

  // if result is json tyle and needs to be transformed into yaml
  if( (
    origin->lang == TYPE_YAML ||
    ref->lang == TYPE_YAML
  ) && is_json(from) ){
    char *out = json2yaml( from );
    memset( from, 0, strlen(from));
    strcat( from , out );
    from[strlen(from) - 1] = '\0';
  }


  return false;
}

void _add_spaces(char *econtent, int spaces) {
    char temp[ MAX_SIZE ];
    size_t temp_len = 0;

    for (size_t i = 0; i < strlen(econtent); i++) {
        char c = econtent[i];

        // Append character safely
        if (temp_len < MAX_SIZE - 1) {
            temp[temp_len++] = c;
        }

        // If newline, add spaces
        if (c == '\n') {
            for (int j = 0; j < spaces && temp_len < MAX_SIZE - 1; j++) {
                temp[temp_len++] = ' ';
            }
        }
    }

    temp[temp_len] = '\0';

    strncpy(econtent , temp, MAX_SIZE       - 1);
    econtent[ MAX_SIZE         - 1] = '\0';
    memset( temp, 0, sizeof(temp)); // reset
}


bool _append_data(
  RefLink  * origin,
  RefTag   * tag,
  char     * extracted
){
  int  offset = 0;
  char *pos   = origin->render;
  char *found;

  char temp[ MAX_SIZE ];


  while ((found = strstr(pos, tag->start)) != NULL) {
      int before_match = found - pos;
      if (offset + before_match + strlen(extracted) >= MAX_SIZE - 1) {

        memset( temp, 0, sizeof(temp));
        return true; // data too big //
      }

      strncpy(temp + offset, pos, before_match);
      offset += before_match;
      strcpy(temp + offset, extracted);
      offset += strlen(extracted);
      pos = found + strlen(tag->full);
  }

  strcpy(temp + offset, pos); // Copy remaining part of content


  strncpy(origin->render , temp, MAX_SIZE       - 1);
  origin->render[ MAX_SIZE         - 1] = '\0';

  memset( temp, 0, sizeof(temp));
  return false;
}

int _interpret_slots(
  RefBuffer * data,
  RefLink   * link,
  RefTag    * tag,
  char      * part,
  char      * extracted
){
    int s,se,sb,sbe,nl,ns,ne,eb,eeb;
    sscanf(
      part     , "%d,%d,%d,%d,%d,%d,%d,%d,%d",
      &s,&se,&sb,&sbe,&nl,&ns,&ne,&eb,&eeb
    );

    // open tag
    char  open_tag[sbe-sb];
    strncpy(open_tag, tag->full + sb, sbe - sb);
    open_tag[sbe-sb] = '\0';

    // init fill_tag size
    bool has_ends = eb != -1;
    int  ft_size  = has_ends ? eeb - sb : sbe - sb;
    int  ft_end   = has_ends ? eeb : sbe;
    char full_tag [ft_size];


    strncpy(full_tag, tag->full + sb, ft_end - sb);
    full_tag[ft_size] = '\0';

    char name [sbe-sb];
    bool has_name = attr_val(open_tag, "name"  , name     );


    struct RefSlot* slot = malloc(sizeof(struct RefSlot));

    slot->name = has_name ? name : "default";

    // get inner values
    char slot_entries[ MAX_SIZE ];
    char slot_tpoints[ MAX_SIZE ];

    tag_entries( extracted , slot_entries, ","       , ";" , "slot" , true );
    _xml_tags(    extracted , slot_entries, slot_tpoints    ,   ";"  );
    slot_tpoints[sizeof(slot_tpoints) - 1] = '\0';

    char *ptr_slots = slot_tpoints ;
    char *d_pos_slots              ;

    while ((d_pos_slots = strchr(ptr_slots, ';')) != NULL) {
      int s2,se2,sb2,sbe2,nl2,ns2,ne2,eb2,eeb2;
      sscanf(
        ptr_slots     , "%d,%d,%d,%d,%d,%d,%d,%d,%d",
        &s2,&se2,&sb2,&sbe2,&nl2,&ns2,&ne2,&eb2,&eeb2
      );

      // open tag
      char  slot_tag[sbe2-sb2];
      strncpy(slot_tag, extracted + sb2, sbe2 - sb2);
      slot_tag[sbe2-sb2] = '\0';

      // init fill_tag size
      int  ft_size2 = eb2 != -1 ? eeb2 - sb2 : sbe2 - sb2;
      int  ft_end2  = eb2 != -1 ? eeb2 : sbe2;
      char slot_full[ft_size2];


      strncpy(slot_full, extracted + sb2, ft_end2 - sb2 );
      slot_full[ft_size2] = '\0';


      char name2 [sbe2-sb2];
      bool has_name2 = attr_val(slot_tag, "name"  , name2 );



      if ( strcmp(has_name2 ? name2 : "default" , slot->name ) == 0 ) {

        if (!has_ends) {
          size_t len  = strlen(slot_full);
          char* start = extracted;
          while ((start = strstr(start, slot_full)) != NULL) {
              char* end = start + len;
              memmove(start, end, strlen(end) + 1);
          }
          break;
        }

        char content[eb - sbe];
        strncpy(content, tag->full + sbe, eb - sbe);
        content[eb - sbe] = '\0';

        char* res = str_replace( extracted, slot_full, content );

        strncpy(extracted , res, MAX_SIZE       - 1);
        extracted[ MAX_SIZE         - 1] = '\0';

        free(res);
      }

      *d_pos_slots = ';';
      ptr_slots = d_pos_slots + 1;
    }

    memset(slot_entries, 0, sizeof(slot_entries));
    memset(slot_tpoints, 0, sizeof(slot_tpoints));


    free(slot);
    return 0;
}


bool _replace_ref(
  RefBuffer* data,
  RefLink  * origin,
  RefLink  * ref,
  RefTag   * tag
){
  bool has_errors = false;
  char extracted[ MAX_SIZE ];
  bool is_transformed = false;

  if( ref->lang == TYPE_YAML ){ // if yaml type
    char* json = yaml2json_string( ref->render );

    if ( json == NULL ){
      json = yaml2json_string( ref->content );
    }

    is_transformed = true;
    if ( json == NULL ){

      strncpy(extracted , "ERROR PARSING YAML : " , MAX_SIZE       - 1);

      strcat( extracted , ref->alias         );
      strcat( extracted , " -> "             );
      strcat( extracted , ref->name          );

      extracted[ MAX_SIZE         - 1] = '\0';
      has_errors = true;
    } else {
      strncpy(extracted ,  json , MAX_SIZE       - 1);
      extracted[ MAX_SIZE         - 1] = '\0';
    }
  }

  if (
      (ref->lang == TYPE_JSON || ref->lang == TYPE_YAML) &&
      (tag->has_path || tag->end_slash) && !has_errors
  ) {
    bool error = _extract_content( extracted, tag, ref , origin );
    is_transformed = true;

    if (error) {
      strncpy(extracted , "ERROR IN PATH : " , MAX_SIZE - 1);

      strcat( extracted , ref->alias );
      strcat( extracted , " -> "     );
      strcat( extracted , ref->name  );
      strcat( extracted , " -> "     );
      strcat( extracted , tag->link  );

      extracted[ MAX_SIZE         - 1] = '\0';
      has_errors = true;
    }
  }

  if (
      !tag->item_list && (
        origin->lang == TYPE_CSS ||
        strncmp(tag->expose ,"css" , 3) == 0
      ) && is_json_dict(extracted)
  ) {
    json2css(extracted,MAX_SIZE);
  }


  if (!is_transformed) {
    strncpy(extracted , ref->render , MAX_SIZE       - 1);
    extracted[ MAX_SIZE         - 1] = '\0';
  }


  // interpret slots
  if (tag->has_ends) {
    char slot_entries[ MAX_SIZE ];
    char slot_tpoints[ MAX_SIZE ];

    tag_entries( tag->full, slot_entries, ","       , ";" , "slot" , true );
    _xml_tags(    tag->full, slot_entries, slot_tpoints    ,   ";" );
    slot_tpoints[sizeof(slot_tpoints) - 1] = '\0';

    char *ptr_slots = slot_tpoints ;
    char *d_pos_slots              ;

    while ((d_pos_slots = strchr(ptr_slots, ';')) != NULL) {

      if (_interpret_slots(data,origin,tag,ptr_slots,extracted) != 0 ) {
        data->state = -1;
        return 1;
      }

      *d_pos_slots = ';';
      ptr_slots = d_pos_slots + 1;
    }

    memset(slot_entries, 0, sizeof(slot_entries));
    memset(slot_tpoints, 0, sizeof(slot_tpoints));
  }
  // end of slot interpretation


  if (tag->spaces != 0 && !has_errors) {
    _add_spaces(extracted,tag->spaces);
  }

  bool added_error = _append_data(origin,tag,extracted);

  if (added_error) {
    memset( extracted, 0, sizeof(extracted));
    return true;
  }


  memset( extracted, 0, sizeof(extracted));
  return false;
}

int _interpret_refs(RefBuffer* data, RefLink* link, RefTag* tag, char* part){
    int s,se,sb,sbe,nl,ns,ne,eb,eeb;

    sscanf(
      part     , "%d,%d,%d,%d,%d,%d,%d,%d,%d",
      &s,&se,&sb,&sbe,&nl,&ns,&ne,&eb,&eeb
    );

    // open tag
    char  open_tag[sbe-sb];
    strncpy(open_tag, link->content + sb, sbe - sb);
    open_tag[sbe-sb] = '\0';

    // init fill_tag size
    int  ft_size = eb != -1 ? eeb - sb : sbe - sb;
    int  ft_end  = eb != -1 ? eeb : sbe;
    char full_tag[ft_size];
    strncpy(full_tag, link->content + sb, ft_end - sb);
    full_tag[ft_size] = '\0';

    // spaces to add
    int inline_spaces = (
      strstr(open_tag, "ignore-spaces" ) ||
      strstr(open_tag, "spaceless"     )
    ) ? 0 : s - nl - 1;

    // extract arguments
    char tag_link [sbe-sb];
    char expose   [sbe-sb];

    char space[ SPACE_ALIAS_LENGTH    ];
         space[ SPACE_ALIAS_LENGTH - 1] = '\0';


    bool has_space = attr_val(open_tag, "space"  , space     );
    bool has_link  = attr_val(open_tag, "link"   , tag_link  );
                     attr_val(open_tag, "expose" , expose    );



    // if there is no space setup in tag it means
    // it's placed in the same space as current link
    if (!has_space ) {
      strcpy(space, link->alias);
    }

    // a tag without link needs to be removed
    if (!has_link) {
      size_t len  = strlen(full_tag);
      char* start = link->render;

      while ((start = strstr(start, full_tag)) != NULL) {
          char* end = start + len;
          memmove(start, end, strlen(end) + 1);
      }

      link->render[ strlen(link->render)         - 1] = '\0';
      return 0;
    }


    // fragment in path
    char json_path[sbe-sb];
    char *slash = strchr(tag_link, '/');

    if (slash != NULL) {
      int first_length = slash - tag_link;
      tag_link[first_length] = '\0';
      strcpy(json_path, slash + 1);
    } else {
      json_path[0] = '\0';
    }

    // get last path key
    bool has_path = json_path[0] != '\0';
    char outerkey[sbe-sb];

    if (has_path) {
      size_t len = strlen(json_path);
      const char *end = json_path + len - 1;

      while (end > json_path && *end == '/') { end--;   }
      const char *last_slash = end;
      while (last_slash > json_path && *last_slash != '/') { last_slash--; }

      if (*last_slash == '/') { last_slash++; }

      size_t segment_length = end - last_slash + 1;
      if (segment_length < sizeof(outerkey)) {
        strncpy(outerkey, last_slash, segment_length);
        outerkey[segment_length] = '\0';
      }
    }

    tag->start     = open_tag;
    tag->full      = full_tag;
    tag->spaces    = inline_spaces ;
    tag->link      = tag_link;
    tag->path      = json_path ;
    tag->has_path  = has_path;
    tag->end_slash = has_path ? json_path[strlen(json_path) -1] == '/' : false;
    tag->expose    = expose;
    tag->outerkey  = outerkey;
    tag->item_list = is_number( outerkey );
    tag->has_ends  = eb != -1;


    // update content
    bool found = false;
    for (int j = 0; j < data->count; j++) {
      if (data->state == -1) { return 1; }

      RefLink* ref = data->links[j];
      //
      // printf("%s \n", tag->full );
      // printf("[%s]=[%s]=>%d\n", ref->alias ,space , strncmp(space    , ref->alias , strlen(ref->alias )) == 0);
      // printf("[%s]=[%s]=>%d\n", ref->name ,tag_link , strncmp(tag_link    , ref->name , strlen(ref->name )) == 0);

      if (!(
        (
          has_space ? (strncmp( space , ref->alias , strlen(ref->alias )) == 0) : true
        ) &&
        strncmp(tag_link , ref->name  , strlen(ref->name  )) == 0
      )) {
        continue;
      }

      found      = true;
      tag->name  = ref->name;
      if (!has_path) {
        tag->outerkey  = ref->name;
        tag->item_list = false;
      }

      bool error = _replace_ref( data, link, ref , tag );

      if (error) {
        return 1;
      }

      break; // no need to search more if found
    }

    if (!found) { // if not found remove tag
      size_t len  = strlen(full_tag);
      char* start = link->render;
      while ((start = strstr(start, full_tag)) != NULL) {
          char* end = start + len;
          memmove(start, end, strlen(end) + 1);
      }
    }

    return 0;
}



int interpret(RefBuffer* data, RefLink* link){
  if (!link->has_refs) { return 0; }

  char entries[ MAX_SIZE ];
  char tpoints[ MAX_SIZE ];
  memset(entries, 0, MAX_SIZE);
  memset(tpoints, 0, MAX_SIZE);


  tag_entries( link->content, entries, ","  , ";" , "ref" , true );
  _xml_tags(    link->content, entries, tpoints    ,   ";" );

  tpoints[sizeof(tpoints) - 1] = '\0';

  char *ptr = tpoints ;
  char *d_pos         ;

  int  error = 0;
  while ((d_pos = strchr(ptr, ';')) != NULL) {
    if (data->state == -1) {
      error = 1;
      break;
    }

    struct RefTag* tag = malloc(sizeof(struct RefTag));

    if (_interpret_refs(data,link,tag,ptr) != 0 ) {
      error = 1;
      data->state = -1;
      free(tag);
      break;
    }

    free(tag);

    *d_pos = ';';
    ptr = d_pos + 1;
  }

  memset(entries, 0, sizeof(entries));
  memset(tpoints, 0, sizeof(tpoints));
  return error;
}
