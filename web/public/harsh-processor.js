const BLOCK_SIZE = 512

class HarshProcessor extends AudioWorkletProcessor {
  constructor() {
    super()
    this.buffer = new Float32Array(BLOCK_SIZE * 16)
    this.readIdx = 0
    this.writeIdx = 0
    this.underflowCount = 0

    this.port.onmessage = (e) => {
      if (e.data.samples) {
        this.enqueue(e.data.samples)
      }
    }
  }

  enqueue(samples) {
    const available = this.buffer.length - this.usedSpace()
    if (available < samples.length) {
      return
    }

    for (let i = 0; i < samples.length; i++) {
      this.buffer[this.writeIdx] = samples[i]
      this.writeIdx = (this.writeIdx + 1) % this.buffer.length
    }
  }

  usedSpace() {
    return (this.writeIdx >= this.readIdx) ? this.writeIdx - this.readIdx : this.buffer.length - this.readIdx + this.writeIdx
  }

  process(_, outputs) {
    const out = outputs[0][0]
    const needed = out.length

    if (this.usedSpace() < needed) {
      this.underflowCount++
      console.warn('UNDERFLOW!')
      out.fill(0)
      return true
    }

    for (let i = 0; i < needed; i++) {
      out[i] = this.buffer[this.readIdx]
      this.readIdx = (this.readIdx + 1) % this.buffer.length
    }

    return true
  }
}

registerProcessor('harsh-processor', HarshProcessor)
