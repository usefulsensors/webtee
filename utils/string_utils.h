#ifndef INCLUDE_UTIL_STRING_UTILS_H
#define INCLUDE_UTIL_STRING_UTILS_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __CPLUSPLUS
extern "C" {
#endif  // __CPLUSPLUS

  bool string_starts_with(const char* string, const char* start);
  bool string_ends_with(const char* string, const char* ending);

  // Returns a copy of the input. Caller owns and has to free() the result.
  char* string_duplicate(const char* string);

  // Allocates a buffer of the right size and then calls sprintf to write into
  // it, avoiding the risk of overwriting the end. Caller must free the result.
  char* string_alloc_sprintf(const char* format, ...);

  char* string_append(const char* a, const char* b);

  char* string_append_in_place(char* a, const char* b);

  // Splits a string into multiple parts, based on the single-character separator.
  // The `max_splits` arguments controls the maximum number of parts that will be
  // produced, or -1 for no maximum. The caller is responsible for calling free() 
  // on the `outputs` array, and for all of the entries in that array.
  void string_split(const char* input, char separator, const int max_splits,
    char*** outputs, int* outputs_length);

  // Convenience function to deallocate the memory allocated by the functions
  // that produce a list of strings, like string_split.
  void string_list_free(char** list, int list_length);

  // Appends a new string to the end of a list. A copy is made of the input
  // string, so the original can modified or freed independently of the list.
  void string_list_add(const char* new, char*** list, int* list_length);

  char* string_join(const char** list, int list_length, const char* separator);

  // Produces a new list that contains only the strings for which the callback
  // function returns true.
  typedef bool (*string_list_filter_funcptr)(const char* a, void* cookie);
  void string_list_filter(const char** in_list, int in_list_length,
    string_list_filter_funcptr should_keep_func, void* cookie, char*** out_list,
    int* out_list_length);

  // Takes a byte array of a given length, and returns a zero-terminated copy.
  char* string_length_to_null_terminated(const char* input, const size_t input_byte_count);

  // Returns the substring from character start_index, to end_index-1.
  char* string_from_range(const char* input, int start_index, int end_index);

  // Wrapper for integer parsing that returns true if value is understood.
  bool string_to_int32(const char* input, int32_t* output);

#ifdef __CPLUSPLUS
}
#endif  // __CPLUSPLUS

#endif  // INCLUDE_UTIL_STRING_UTILS_H
