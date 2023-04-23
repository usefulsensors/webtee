#include "settings.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_utils.h"
#include "yargs.h"


static void set_defaults(Settings* settings) {
  settings->host_name = NULL;
  settings->port = 9923;
  settings->protocol = "http";
  settings->use_external_ip = false;
}

Settings* settings_init_from_argv(int argc, char** argv) {

  Settings* settings = (Settings*)(calloc(1, sizeof(Settings)));
  set_defaults(settings);

  bool show_help = false;
  const YargsFlag flags[] = {
    YARGS_BOOL("help", "?", &show_help, "Displays usage information"),
    YARGS_STRING("host_name", "h", &settings->host_name, 
      "Domain name to use in the hosting URL. If not set the local machine's name is used."),
    YARGS_INT32("port", "p", &settings->port, 
      "Port number to use for hosting URL. If none, defaults to 9923."),
    YARGS_STRING("protocol", "r", &settings->protocol, 
      "Protocol to use for the hosting URL. Default is 'http'."),
    YARGS_BOOL("use_external_ip", "e", &settings->use_external_ip,
      "Use the `dig` command to attempt to find this machine's external IP for the hosting URL.")
  };
  const int flags_length = sizeof(flags) / sizeof(flags[0]);

  const char* app_description =
    "Speech recognition tool to convert audio to text transcripts.";
  const bool init_status = yargs_init(flags, flags_length,
    app_description, argv, argc);
  if (!init_status) {
    settings_free(settings);
    return NULL;
  }

  if (show_help) {
    yargs_print_usage(flags, flags_length, app_description);
    settings_free(settings);
    return NULL;
  }

  return settings;
}

void settings_free(Settings* settings) {
  yargs_free();
  if (settings == NULL) {
    return;
  }
  free(settings);
}
