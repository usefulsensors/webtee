#include <limits.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "qrcodegen/qrcodegen.h"

#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
             "</head><body>libmicrohttpd demo</body></html>"

#define MAX_URL_LENGTH (1024)

static enum MHD_Result
ahc_echo(void * cls,
         struct MHD_Connection * connection,
         const char * url,
         const char * method,
         const char * version,
         const char * upload_data,
         size_t * upload_data_size,
         void ** ptr) {
  static int dummy;
  const char * page = cls;
  struct MHD_Response * response;
  int ret;

  if (0 != strcmp(method, "GET"))
    return MHD_NO; /* unexpected method */
  if (&dummy != *ptr)
    {
      /* The first time only the headers are valid,
         do not respond in the first round... */
      *ptr = &dummy;
      return MHD_YES;
    }
  if (0 != *upload_data_size)
    return MHD_NO; /* upload data in a GET!? */
  *ptr = NULL; /* clear context pointer */
  response = MHD_create_response_from_buffer (strlen(page),
                                              (void*) page,
  					      MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response(connection,
			   MHD_HTTP_OK,
			   response);
  MHD_destroy_response(response);
  return ret;
}

// The result needs to be free()-ed after use.
static char* build_url(const char* protocol, const char* domain, const int port) {
    char* result = malloc(MAX_URL_LENGTH + 1);
    snprintf(result, MAX_URL_LENGTH, "%s://%s:%d", protocol, domain, port);
    // Just to be careful of non-standard snprintf() implementations.
    result[MAX_URL_LENGTH] = 0;
    return result;
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        printf("%s PORT\n",
        argv[0]);
        return 1;
    }

    const int port = atoi(argv[1]);
    const char* protocol = "http";

    struct utsname name_info;
    uname(&name_info);
    const char* host_name = name_info.nodename;

    const char* service_url = build_url(protocol, host_name, port);

    print_text_as_qr_to_terminal(service_url);

  struct MHD_Daemon * d;
  d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
		       port,
		       NULL,
		       NULL,
		       &ahc_echo,
		       PAGE,
		       MHD_OPTION_END);
  if (d == NULL)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon(d);
  return 0;
}