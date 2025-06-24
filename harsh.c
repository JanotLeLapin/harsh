#include <fcntl.h>
#include <sndfile.h>
#include <stdio.h>
#include <string.h>
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
    for (i = 0; i < buf_size; i++) {
      h_graph_process_node(g, out, ctx);
      buf[i] = out->out;
      ctx->current_frame++;
    }
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
  size_t len;
  void *src;
  h_context ctx;

  char *filename;

  if (argc < 2) {
    filename = "example.scm";
  } else {
    filename = argv[1];
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

  ctx.current_frame = 0;
  ctx.sr = 44100;

  h_graph_preview(&graph);

  if (-1 == h_graph_render_wav32("out.wav", &graph, &ctx, 512 * 1000, 512)) {
    fprintf(stderr, "could not render graph\n");
  }

  h_graph_free(&graph);

  return 0;
}
