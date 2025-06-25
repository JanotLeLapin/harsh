const CANVAS_WIDTH = 1024
const CANVAS_HEIGHT = 256

const BLOCK_SIZE = 512

export function setupAudio(element) {
  const audioCtx = new (window.AudioContext || window.webkitAudioContext)()
  let workletNode;
  const harshCtx = {
    graphPtr: undefined,
    srcPtr: undefined,
    outPtr: undefined,
    ctxPtr: undefined,
    bufPtr: undefined,
  }

  let synth = `(synth
    (def output (sine :freq 440.0 :phase 0.0)))`

  console.log('setting up audio')

  async function initAudio() {
    await audioCtx.audioWorklet.addModule('/harsh-processor.js')
    workletNode = new AudioWorkletNode(audioCtx, 'harsh-processor')
    workletNode.connect(audioCtx.destination)
  }

  function startAudio() {
    if (audioCtx.state === 'suspended') {
      audioCtx.resume()
    }
  }

  function sendAudioBlock(samples) {
    if (workletNode) {
      workletNode.port.postMessage({ samples })
    }
  }

  function allocateResources() {
    const hmSize = window.Module.ccall('w_h_hm_t_size', 'number')
    harshCtx.graphPtr = window.Module._malloc(hmSize)

    const srcEncoded = new TextEncoder().encode(synth + '\0')
    harshCtx.srcPtr = window.Module._malloc(srcEncoded.length)
    window.Module.HEAPU8.set(srcEncoded, harshCtx.srcPtr)

    window.Module.ccall('h_dsl_load', null, ['number', 'number', 'number'], [harshCtx.graphPtr, harshCtx.srcPtr, srcEncoded.length])

    const outEncoded = new TextEncoder().encode('output\0')
    harshCtx.outPtr = window.Module._malloc(outEncoded.length)
    window.Module.HEAPU8.set(outEncoded, harshCtx.outPtr)

    const ctxSize = window.Module.ccall('w_h_context_size', 'number')
    harshCtx.ctxPtr = window.Module._malloc(ctxSize)
    window.Module.ccall('w_context_init', null, ['number', 'number'], [harshCtx.ctxPtr, 44100])

    harshCtx.bufPtr = window.Module._malloc(4 * BLOCK_SIZE)
  }

  function freeResources() {
    window.Module._free(harshCtx.bufPtr)
    window.Module._free(harshCtx.ctxPtr)
    window.Module._free(harshCtx.outPtr)
    window.Module._free(harshCtx.srcPtr)
    window.Module.ccall('h_graph_free', null, ['number'], [harshCtx.graphPtr])
  }

  function processAudio() {
    if (!harshCtx.graphPtr) {
      allocateResources()
    }

    window.Module.ccall('w_graph_render_block', null, ['number', 'number', 'number', 'number', 'number'], [harshCtx.graphPtr, harshCtx.outPtr, harshCtx.ctxPtr, harshCtx.bufPtr, BLOCK_SIZE])

    const samples = new Float32Array(window.Module.HEAPF32.buffer, harshCtx.bufPtr, BLOCK_SIZE)
    const samplesCopy = new Float32Array(BLOCK_SIZE)
    samplesCopy.set(samples)

    setTimeout(processAudio, 0)

    sendAudioBlock(samplesCopy)
  }

  element.querySelector('button').addEventListener('click', () => {
    initAudio().then(startAudio)
    processAudio()
    // const canvas = element.querySelector('canvas')
    // const ctx = canvas.getContext('2d')

    // ctx.fillStyle = 'white'
    // ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT)

    // ctx.fillStyle = 'blue'
    // ctx.moveTo(0, CANVAS_HEIGHT / 2)
    // for (let i = 0; i < bufSize; i++) {
    //   console.log(samples[i])
    //   ctx.lineTo(
    //     i / bufSize * CANVAS_WIDTH,
    //     CANVAS_HEIGHT - ((samples[i] * 0.5 + 0.5) * CANVAS_HEIGHT),
    //   )
    // }
    // ctx.stroke()
  })
}
