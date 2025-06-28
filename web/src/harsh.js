export class HarshGraph {
  /**
   * @param {number} graphPtr
   * @param {number} bufPtr 
   * @param {number} bufSize 
   * @param {number} ctxPtr
   * @param {number} outPtr
   * @param {number} srcPtr
   * @param {(graph: number, out: number, ctx: number, size: number) => void} renderBlock 
   */
  constructor(graphPtr, bufPtr, bufSize, ctxPtr, outPtr, srcPtr, renderBlock) {
    this.graphPtr = graphPtr
    this.bufPtr = bufPtr
    this.bufSize = bufSize
    this.ctxPtr = ctxPtr
    this.outPtr = outPtr
    this.srcPtr = srcPtr
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

    window.Module.ccall('h_dsl_load', null, ['number', 'number', 'number'], [graphPtr, srcPtr, srcEncoded.length])

    const outEncoded = new TextEncoder().encode('output\0')
    const outPtr = window.Module._malloc(outEncoded.length)
    window.Module.HEAPU8.set(outEncoded, outPtr)

    const ctxSize = window.Module.ccall('w_h_context_size', 'number')
    const ctxPtr = window.Module._malloc(ctxSize)
    window.Module.ccall('w_context_init', null, ['number', 'number'], [ctxPtr, sampleRate])

    const bufPtr = window.Module._malloc(4 * bufSize)

    const renderBlock = window.Module.cwrap('w_graph_render_block', null, ['number', 'number', 'number', 'number', 'number'])

    return new HarshGraph(graphPtr, bufPtr, bufSize, ctxPtr, outPtr, srcPtr, renderBlock)
  }

  /**
   * @param {Float32Array} samples 
   */
  render(samples) {
    this.renderBlock(this.graphPtr, this.outPtr, this.ctxPtr, this.bufPtr, this.bufSize)
    samples.set(new Float32Array(window.Module.HEAPF32.buffer, this.bufPtr, this.bufSize))
  }

  free() {
    window.Module._free(this.bufPtr)
    window.Module._free(this.ctxPtr)
    window.Module._free(this.outPtr)
    window.Module._free(this.srcPtr)
    window.Module.ccall('h_graph_free', null, ['number'], [this.graphPtr])
  }
}
