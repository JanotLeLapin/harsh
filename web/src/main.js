import './style.css'
import { setupAudio } from './audio.js'

document.querySelector('#app').innerHTML = `
  <div>
    <h1>Harsh</h1>
    <div id="audio">
      <div id="editor"></div>
      <button>Generate</button>
      <canvas width="1024px" height="256px"></canvas>
    </div>
  </div>
`

setupAudio(document.querySelector('#audio'))
