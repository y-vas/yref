#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "entries.h"
#include "refer.h"

#include <Python.h>

PyObject* find_links(PyObject* self,PyObject* args){
  char *content;
  if (!PyArg_ParseTuple(args, "s", &content)) {
      return NULL;
  }

  PyObject *links = PyList_New(0);  // Create an empty list
  if (!links) {
      PyErr_SetString(PyExc_MemoryError, "Could not allocate PyList");
      return NULL;
  }

  if (!strstr(content, "<ref")) { return NULL; }

  char entries[ MAX_SIZE ];
  char tpoints[ MAX_SIZE ];

  memset(entries, 0, MAX_SIZE);
  memset(tpoints, 0, MAX_SIZE);

  tag_entries( content , entries, ","  , ";" , "ref" , true );
  _xml_tags(   content , entries, tpoints    ,   ";" );

  tpoints[sizeof(tpoints) - 1] = '\0';

  char *ptr = tpoints ;
  char *d_pos         ;



  while ((d_pos = strchr(ptr, ';')) != NULL) {
    int s,se,sb,sbe,nl,ns,ne,eb,eeb;

    sscanf(
      ptr   , "%d,%d,%d,%d,%d,%d,%d,%d,%d",
      &s,&se,&sb,&sbe,&nl,&ns,&ne,&eb,&eeb
    );

    // open tag
    char  open_tag[sbe-sb];
    strncpy(open_tag, content + sb, sbe - sb);
    open_tag[sbe-sb] = '\0';


    char tag_link [sbe-sb];
    char space[ SPACE_ALIAS_LENGTH    ];
         space[ SPACE_ALIAS_LENGTH - 1] = '\0';

    bool has_space = attr_val(open_tag, "space"  , space     );


    bool has_link  = attr_val(open_tag, "link"   , tag_link  );
    char *slash = strchr(tag_link, '/');
    if (slash != NULL) { tag_link[slash - tag_link] = '\0'; }

    if (has_link) {
      PyObject *d = PyDict_New();

      PyDict_SetItemString(d, "link", PyUnicode_FromString(tag_link));
      if (has_space) {
        PyDict_SetItemString(d, "space", PyUnicode_FromString(space));
      } else {
        PyDict_SetItemString(d, "space", Py_None );
        Py_INCREF(Py_None);
      }

      PyList_Append(links, d);

      Py_DECREF(d);
    }

    *d_pos = ';';
    ptr = d_pos + 1;
  }

  memset(entries, 0, sizeof(entries));
  memset(tpoints, 0, sizeof(tpoints));
  return links;
}
