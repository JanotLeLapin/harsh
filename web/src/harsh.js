const leNum = (buf) => buf.reduceRight((previous, current) => current | (previous << 8))

const getString = (ptr) => {
  const buf = new Uint8Array(window.Module.HEAPU8.buffer, ptr)
  for (let i = 0; i < 65535; i++) {
    if (buf[i] === 0) {
      return new TextDecoder().decode(buf.slice(0, i))
    }
  }
  return null
}

const getNodePlain = (buf) => {
  const pPtr = leNum(buf.slice(4, 8))
  const len = leNum(buf.slice(8, 12))

  const pBuf = new Uint8Array(window.Module.HEAPU8.buffer, pPtr, len)
  return new TextDecoder().decode(pBuf)
}

const getErrors = (ctx) => {
  const errorSize = window.Module.ccall('w_h_dsl_error_t_size', 'number')
  const nodeSize = window.Module.ccall('w_h_dsl_node_t_size', 'number')
  const errors = new Uint8Array(window.Module.HEAPU8.buffer, ctx + 8, errorSize * 64)
  const errorCount = leNum(new Uint8Array(window.Module.HEAPU8.buffer, ctx + 8 + errorSize * 64, 4))

  return new Array(errorCount)
    .fill(0)
    .map((_, i) => i * errorSize)
    .map((offset) => {
      const type = {
        0: 'warn',
        1: 'error',
      }[errors[offset]]

      const [node, problem] = [
        errors.slice(offset + 4, offset + nodeSize),
        errors.slice(offset + nodeSize + 4, offset + nodeSize * 2),
      ].map(getNodePlain)

      const messagePtr = leNum(errors.slice(offset + 4 + nodeSize * 2, offset + 4 + nodeSize * 2 + 4))
      const message = getString(messagePtr)

      return leNum(errors.slice(offset + 4, offset + 8)) + ': ' + type + ': ' + node + ': ' + message + ': ' + problem
    })
}

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
   * @returns {{ errors: string[], graph: HarshGraph }}
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
    const errors = getErrors(dslCtx)

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

    return {
      errors,
      graph: new HarshGraph(graphPtr, bufPtr, bufSize, ctxPtr, outPtr, renderBlock),
    }
  }

  /**
   * @param {Float32Array} samples 
   * @param {number} offset
   */
  render(samples, offset) {
    this.renderBlock(this.graphPtr, this.outPtr, this.ctxPtr, this.bufPtr)
    samples.set(new Float32Array(window.Module.HEAPF32.buffer, this.bufPtr, this.bufSize * 2), offset)
  }

  free() {
    window.Module._free(this.bufPtr)
    window.Module._free(this.ctxPtr)
    window.Module._free(this.outPtr)
    window.Module.ccall('h_graph_free', null, ['number'], [this.graphPtr])
  }
}
