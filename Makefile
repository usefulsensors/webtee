LDFLAGS := \
 -lmicrohttpd

SHARED_FLAGS := \
	-O3 \
	-I.

CPPFLAGS := \
	$(SHARED_FLAGS) \
	-std=c99 \

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

all: $(WEBTEE_BIN)

$(QRCODEGEN_LIB): $(QRCODEGEN_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(info $(QRCODEGEN_LIB))

$(WEBTEE_BIN): $(WEBTEE_OBJS) $(QRCODEGEN_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(WEBTEE_OBJS) -L$(LIBS_DIR) -lqrcodegen $(LDFLAGS) -o $@

clean:
	$(shell rm -rf $(GEN_DIR))
