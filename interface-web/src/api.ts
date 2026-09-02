import type { AudioTelemetry, DeviceState, ScanResult, Settings, StateMessage } from './types'

const configuredUrl = import.meta.env.VITE_DEVICE_URL || 'http://transmissor-si4713.local'
export const deviceUrl = configuredUrl.replace(/\/$/, '')
export const usingMock = import.meta.env.VITE_USE_MOCK !== 'false'

let state: DeviceState = {
  desired: {
    frequencyKhz: 9950,
    powerDbuv: 100,
    antennaCap: 0,
    txEnabled: false,
    stereo: true,
    preemphasisUs: 50,
    audioDeviationKhz: 66,
    muted: false,
    rdsEnabled: true,
    rdsPi: 0x4713,
    rdsPs: 'SI4713',
    rdsText: 'Transmissor FM Si4713',
    rdsTemplate: '{data} {hora}',
    rdsSource: 'manual',
    onAirRssiThreshold: 50,
  },
  applied: {
    onAir: false,
    txActive: false,
    frequencyKhz: 9950,
    powerDbuv: 100,
    antennaCap: 52,
    audioLevelDbfs: -38,
    asq: 0,
  },
  receiver: {
    available: true,
    frequencyKhz: 9950,
    rssi: 75,
    rssiValid: true,
    threshold: 50,
    carrierDetected: false,
    rdsSynchronized: true,
    rdsPs: 'SI4713',
    rdsText: 'Transmissor FM Si4713',
  },
  system: {
    si4713Available: true,
    recovering: false,
    recoveries: 0,
    i2cCommunicationFailures: 0,
    rfStateMismatches: 0,
    si4713InterruptPin: 4,
    si4713InterruptCount: 0,
    si4713LastInterruptMs: 0,
    si4713LastInterrupt: 'none',
    si4713InterruptPending: false,
    scanRunning: false,
    scanFinished: true,
    scanProgress: 100,
    wifiConnected: true,
    wifiPortalActive: false,
    ip: '192.168.1.50',
    timeValid: true,
    uptimeMs: 120000,
    firmwareVersion: '0.1.23-rda-rds',
    frequencyMinKhz: 7610,
    frequencyMaxKhz: 10800,
    frequencyStepKhz: 10,
  },
}

let phrases = ['Clássicos e sucessos', 'Você está ouvindo a nossa rádio', 'Música e informação']
const seededNoise = (index: number) => 7 + ((index * 17 + index * index * 3) % 38)
let scan: ScanResult = {
  running: false,
  finished: true,
  progress: 100,
  recommendedFrequencyKhz: 9570,
  recommendedNoiseLevel: 2,
  measurements: Array.from({ length: 83 }, (_, index) => ({
    frequencyKhz: 7610 + index * 25,
    noiseLevel: index === 33 ? 2 : seededNoise(index),
  })),
}

