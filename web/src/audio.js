const CANVAS_WIDTH = 1024
const CANVAS_HEIGHT = 256

export function setupAudio(element) {
  let synth = `
  (synth
    (def target-freq (+ 8000.0 (* 4000.0 (sine :freq 0.1 :phase 0.0))))
    (def bits (* 4 (+ 1.2 (sine :freq 0.15 :phase 0.0))))
    (def output (bitcrush (diode (sine :freq 55.0 :phase 0.0)) :target_freq (ref target-freq) :bits (ref bits))))
  `

  console.log('setting up audio')

  element.querySelector('button').addEventListener('click', () => {
    const canvas = element.querySelector('canvas')
    const ctx = canvas.getContext('2d')

    const hmSize = window.Module.ccall('w_h_hm_t_size', 'number')
    const graphPtr = window.Module._malloc(hmSize)

    const srcEncoded = new TextEncoder().encode(synth + '\0')
    const srcPtr = window.Module._malloc(srcEncoded.length)
    window.Module.HEAPU8.set(srcEncoded, srcPtr)

    window.Module.ccall('h_dsl_load', null, ['number', 'number', 'number'], [graphPtr, srcPtr, srcEncoded.length])

    const outEncoded = new TextEncoder().encode('output\0')
    const outPtr = window.Module._malloc(outEncoded.length)
    window.Module.HEAPU8.set(outEncoded, outPtr)

    const ctxSize = window.Module.ccall('w_h_context_size', 'number')
    const ctxPtr = window.Module._malloc(ctxSize)
    window.Module.ccall('w_context_init', null, ['number', 'number'], [ctxPtr, 44100])

    const bufSize = 512 * 10
    const bufPtr = window.Module._malloc(4 * bufSize)

    console.log('processing')

    window.Module.ccall('w_graph_render_block', null, ['number', 'number', 'number', 'number', 'number'], [graphPtr, outPtr, ctxPtr, bufPtr, bufSize])

    const samples = new Float32Array(window.Module.HEAPF32.buffer, bufPtr, bufSize)

    ctx.fillStyle = 'white'
    ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT)

    ctx.fillStyle = 'blue'
    ctx.moveTo(0, CANVAS_HEIGHT / 2)
    for (let i = 0; i < bufSize; i++) {
      console.log(samples[i])
      ctx.lineTo(
        i / bufSize * CANVAS_WIDTH,
        CANVAS_HEIGHT - ((samples[i] * 0.5 + 0.5) * CANVAS_HEIGHT),
      )
    }
    ctx.stroke()

    window.Module._free(bufPtr)
    window.Module._free(ctxPtr)
    window.Module._free(outPtr)
    window.Module._free(srcPtr)
    window.Module.ccall('h_graph_free', null, ['number'], [graphPtr])
  })
}
