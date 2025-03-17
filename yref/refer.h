#ifndef REFER_H
#define REFER_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_SIZE           65535
#define SPACE_NAME_LENGTH  26 //VARCHAR 26
#define USER_NAME_LENGTH   45 //VARCHAR 45
#define REF_NAME_LENGTH    45 //VARCHAR 45
#define SPACE_ALIAS_LENGTH (USER_NAME_LENGTH + SPACE_NAME_LENGTH + 1)

#define TYPE_NONE 0
#define TYPE_YAML 1
#define TYPE_JSON 2
#define TYPE_CSS  3


typedef struct RefLink {
    char alias  [ SPACE_ALIAS_LENGTH ];
    char name   [ REF_NAME_LENGTH    ];

    char content[ MAX_SIZE ];
    char render [ MAX_SIZE ];

    int  lang      ;
    bool is_source ;
    bool has_refs  ;
} RefLink;


typedef struct RefTag {
    char* start     ;
    char* full      ;
    int   spaces    ;
    char* link      ;
    char* space     ;
    char* path      ;
    bool  has_path  ;
    char* name      ;
    char* outerkey  ;
    char* expose    ;
    bool  item_list ;
    bool  end_slash ; // for link
    bool  has_ends  ;
} RefTag;

typedef struct RefSlot {
    char* name      ;
    char* content   ;
} RefSlot;

typedef struct RefBuffer {
    int  state  ;               // -1 = reached max size
    long length ;
    char render[MAX_SIZE];

    RefLink** links;            // Array of pointers to Link
    int       count;            // Number of links
} RefBuffer;

RefLink* _setup_link(RefBuffer* data, char* alias, char* name, int lang, char* content , bool is_source );
int interpret(RefBuffer* data, RefLink* link);

#endif
