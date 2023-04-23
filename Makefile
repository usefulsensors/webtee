LDFLAGS := \
 -lmicrohttpd

SHARED_FLAGS := \
	-O3 \
	-I. \
	-Ithird_party \
	-Iutils

CPPFLAGS := \
	$(SHARED_FLAGS) \
	-std=gnu99 \

CXXFLAGS := \
	$(SHARED_FLAGS)

ARFLAGS := -r

GEN_DIR := gen/
OBJS_DIR := $(GEN_DIR)objs/
LIBS_DIR := $(GEN_DIR)libs/
BIN_DIR := $(GEN_DIR)bin/

QRCODEGEN_SRCS := \
	qrcodegen/qrcodegen.c
QRCODEGEN_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(QRCODEGEN_SRCS)))
QRCODEGEN_LIB := $(LIBS_DIR)/libqrcodegen.a

STRING_UTILS_SRCS := \
	utils/string_utils.c
STRING_UTILS_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(STRING_UTILS_SRCS)))
STRING_UTILS_LIB := $(LIBS_DIR)/libstringutils.a

STRING_UTILS_TEST_SRCS := \
	utils/string_utils_test.c
STRING_UTILS_TEST_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(STRING_UTILS_TEST_SRCS)))
STRING_UTILS_TEST_BIN := $(BIN_DIR)/string_utils_test

YARGS_SRCS := \
	utils/yargs.c
YARGS_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(YARGS_SRCS)))
YARGS_LIB := $(LIBS_DIR)/libyargs.a

YARGS_TEST_SRCS := \
	utils/yargs_test.c
YARGS_TEST_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(YARGS_TEST_SRCS)))
YARGS_TEST_BIN := $(BIN_DIR)/yargs_test

SETTINGS_SRCS := \
	settings.c
SETTINGS_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(SETTINGS_SRCS)))
SETTINGS_LIB := $(LIBS_DIR)/libsettings.a

SETTINGS_TEST_SRCS := \
	settings_test.c
SETTINGS_TEST_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(SETTINGS_TEST_SRCS)))
SETTINGS_TEST_BIN := $(BIN_DIR)/settings_test

FILE_UTILS_SRCS := \
	utils/file_utils.c
FILE_UTILS_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(FILE_UTILS_SRCS)))
FILE_UTILS_LIB := $(LIBS_DIR)/libfileutils.a

FILE_UTILS_TEST_SRCS := \
	utils/file_utils_test.c
FILE_UTILS_TEST_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(FILE_UTILS_TEST_SRCS)))
FILE_UTILS_TEST_BIN := $(BIN_DIR)/file_utils_test

WEBTEE_SRCS := \
	main.c
WEBTEE_OBJS := $(patsubst %.cc, $(OBJS_DIR)%.o, $(patsubst %.c, $(OBJS_DIR)%.o, $(WEBTEE_SRCS)))
WEBTEE_BIN := $(BIN_DIR)/webtee

$(OBJS_DIR)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(OBJS_DIR)%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(WEBTEE_BIN) $(STRING_UTILS_TEST_BIN) $(YARGS_TEST_BIN) $(SETTINGS_TEST_BIN)

test: \
  run_string_utils_test \
  run_yargs_test \
  run_settings_test

$(QRCODEGEN_LIB): $(QRCODEGEN_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(STRING_UTILS_LIB): $(STRING_UTILS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(STRING_UTILS_TEST_BIN): $(STRING_UTILS_TEST_OBJS) $(STRING_UTILS_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(STRING_UTILS_TEST_OBJS) -L$(LIBS_DIR) -lstringutils $(LDFLAGS) -o $@

run_string_utils_test: $(STRING_UTILS_TEST_BIN)
	$<

$(YARGS_LIB): $(YARGS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(YARGS_TEST_BIN): $(YARGS_TEST_OBJS) $(YARGS_LIB) $(STRING_UTILS_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(YARGS_TEST_OBJS) -L$(LIBS_DIR) -lstringutils -lyargs $(LDFLAGS) -o $@

run_yargs_test: $(YARGS_TEST_BIN)
	$<

$(SETTINGS_LIB): $(SETTINGS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(SETTINGS_TEST_BIN): $(SETTINGS_TEST_OBJS) $(YARGS_LIB) $(STRING_UTILS_LIB) $(SETTINGS_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SETTINGS_TEST_OBJS) -L$(LIBS_DIR) -lyargs -lstringutils -lsettings $(LDFLAGS) -o $@

run_settings_test: $(SETTINGS_TEST_BIN)
	$<

$(FILE_UTILS_LIB): $(FILE_UTILS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(FILE_UTILS_TEST_BIN): $(FILE_UTILS_TEST_OBJS) $(YARGS_LIB) $(STRING_UTILS_LIB) $(FILE_UTILS_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(FILE_UTILS_TEST_OBJS) -L$(LIBS_DIR) -lstringutils -lfileutils $(LDFLAGS) -o $@

run_file_utils_test: $(FILE_UTILS_TEST_BIN)
	$<

$(WEBTEE_BIN): $(WEBTEE_OBJS) $(QRCODEGEN_LIB) $(STRING_UTILS_LIB) $(YARGS_LIB) $(SETTINGS_LIB) $(FILE_UTILS_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(WEBTEE_OBJS) -L$(LIBS_DIR) -lqrcodegen -lsettings -lstringutils -lyargs -lfileutils $(LDFLAGS) -o $@

clean:
	$(shell rm -rf $(GEN_DIR))
