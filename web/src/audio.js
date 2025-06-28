import { basicSetup } from 'codemirror';
import { EditorView } from '@codemirror/view';
import { HarshGraph } from './harsh';

const CANVAS_WIDTH = 1024
const CANVAS_HEIGHT = 256

const BLOCK_SIZE = 512

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

  const samples = new Float32Array(BLOCK_SIZE)

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
    for (let i = 0; i < samples.length; i++) {
      ctx.lineTo(
        i / samples.length * CANVAS_WIDTH,
        CANVAS_HEIGHT - ((samples[i] * 0.5 + 0.5) * CANVAS_HEIGHT),
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
    workletNode = new AudioWorkletNode(audioCtx, 'harsh-processor')
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

  element.querySelector('button').addEventListener('click', () => {
    synth = editorView.state.doc.toString()

    if (graph) {
      graph.free()
    }

    graph = HarshGraph.create(synth, BLOCK_SIZE, audioCtx.sampleRate)

    startAudio()

    for (let i = 0; i < 4; i++) {
      renderBlock()
    }
  })

  initAudio()
}
