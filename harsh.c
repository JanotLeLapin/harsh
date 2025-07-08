#include <fcntl.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>

#include "harsh.h"

int
h_graph_render_wav32(const char *filename, h_hm_t *g, h_context *ctx, size_t block_count)
{
  float *buf;
  h_graph_node_t *out;
  SF_INFO sfinfo;
  SNDFILE *f;
  size_t i;

  buf = malloc(sizeof(float) * BLOCK_SIZE * 2);
  if (0 == buf) {
    perror("malloc");
    return -1;
  }

  out = h_hm_get(g, "output");

  sfinfo.frames = block_count * BLOCK_SIZE;
  sfinfo.samplerate = ctx->sr;
  sfinfo.channels = 2;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

  f = sf_open(filename, SFM_WRITE, &sfinfo);
  if (0 == f) {
    fprintf(stderr, "sf_open: could not open file\n");
    return -1;
  }

  while (ctx->current_block < block_count) {
    h_graph_process_node(g, out, ctx);
    for (i = 0; i < BLOCK_SIZE; i++) {
      buf[i * 2] = out->out[0][i];
      buf[i * 2 + 1] = out->out[1][i];
    }
    sf_writef_float(f, buf, BLOCK_SIZE);
    ctx->current_block++;
  }

  sf_close(f);
  free(buf);

  return 0;
}

int
main(int argc, char **argv)
{
  h_hm_t graph;
  int fd;
  size_t i, sample_count = 512 * 1000;
  h_context ctx;
  h_dsl_ctx_t dsl_ctx;
  h_dsl_error_t *err;
  char *label;
  clock_t start, end;

  char *filename;

  if (argc < 2) {
    fprintf(stderr, "missing input file\n");
    return -1;
  } else {
    filename = argv[1];
  }

  ctx.current_block = 0;
  ctx.sr = 44100.0f;

  for (i = 2; i < argc; i++) {
    if (!strcmp("--sample-rate", argv[i]) || !strcmp("-sr", argv[i])) {
      ctx.sr = strtof(argv[++i], NULL);
    }
  }

  fd = open(filename, O_RDONLY);
  if (-1 == fd) {
    perror("open");
    return -1;
  }

  dsl_ctx.src_len = lseek(fd, 0, SEEK_END);
  dsl_ctx.src = mmap(0, dsl_ctx.src_len, PROT_READ, MAP_PRIVATE, fd, 0);
  dsl_ctx.error_count = 0;
  dsl_ctx.failed = 0;

  h_dsl_load(&graph, &dsl_ctx);

  for (i = 0; i < dsl_ctx.error_count; i++) {
    err = &dsl_ctx.errors[i];
    switch (err->type) {
    case H_DSL_ERROR_WARN:
      label = "warn";
      break;
    case H_DSL_ERROR_SEVERE:
      label = "severe";
      break;
    }
    fprintf(stderr, "%s: %.*s: %s: '%.*s'\n", label, (int) err->node.plain.len, err->node.plain.p, err->message, (int) err->problem.plain.len, err->problem.plain.p);
  }

  munmap(dsl_ctx.src, dsl_ctx.src_len);
  close(fd);

  if (dsl_ctx.failed) {
    fprintf(stderr, "severe error encountered, could not compile graph\n");
  } else {
    h_graph_preview(&graph);

    start = clock();
    if (-1 == h_graph_render_wav32("out.wav", &graph, &ctx, 65536)) {
      fprintf(stderr, "could not render graph\n");
    }
    end = clock();

    fprintf(stderr, "rendered %ld samples (%f sample rate, %f seconds), took %fs.\n", sample_count, ctx.sr, (float) sample_count / ctx.sr, (float) (end - start) / CLOCKS_PER_SEC);
  }

  h_graph_free(&graph);

  return 0;
}
