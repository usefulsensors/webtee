#include <errno.h>
#include <limits.h>
#include <microhttpd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "qrcodegen/qrcodegen.h"
#include "settings.h"
#include "utils/string_utils.h"

#define READ_BUFFER_BYTE_COUNT (1024)

uint8_t* g_log_content_bytes = NULL;
size_t g_log_content_byte_count = 0;
pthread_mutex_t g_log_content_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    if (0 != strcmp(method, "GET")) {
        return MHD_NO; /* unexpected method */
    }
    if (&dummy != *ptr) {
        /* The first time only the headers are valid,
         do not respond in the first round... */
        *ptr = &dummy;
        return MHD_YES;
    }
    if (0 != *upload_data_size) {
        return MHD_NO; /* upload data in a GET!? */
    }
    *ptr = NULL; /* clear context pointer */

    pthread_mutex_lock(&g_log_content_mutex);
    struct MHD_Response * response = MHD_create_response_from_buffer(
    g_log_content_byte_count, g_log_content_bytes, MHD_RESPMEM_PERSISTENT);
    const int queue_status = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    pthread_mutex_unlock(&g_log_content_mutex);

    return queue_status;
}

// The result needs to be free()-ed after use.
static char* build_url(const char* protocol, const char* domain, const int port) {
    return string_alloc_sprintf("%s://%s:%d", protocol, domain, port);
}

static bool read_stdin() {
 
    uint8_t* read_buffer_bytes = malloc(READ_BUFFER_BYTE_COUNT);

    bool ended_normally = true;
    while (true) {
        // Watch stdin (fd 0) to see when it has input.
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);

        // Wait up to five seconds.
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        const int select_result = select(1, &rfds, NULL, NULL, &tv);

        if (select_result == -1) {
            ended_normally = false;
            break;
        } else if (select_result == 0) {
            // Timed out, try again later.
            continue;
        } else {
            const size_t bytes_read = read(STDIN_FILENO, read_buffer_bytes, READ_BUFFER_BYTE_COUNT);
            if (bytes_read < 0 && errno == EINTR) {
                continue;
            }
            if (bytes_read <= 0) {
                ended_normally = true;
                break;
            }

            pthread_mutex_lock(&g_log_content_mutex);

            const size_t new_log_contents_byte_count = (g_log_content_byte_count + bytes_read);
            uint8_t* new_log_content_bytes = realloc(g_log_content_bytes, new_log_contents_byte_count);
            uint8_t* old_log_content_bytes_end = (new_log_content_bytes + g_log_content_byte_count);
            // Append the new data to the end of the buffer.
            memcpy(old_log_content_bytes_end, read_buffer_bytes, bytes_read);

            g_log_content_bytes = new_log_content_bytes;
            g_log_content_byte_count = new_log_contents_byte_count;

            pthread_mutex_unlock(&g_log_content_mutex);

        }
    }
    free(read_buffer_bytes);

    return ended_normally;
}

int main(int argc, char ** argv) {
    Settings* settings = settings_init_from_argv(argc, argv);
    if (settings == NULL) {
        return 1;
    }

    const int port = settings->port;
    const char* protocol = settings->protocol;

    const char* host_name = settings->host_name;
    if (settings->host_name == NULL) {
        static struct utsname name_info;
        uname(&name_info);
        host_name = name_info.nodename;
    }

    const char* service_url = build_url(protocol, host_name, port);

    print_text_as_qr_to_terminal(service_url);
    fprintf(stderr, "%s\n", service_url);

    struct MHD_Daemon * d = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION,
        port,
        NULL,
        NULL,
        &ahc_echo,
        NULL,
        MHD_OPTION_END);
    if (d == NULL) {
        fprintf(stderr, "Failed to create microhttpd server for %s\n",
            service_url);
        return 1;
    }

    read_stdin();

    MHD_stop_daemon(d);

    settings_free(settings);
    return 0;
}
