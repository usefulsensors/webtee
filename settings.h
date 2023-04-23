#ifndef INCLUDE_SETTINGS_H
#define INCLUDE_SETTINGS_H

#include <stdbool.h>

#include "yargs.h"

#ifdef __CPLUSPLUS
extern "C" {
#endif // __CPLUSPLUS

typedef struct SettingsStruct {
  const char *host_name;
  int port;
  const char* protocol;
  bool use_external_ip;
} Settings;

Settings *settings_init_from_argv(int argc, char **argv);
void settings_free(Settings *settings);

#ifdef __CPLUSPLUS
}
#endif // __CPLUSPLUS

#endif // INCLUDE_SETTINGS_H