const clone = <T,>(value: T): T => structuredClone(value)

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${deviceUrl}${path}`, {
    ...init,
    headers: { 'Content-Type': 'application/json', ...init?.headers },
  })
  if (!response.ok) {
    const body = await response.json().catch(() => ({}))
    throw new Error(body.detail || body.error || `Falha HTTP ${response.status}`)
  }
  return response.status === 204 ? (undefined as T) : response.json()
}

export async function getState(): Promise<DeviceState> {
  return usingMock ? clone(state) : request('/api/v1/state')
}

export async function applySettings(settings: Settings): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/settings', { method: 'PUT', body: JSON.stringify(settings) })
  state.desired = clone(settings)
  state.applied = {
    ...state.applied,
    txActive: settings.txEnabled && state.system.si4713Available,
    onAir: settings.txEnabled
      && state.system.si4713Available
      && state.receiver.rssi >= settings.onAirRssiThreshold,
    frequencyKhz: settings.frequencyKhz,
    powerDbuv: settings.powerDbuv,
    antennaCap: settings.antennaCap === 0
      ? state.applied.antennaCap || 52
      : settings.antennaCap,
  }
  state.receiver = {
    ...state.receiver,
    threshold: settings.onAirRssiThreshold,
    carrierDetected: state.applied.onAir,
  }
  return clone(state)
}

export async function previewFrequency(frequencyKhz: number): Promise<DeviceState> {
  if (!usingMock) {
    return request('/api/v1/frequency/adjust', {
      method: 'PUT',
      body: JSON.stringify({ frequencyKhz }),
    })
  }
  state.desired.frequencyKhz = frequencyKhz
  state.applied.frequencyKhz = frequencyKhz
  state.applied.onAir = false
  state.applied.txActive = false
  state.receiver.carrierDetected = false
  return clone(state)
}

export async function setTransmission(enabled: boolean): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/tx', { method: 'POST', body: JSON.stringify({ enabled }) })
  state.desired.txEnabled = enabled
  state.applied.txActive = enabled && state.system.si4713Available
  state.applied.onAir = state.applied.txActive
    && state.receiver.rssi >= state.desired.onAirRssiThreshold
  state.receiver.carrierDetected = state.applied.onAir
  return clone(state)
}

export async function restartRf(): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/tx/restart', { method: 'POST' })
  state.applied.txActive = state.desired.txEnabled
      && state.system.si4713Available
  state.applied.onAir = state.applied.txActive
      && state.receiver.rssi >= state.desired.onAirRssiThreshold
  state.receiver.carrierDetected = state.applied.onAir
  return clone(state)
}

export async function acknowledgeSi4713Interrupt(): Promise<DeviceState> {
  if (!usingMock) {
    return request('/api/v1/si4713/interrupt/ack', { method: 'POST' })
  }
  state.system.si4713InterruptPending = false
  return clone(state)
}

export async function setAudioMonitoring(enabled: boolean): Promise<void> {
  if (usingMock) return
  // A interface solicita explicitamente a leitura periodica de audio na I2C.
  // O WebSocket pode permanecer conectado para receber estado e interrupcoes.
  await request('/api/v1/audio/monitor', {
    method: 'PUT',
    body: JSON.stringify({ enabled }),
  })
}

export async function saveSettings(): Promise<void> {
  if (!usingMock) await request('/api/v1/settings/save', { method: 'POST' })
}

export async function restoreDefaults(): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/settings/defaults', { method: 'POST' })
  state.desired = { ...state.desired, frequencyKhz: 9950, powerDbuv: 100, antennaCap: 0, txEnabled: false, onAirRssiThreshold: 50 }
  state.applied = { ...state.applied, frequencyKhz: 9950, powerDbuv: 100, antennaCap: 52, onAir: false, txActive: false }
  state.receiver = { ...state.receiver, frequencyKhz: 9950, threshold: 50, carrierDetected: false }
  return clone(state)
}

export async function startScan(): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/scan/start', { method: 'POST' })
  scan = { ...scan, running: true, finished: false, progress: 0 }
  state.system = { ...state.system, scanRunning: true, scanFinished: false, scanProgress: 0 }
  state.applied.onAir = false
  state.applied.txActive = false
  state.receiver.carrierDetected = false
  return clone(state)
}

export async function getScan(): Promise<ScanResult> {
  if (!usingMock) return request('/api/v1/scan/results')
  if (scan.running) {
    const next = Math.min(100, scan.progress + 8)
    scan = { ...scan, progress: next, running: next < 100, finished: next === 100 }
    state.system = { ...state.system, scanProgress: next, scanRunning: next < 100, scanFinished: next === 100 }
  }
  return clone(scan)
}

export async function applyScannedFrequency(frequencyKhz: number): Promise<DeviceState> {
  if (!usingMock) return request('/api/v1/scan/apply', { method: 'POST', body: JSON.stringify({ frequencyKhz }) })
  state.desired.frequencyKhz = frequencyKhz
  state.applied.frequencyKhz = frequencyKhz
  state.applied.txActive = state.desired.txEnabled && state.system.si4713Available
  state.applied.onAir = state.applied.txActive
    && state.receiver.rssi >= state.desired.onAirRssiThreshold
  state.receiver.frequencyKhz = frequencyKhz
  state.receiver.carrierDetected = state.applied.onAir
  return clone(state)
}

export async function getPhrases(): Promise<string[]> {
  if (usingMock) return clone(phrases)
  const response = await request<{ phrases: string[] }>('/api/v1/rds/phrases')
  return response.phrases
}

export async function putPhrases(next: string[]): Promise<string[]> {
  if (usingMock) {
    phrases = next.filter(Boolean).slice(0, 12)
    return clone(phrases)
  }
  const response = await request<{ phrases: string[] }>('/api/v1/rds/phrases', {
    method: 'PUT',
    body: JSON.stringify({ phrases: next }),
  })
  return response.phrases
}

export async function openWifiPortal(): Promise<void> {
  if (!usingMock) await request('/api/v1/wifi/portal', { method: 'POST' })
}

export type ConnectionStatus = 'connecting' | 'online' | 'offline'

export function subscribeAudio(
  onData: (telemetry: AudioTelemetry) => void,
  onStatus: (status: ConnectionStatus) => void,
  onState?: (state: DeviceState) => void,
): () => void {
  if (usingMock) {
    onStatus('online')
    let sequence = 0
    const timer = window.setInterval(() => {
      const level = state.applied.onAir && !state.desired.muted ? -32 + Math.round(Math.random() * 26) : -60
      state.applied.audioLevelDbfs = level
      state.applied.asq = 0
      onData({ type: 'audio', sequence: ++sequence, timestampMs: Date.now(), levelDbfs: level, asq: state.applied.asq, overmodulation: false, onAir: state.applied.onAir })
    }, 100)
    return () => window.clearInterval(timer)
  }

  const url = new URL(deviceUrl)
  const websocketUrl = `${url.protocol === 'https:' ? 'wss:' : 'ws:'}//${url.hostname}:81`
  let socket: WebSocket | null = null
  let reconexao: number | null = null
  let encerrado = false

  const conectar = () => {
    if (encerrado) return
    onStatus('connecting')
    socket = new WebSocket(websocketUrl)
    socket.onopen = () => onStatus('online')
    socket.onmessage = (event) => {
      try {
        const mensagem = JSON.parse(event.data) as StateMessage | AudioTelemetry
        if (mensagem.type === 'state') {
          onState?.(mensagem as DeviceState)
        } else {
          onData(mensagem as AudioTelemetry)
        }
      } catch { /* ignora quadros inválidos */ }
    }
    socket.onclose = () => {
      socket = null
      if (!encerrado) {
        onStatus('offline')
        reconexao = window.setTimeout(conectar, 2000)
      }
    }
    socket.onerror = () => socket?.close()
  }
  conectar()
  return () => {
    encerrado = true
    if (reconexao !== null) window.clearTimeout(reconexao)
    socket?.close()
  }
}
