#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;        // 1-byte character (ASCII)
    if ((c & 0xE0) == 0xC0) return 2;  // 2-byte character
    if ((c & 0xF0) == 0xE0) return 3;  // 3-byte character
    if ((c & 0xF8) == 0xF0) return 4;  // 4-byte character
    return -1;  // Invalid UTF-8 character
}

// Function to find and print HTML tags with their positions
bool tag_entries(const char* content, char* entries , char* separator, char* separator2 , char* tag_filtered, bool binary) {

  // returns
  // 13,56,13,56,11;...
  // 0 = start |<
  // 1 = end   >|
  // 2 = start binary |<
  // 3 = end   binary >|
  // 4 = start of the line \n|whatever here <tag/> asdf


  int i      = 0;
  int length = strlen(content);
  int count  = 0;

  int raw_filter_length = strlen(tag_filtered);
  char filter1[raw_filter_length + 2];
  char filter2[raw_filter_length + 3];

  if (raw_filter_length != 0) {
    snprintf(filter1, sizeof(filter1), "<%s"  , tag_filtered);
    snprintf(filter2, sizeof(filter2), "</%s" , tag_filtered);
  } else {
    // Set the first character to '\0' to make it an empty string
    filter1[0] = '\0';
    filter2[0] = '\0';
  }

  // Construct the filter
  char utf8_char[12];  // Buffer to hold converted integers
  bool found = false;
  int  filter1_length = strlen(filter1);
  int  filter2_length = strlen(filter2);
  int  next_line = 0;

  while (i < length) {
    int char_len = utf8_char_len((unsigned char)content[i]);
    if (char_len == -1) { i++; continue; } // Skip the invalid byte

    char c = content[i];

    if (c == '\n') { next_line = count; }

    if (c != '<') {
      count++; i++; // Move to the next character if no '<' is found
      continue;
    }

    // printf("begin at index %d: %c\n", count, c);
    // We are now at the start of a tag '<'
    int tag_start = count; // Mark the start of the tag
    int tag_start_binary = i; // Mark the start of the tag

    // filter tags
    if (
      ( filter1_length != 0 && !(i + filter1_length > length) ) ||
      ( filter2_length != 0 && !(i + filter2_length > length) )
    ) {
      if (
          !(strncmp(content + i, filter1, filter1_length) == 0) &&
          !(strncmp(content + i, filter2, filter2_length) == 0)
      ) {
        count++; i++;
        continue;
      }
    }

    count++; i++; // Move past '<'

    //  handle nested tags
    int prev = 0;
    while (i < length) {
      int char2_len = utf8_char_len((unsigned char)content[i]);
      if (char2_len == -1) { i++; continue; } // Skip the invalid byte

      char c2 = content[i];

      // printf("Character at index %d: %c | %d\n", count, c2, prev);
      if (c2 == '>' && prev == 0) {

        sprintf(utf8_char, "%d", tag_start);
        strcat(entries, utf8_char); //add
        strcat(entries, separator);
        sprintf(utf8_char, "%d", count+1);
        strcat(entries, utf8_char); //add

        if (binary) {
          strcat(entries, separator);
          sprintf(utf8_char, "%d", tag_start_binary);
          strcat(entries, utf8_char); //add
          strcat(entries, separator);
          sprintf(utf8_char, "%d", i+1);
          strcat(entries, utf8_char); //add
        }

        strcat(entries, separator);
        sprintf(utf8_char, "%d", next_line);
        strcat(entries, utf8_char); //add

        strcat(entries, separator2);

        found = true;
        // printf("end at index %d: %c | %d\n", count, c2, prev);
        break;
      }
      if (c2 == '>' && prev != 0) {
        // prev = 4
        // pos [i] -> <im <asdf<xc<xy>>>[>]

        int y = i-1;
        int cnt = count - 1;
        int skip = 0;

        // printf("new ------- - y: %d, skip: %d, char:%c \n", y, skip,content[y]);
        while (y > 0) {
          int char3_len = utf8_char_len((unsigned char)content[y]);
          if (char3_len == -1) { y++; continue; } // Skip the invalid byte

          char c3 = content[y];

          // printf(" - y: %d, skip: %d, char:%c \n", y, skip,content[y]);
          if (c3 == '<' && skip == 0) { break; }
          if (c3 == '>') { skip++; }
          if (c3 == '<' && skip != 0) { skip--; }
          y--; cnt--;
        }

        // filter tags
        if (filter1_length != 0 || filter2_length != 0) {
          if (
            ( strncmp(content + y, filter1, filter1_length) != 0 ) &&
            ( strncmp(content + y, filter2, filter2_length) != 0 )
          ) {
            prev--;
            continue;
          }
        }

        found = true;
        sprintf(utf8_char, "%d", cnt);
        strcat(entries, utf8_char);
        strcat(entries, separator);
        sprintf(utf8_char, "%d", count+1);
        strcat(entries, utf8_char);

        if (binary) {
          strcat(entries, separator);
          sprintf(utf8_char, "%d", y);
          strcat(entries, utf8_char);
          strcat(entries, separator);
          sprintf(utf8_char, "%d", i+1);
          strcat(entries, utf8_char);
        }

        // next_line
        strcat(entries, separator);
        sprintf(utf8_char, "%d", cnt);
        strcat(entries, utf8_char);

        strcat(entries, separator2);

        prev--; // reset prevs
      }

      if (c2 == '<') { prev++; }
      count++; i++;
    }
  }

  return found;
}
