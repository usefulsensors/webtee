#include "settings.h"

#include "acutest.h"

#include "settings.c"

#include <stdlib.h>


void test_set_defaults() {
  Settings settings = {};
  set_defaults(&settings);
  TEST_CHECK(settings.host_name == NULL);
  TEST_INTEQ(settings.port, 9923);
  TEST_STREQ(settings.protocol, "http");
}

void test_settings_init_from_argv() {

  char* argv1[] = { "program" };
  const int argc1 = sizeof(argv1) / sizeof(argv1[0]);
  Settings* settings1 = settings_init_from_argv(argc1, argv1);
  TEST_CHECK(settings1 != NULL);
  settings_free(settings1);

  char* argv2[] = { "program", "--unknown_flag" };
  const int argc2 = sizeof(argv2) / sizeof(argv2[0]);
  Settings* settings2 = settings_init_from_argv(argc2, argv2);
  TEST_CHECK(settings2 == NULL);

  char* argv3[] = { "program",
    "--host_name", "foo",
    "--port", "2377",
    "--protocol", "https",
    "--use_external_ip",
  };
  const int argc3 = sizeof(argv3) / sizeof(argv3[0]);
  Settings* settings3 = settings_init_from_argv(argc3, argv3);
  TEST_CHECK(settings3 != NULL);
  TEST_STREQ(settings3->host_name, "foo");
  TEST_INTEQ(settings3->port, 2377);
  TEST_STREQ(settings3->protocol, "https");
  TEST_CHECK(settings3->use_external_ip);
  settings_free(settings3);

  char* argv4[] = { "program",
    "-h", "foo",
    "-p", "2377",
    "-r", "https",
    "-e",
  };
  const int argc4 = sizeof(argv4) / sizeof(argv4[0]);
  Settings* settings4 = settings_init_from_argv(argc4, argv4);
  TEST_CHECK(settings4 != NULL);
  TEST_STREQ(settings4->host_name, "foo");
  TEST_INTEQ(settings4->port, 2377);
  TEST_STREQ(settings4->protocol, "https");
  TEST_CHECK(settings3->use_external_ip);
  settings_free(settings4);
}

TEST_LIST = {
  {"test_set_defaults", test_set_defaults},
  {"test_settings_init_from_argv", test_settings_init_from_argv},
  {NULL, NULL},
};