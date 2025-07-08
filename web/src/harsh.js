export class HarshGraph {
  /**
   * @param {number} graphPtr
   * @param {number} bufPtr 
   * @param {number} bufSize 
   * @param {number} ctxPtr
   * @param {number} outPtr
   * @param {(graph: number, out: number, ctx: number, size: number) => void} renderBlock 
   */
  constructor(graphPtr, bufPtr, bufSize, ctxPtr, outPtr, renderBlock) {
    this.graphPtr = graphPtr
    this.bufPtr = bufPtr
    this.bufSize = bufSize
    this.ctxPtr = ctxPtr
    this.outPtr = outPtr
    this.renderBlock = renderBlock
  }

  /**
   * @param {string} src 
   * @param {number} bufSize 
   * @param {number} sampleRate
   */
  static create(src, bufSize, sampleRate) {
    const hmSize = window.Module.ccall('w_h_hm_t_size', 'number')
    const graphPtr = window.Module._malloc(hmSize)

    const srcEncoded = new TextEncoder().encode(src + '\0')
    const srcPtr = window.Module._malloc(srcEncoded.length)
    window.Module.HEAPU8.set(srcEncoded, srcPtr)

    const dslCtxSize = window.Module.ccall('w_h_dsl_ctx_t_size', 'number')
    const dslCtx = window.Module._malloc(dslCtxSize)
    window.Module.ccall('w_dsl_ctx_init', null, ['number', 'number', 'number'], [dslCtx, srcPtr, srcEncoded.length])

    window.Module.ccall('h_dsl_load', null, ['number', 'number'], [graphPtr, dslCtx])

    window.Module._free(dslCtx)
    window.Module._free(srcPtr)

    const outEncoded = new TextEncoder().encode('output\0')
    const outPtr = window.Module._malloc(outEncoded.length)
    window.Module.HEAPU8.set(outEncoded, outPtr)

    const ctxSize = window.Module.ccall('w_h_context_size', 'number')
    const ctxPtr = window.Module._malloc(ctxSize)
    window.Module.ccall('w_context_init', null, ['number', 'number'], [ctxPtr, sampleRate])

    const bufPtr = window.Module._malloc(2 * 4 * bufSize)

    const renderBlock = window.Module.cwrap('w_graph_render_block', null, ['number', 'number', 'number', 'number'])

    return new HarshGraph(graphPtr, bufPtr, bufSize, ctxPtr, outPtr, renderBlock)
  }

  /**
   * @param {Float32Array} samples 
   */
  render(samples) {
    this.renderBlock(this.graphPtr, this.outPtr, this.ctxPtr, this.bufPtr)
    samples.set(new Float32Array(window.Module.HEAPF32.buffer, this.bufPtr, this.bufSize * 2))
  }

  free() {
    window.Module._free(this.bufPtr)
    window.Module._free(this.ctxPtr)
    window.Module._free(this.outPtr)
    window.Module.ccall('h_graph_free', null, ['number'], [this.graphPtr])
  }
}
