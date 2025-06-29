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
h_graph_render_wav32(const char *filename, h_hm_t *g, h_context *ctx, size_t sample_count, size_t buf_size)
{
  float *buf;
  h_graph_node_t *out;
  SF_INFO sfinfo;
  SNDFILE *f;
  size_t i;

  buf = malloc(sizeof(float) * buf_size);
  if (0 == buf) {
    perror("malloc");
    return -1;
  }

  out = h_hm_get(g, "output");

  sfinfo.frames = sample_count;
  sfinfo.samplerate = ctx->sr;
  sfinfo.channels = 1;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

  f = sf_open(filename, SFM_WRITE, &sfinfo);
  if (0 == f) {
    fprintf(stderr, "sf_open: could not open file\n");
    return -1;
  }

  while (ctx->current_frame < sample_count) {
    h_graph_render_block(g, out, ctx, buf, buf_size);
    sf_write_float(f, buf, buf_size);
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
  size_t i, len, sample_count = 512 * 1000;
  h_context ctx;
  void *src;
  clock_t start, end;

  char *filename;

  if (argc < 2) {
    fprintf(stderr, "missing input file\n");
    return -1;
  } else {
    filename = argv[1];
  }

  ctx.current_frame = 0;
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
  len = lseek(fd, 0, SEEK_END);
  src = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);

  h_dsl_load(&graph, src, len);

  munmap(src, len);
  close(fd);

  h_graph_preview(&graph);

  start = clock();
  if (-1 == h_graph_render_wav32("out.wav", &graph, &ctx, 512 * 1000, 512)) {
    fprintf(stderr, "could not render graph\n");
  }
  end = clock();

  h_graph_free(&graph);

  fprintf(stderr, "rendered %ld samples (%f sample rate, %f seconds), took %fs.\n", sample_count, ctx.sr, (float) sample_count / ctx.sr, (float) (end - start) / CLOCKS_PER_SEC);

  return 0;
}
