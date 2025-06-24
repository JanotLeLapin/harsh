CC = gcc
LDFLAGS = -lm -lsndfile
CFLAGS = -Wall -Wextra -O3

EMCC = emcc
LDFLAGS_WASM = -lm -s ALLOW_MEMORY_GROWTH=1
CFLAGS_WASM = -Wall -Wextra -O3

SRCS = harsh.c graph.c dsl.c hm.c vec.c
OBJS = $(SRCS:.c=.o)

SRCS_WASM = wrapper.c graph.c dsl.c hm.c vec.c
OBJS_WASM = $(SRCS_WASM:.c=.wasm.o)

TARGET = harsh
TARGET_WASM = harsh.js

all: $(TARGET)

wasm: $(TARGET_WASM)

$(TARGET): harsh.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_WASM): $(OBJS_WASM)
	$(EMCC) $^ -o $@ \
		-s EXPORTED_FUNCTIONS='["_malloc","_free","_w_h_hm_t_size","_h_dsl_load","_h_graph_free"]' \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8"]' \
		$(LDFLAGS_WASM)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.wasm.o: %.c
	$(EMCC) $(CFLAGS_WASM) -c -o $@ $<

clean:
	rm -f $(OBJS) harsh.o $(TARGET)
	rm -f $(OBJS_WASM) $(TARGET_WASM) harsh.wasm

.PHONY: all wasm clean
