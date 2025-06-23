#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "harsh.h"

int
main(void)
{
  h_hm_t graph;
  h_graph_node_t *out;
  int fd;
  size_t len;
  void *src;
  h_context ctx;

  fd = open("example.scm", O_RDONLY);
  len = lseek(fd, 0, SEEK_END);
  src = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);

  h_dsl_load(&graph, src, len);
  out = h_hm_get(&graph, "output");

  ctx.current_frame = 0;
  ctx.sr = 44100;

  for (ctx.current_frame = 0; ctx.current_frame < 100; ctx.current_frame++) {
    h_graph_process_node(&graph, out, &ctx);
    printf("current sample: %f\n", out->out);
  }

  h_hm_free(&graph);

  printf("hello, world!\n");
  return 0;
}
