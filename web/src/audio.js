import { basicSetup } from 'codemirror';
import { EditorView } from '@codemirror/view';
import { HarshGraph } from './harsh';

const CANVAS_WIDTH = 1024
const CANVAS_HEIGHT = 256

const BLOCK_SIZE = 128

export function setupAudio(element) {
  const audioCtx = new (window.AudioContext || window.webkitAudioContext)()
  let workletNode;

  let graph;

  /*
  let synth = `(synth
  (def target-freq (+ 8000.0 (* 4000.0 (sine :freq 0.1 :phase 0.0))))
  (def bits (* 8 0.5 (+ 1.2 (sine :freq 0.15 :phase 0.0))))
  (def output (bitcrush (diode (sine :freq 55.0 :phase 0.0)) :target_freq (ref target-freq) :bits (ref bits))))`
  */

  let synth;

  const editorTheme = EditorView.theme({
    "&": {
      color: "white",
      backgroundColor: "#034"
    },
    ".cm-content": {
      caretColor: "white"
    },
    "&.cm-focused .cm-cursor": {
      borderLeftColor: "white"
    },
    "&.cm-focused .cm-selectionBackground, ::selection": {
      backgroundColor: "#074"
    },
    ".cm-gutters": {
      backgroundColor: "#045",
      color: "#ddd",
      border: "none"
    },
  }, { dark: true })

  const editorView = new EditorView({
    parent: element.querySelector('#editor'),
    extensions: [basicSetup, editorTheme],
  })

  element.querySelector('#editor').appendChild(editorView.dom)

  const samples = new Float32Array(BLOCK_SIZE * 2)

  console.log('setting up audio')

  function drawSignal() {
    const canvas = element.querySelector('canvas')
    const ctx = canvas.getContext('2d')

    ctx.beginPath()
    ctx.clearRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT)

    ctx.beginPath()
    ctx.fillStyle = 'white'
    ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT)

    ctx.beginPath()
    ctx.fillStyle = 'blue'
    ctx.moveTo(0, CANVAS_HEIGHT / 2)
    for (let i = 0; i < BLOCK_SIZE; i++) {
      ctx.lineTo(
        i / BLOCK_SIZE * CANVAS_WIDTH,
        CANVAS_HEIGHT - ((samples[i * 2] * 0.5 + 0.5) * CANVAS_HEIGHT),
      )
    }
    ctx.stroke()
  }

  function renderBlock() {
    graph.render(samples)
    sendAudioBlock(samples)
    drawSignal()
  }

  async function initAudio() {
    await audioCtx.audioWorklet.addModule(import.meta.env.BASE_URL + '/harsh-processor.js')
    workletNode = new AudioWorkletNode(audioCtx, 'harsh-processor', {
      outputChannelCount: [2],
      numberOfInputs: 0,
      numberOfOutputs: 1,
      channelCount: 2,
      channelCountMode: 'explicit',
    })
    workletNode.port.onmessage = (e) => {
      if (e.data.type === 'requestBlock') {
        renderBlock()
      }
    }
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

  function sendVolume(volume) {
    if (workletNode) {
      workletNode.port.postMessage({ volume })
    }
  }

  element.querySelector('button').addEventListener('click', () => {
    const log = document.querySelector('#log')

    synth = editorView.state.doc.toString()

    if (graph) {
      graph.free()
    }

    const info = document.createElement('p')
    info.textContent = 'compiling'
    log.appendChild(info)

    const res = HarshGraph.create(synth, BLOCK_SIZE, audioCtx.sampleRate)
    graph = res.graph

    for (let i = 0; i < res.errors.length; i++) {
      const error = document.createElement('p')
      error.textContent = res.errors[i]
      log.appendChild(error)
    }

    startAudio()

    for (let i = 0; i < 4; i++) {
      renderBlock()
    }
  })

  element.querySelector('#volume').addEventListener('change', (e) => {
    sendVolume(e.target.value * 0.01)
  })

  initAudio()
}
