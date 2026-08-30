export type RdsSource = 'manual' | 'frase' | 'hora' | 'data' | 'data_hora' | 'modelo'

export interface Settings {
  frequencyKhz: number
  powerDbuv: number
  antennaCap: number
  txEnabled: boolean
  stereo: boolean
  preemphasisUs: 50 | 75
  audioDeviationKhz: number
  muted: boolean
  rdsEnabled: boolean
  rdsPi: number
  rdsPs: string
  rdsText: string
  rdsTemplate: string
  rdsSource: RdsSource
}

export interface AppliedState {
  onAir: boolean
  frequencyKhz: number
  powerDbuv: number
  antennaCap: number
  audioLevelDbfs: number
  asq: number
}

export interface SystemState {
  si4713Available: boolean
  recovering: boolean
  recoveries: number
  i2cCommunicationFailures: number
  rfStateMismatches: number
  scanRunning: boolean
  scanFinished: boolean
  scanProgress: number
  wifiConnected: boolean
  wifiPortalActive: boolean
  ip: string
  timeValid: boolean
  uptimeMs: number
  firmwareVersion: string
}

export interface DeviceState {
  desired: Settings
  applied: AppliedState
  system: SystemState
}

export interface StateMessage extends DeviceState {
  type: 'state'
}

export interface Measurement {
  frequencyKhz: number
  noiseLevel: number
}

export interface ScanResult {
  running: boolean
  finished: boolean
  progress: number
  recommendedFrequencyKhz: number
  recommendedNoiseLevel: number
  measurements: Measurement[]
}

export interface AudioTelemetry {
  type: 'audio'
  sequence: number
  timestampMs: number
  levelDbfs: number
  asq: number
  overmodulation: boolean
  onAir: boolean
}
