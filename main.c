#include <errno.h>
#include <limits.h>
#include <microhttpd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "file_utils.h"
#include "qrcodegen/qrcodegen.h"
#include "settings.h"
#include "string_utils.h"
#include "trace.h"

#define READ_BUFFER_BYTE_COUNT (1024)

uint8_t* g_log_content_bytes = NULL;
size_t g_log_content_byte_count = 0;
pthread_mutex_t g_log_content_mutex = PTHREAD_MUTEX_INITIALIZER;

static int
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

// The result needs to be free()-ed after use.
static char* get_command_line(const char* process_id) {
    char* process_info_dir = string_alloc_sprintf("/proc/%s", process_id);
    char* command_line_path = string_alloc_sprintf("%s/cmdline", process_info_dir);
    TRACE_STR(command_line_path);
    char* current_command_line_bytes;
    size_t current_command_line_byte_count;
    const bool command_line_status = file_read(command_line_path, &current_command_line_bytes, &current_command_line_byte_count);
    if (!command_line_status) {
        fprintf(stderr, "Error reading command line for '%s'.\n", command_line_path);
        free(process_info_dir);
        free(command_line_path);
        return NULL;
    }
    TRACE_BYTES(current_command_line_bytes, current_command_line_byte_count);
    free(command_line_path);

    // Replace null characters separating commands with spaces.
    for (int i = 0; i < current_command_line_byte_count; ++i) {
        if (current_command_line_bytes[i] == 0) {
            current_command_line_bytes[i] = ' ';
        }
    }
    char* current_command_line_string = string_length_to_null_terminated(
        current_command_line_bytes, current_command_line_byte_count);
    free(current_command_line_bytes);

    TRACE_STR(current_command_line_string);

    char* input_link_path = string_alloc_sprintf("%s/fd/0", process_info_dir);
    free(process_info_dir);
    char* input_link_result_bytes = malloc(PATH_MAX);
    const int input_link_result_byte_count = readlink(
        input_link_path, input_link_result_bytes, PATH_MAX);
    free(input_link_path);
    if (input_link_result_byte_count == -1) {
        fprintf(stderr, "Error reading input information for '%s'.\n", input_link_path);
        free(input_link_result_bytes);
        free(current_command_line_string);
        return NULL;
    }

    char* input_link_result_string = string_length_to_null_terminated(
        input_link_result_bytes, input_link_result_byte_count);
    free(input_link_result_bytes);

    TRACE_STR(input_link_result_string);

    const char* pipe_prefix = "pipe:[";
    const char* pipe_suffix = "]";
    if (string_starts_with(input_link_result_string, pipe_prefix)) {
        const int process_number_start = strlen(pipe_prefix);
        const int process_number_end = strlen(input_link_result_string) - strlen(pipe_suffix);
        char* process_number_string = string_from_range(input_link_result_string, process_number_start, process_number_end);
        uint32_t process_number;
        const bool is_process_number = string_to_int32(process_number_string, &process_number);
        if (is_process_number) {
            char* previous_command_line_string = get_command_line(process_number_string);
            if (previous_command_line_string != NULL) {
                char* new_command_line_string = string_alloc_sprintf("%s | %s",
                    previous_command_line_string, current_command_line_string);
                free(current_command_line_string);
                current_command_line_string = new_command_line_string;
            }
        }
        free(process_number_string);
    }
    free(input_link_result_string);

    TRACE_STR(current_command_line_string);

    return current_command_line_string;
}

static void append_to_log_contents(uint8_t* buffer_bytes, size_t buffer_byte_count) {
    pthread_mutex_lock(&g_log_content_mutex);

    const size_t new_log_contents_byte_count = (g_log_content_byte_count + buffer_byte_count);
    uint8_t* new_log_content_bytes = realloc(g_log_content_bytes, new_log_contents_byte_count);
    uint8_t* old_log_content_bytes_end = (new_log_content_bytes + g_log_content_byte_count);
    // Append the new data to the end of the buffer.
    memcpy(old_log_content_bytes_end, buffer_bytes, buffer_byte_count);

    g_log_content_bytes = new_log_content_bytes;
    g_log_content_byte_count = new_log_contents_byte_count;

    pthread_mutex_unlock(&g_log_content_mutex);
}

static bool mirror_stdin() {
 
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

            append_to_log_contents(read_buffer_bytes, bytes_read);
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
        if (settings->use_external_ip) {
            // See https://unix.stackexchange.com/questions/22615/how-can-i-get-my-external-ip-address-in-a-shell-script
            const char* dig_command = "dig @ns1.google.com TXT o-o.myaddr.l.google.com +short";
            FILE *dig_fp = popen(dig_command, "r");
            if (dig_fp == NULL) {
                fprintf(stderr, "Tried to find external IP, but dig command failed.\n");
                return 1;
            }
            const size_t ip_line_max_byte_count = 1024;
            char* ip_line_bytes = malloc(ip_line_max_byte_count);
            const size_t read_byte_count = fread(ip_line_bytes, 1, ip_line_max_byte_count, dig_fp);
            if (read_byte_count == 0) {
                fprintf(stderr, "Tried to find external IP, but dig command returned nothing.\n");
                return 1;
            }
            char* ip_line_string = string_length_to_null_terminated(ip_line_bytes, read_byte_count);
            free(ip_line_bytes);
            char* unquoted_string = string_from_range(ip_line_string, 1, strlen(ip_line_string) - 2);
            free(ip_line_string);
            host_name = unquoted_string;
            pclose(dig_fp);
        }
        if (host_name == NULL) {
            static struct utsname name_info;
            uname(&name_info);
            host_name = name_info.nodename;
        }
    }

    char* service_url = build_url(protocol, host_name, port);
    print_text_as_qr_to_terminal(service_url);
    fprintf(stderr, "%s\n", service_url);
    free(service_url);

    // I'd like to include the pipe input command in the log, but this approach
    // doesn't work, so commenting out for now.
    // char* my_pid_string = string_alloc_sprintf("%d", getpid());
    // char* command_line = get_command_line(my_pid_string);
    // free(command_line);
    // fprintf(stderr, "%s\n", command_line);
    // append_to_log_contents(command_line, strlen(command_line));
    // append_to_log_contents("\n", 1);
    // free(command_line);

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

    mirror_stdin();

    MHD_stop_daemon(d);

    settings_free(settings);
    return 0;
}
