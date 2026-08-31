import { useEffect, useMemo, useRef, useState } from 'react'
import {
  Activity, Antenna, Check, CircleAlert, Clock3, ExternalLink, Gauge,
  HardDriveDownload, LoaderCircle, Radio, RotateCcw, Save,
  Search, Settings2, Signal, SlidersHorizontal, Volume2, VolumeX, Wifi,
  X, Zap,
} from 'lucide-react'
import * as api from './api'
import type { DeviceState, Measurement, RdsSource, ScanResult, Settings } from './types'

const sourceLabels: Record<RdsSource, string> = {
  manual: 'Manual', frase: 'Frase salva', hora: 'Hora', data: 'Data',
  data_hora: 'Data e hora', modelo: 'Modelo',
}

const emptyScan: ScanResult = {
  running: false, finished: false, progress: 0,
  recommendedFrequencyKhz: 0, recommendedNoiseLevel: 0, measurements: [],
}

function mhz(khz: number) {
  return (khz / 100).toFixed(1).replace('.', ',')
}

function antennaPf(raw: number) {
  return (raw / 4).toFixed(2).replace('.', ',')
}

function cn(...classes: Array<string | false | null | undefined>) {
  return classes.filter(Boolean).join(' ')
}

function StatusDot({ ok, children }: { ok: boolean; children: React.ReactNode }) {
  return <span className={cn('status-dot', ok ? 'ok' : 'bad')}><i />{children}</span>
}

function Panel({ title, icon, className, children }: {
  title: string
  icon: React.ReactNode
  className?: string
  children: React.ReactNode
}) {
  return (
    <section className={cn('panel', className)}>
      <header className="panel-title">{icon}<h2>{title}</h2></header>
      {children}
    </section>
  )
}

function NumericField({ label, value, min, max, step = 1, suffix, onChange, onFocus }: {
  label: string; value: number; min: number; max: number; step?: number; suffix: string
  onChange: (value: number) => void
  onFocus?: () => void
}) {
  return (
    <label className="field numeric-field">
      <span>{label}</span>
      <span className="input-with-unit">
        <input type="number" value={value} min={min} max={max} step={step} onFocus={onFocus}
          onChange={(event) => onChange(Number(event.target.value))} />
        <b>{suffix}</b>
      </span>
    </label>
  )
}

function AntennaCapField({ value, effective, onChange }: {
  value: number
  effective: number
  onChange: (value: number) => void
}) {
  const automatic = value === 0
  return (
    <label className="field numeric-field antenna-field">
      <span>Capacitância da antena</span>
      <span className="antenna-controls">
        <select value={automatic ? 'auto' : 'manual'} onChange={(event) => {
          if (event.target.value === 'auto') onChange(0)
          else onChange(Math.max(1, Math.min(191, effective || 1)))
        }}>
          <option value="auto">AUTO</option>
          <option value="manual">Manual</option>
        </select>
        {!automatic && (
          <span className="input-with-unit">
            <input type="number" value={value / 4} min={0.25} max={47.75} step={0.25}
              onChange={(event) => onChange(Math.round(Number(event.target.value) * 4))} />
            <b>pF</b>
          </span>
        )}
      </span>
    </label>
  )
}

function VuMeter({ level, asq, overmodulation }: { level: number; asq: number; overmodulation: boolean }) {
  const normalized = Math.max(0, Math.min(100, ((level + 60) / 60) * 100))
  const asqHex = `0x${asq.toString(16).toUpperCase().padStart(2, '0')}`
  const escalaPreenchimento = normalized > 0 ? `${10000 / normalized}% 100%` : '100% 100%'
  return (
    <div className="vu" aria-label={`Nível de áudio ${level} dBFS`}>
      <div className="vu-reading"><span>Nível instantâneo da entrada</span><strong>{level} <small>dBFS</small></strong></div>
      <div className="vu-scale"><span>-60</span><span>-40</span><span>-20</span><span>-12</span><span>-6</span><span>-3</span><span>0</span></div>
      <div className={cn('vu-track', overmodulation && 'overmodulating')}>
        <div className="vu-fill" style={{ width: `${normalized}%`, backgroundSize: escalaPreenchimento }} />
        <div className="vu-peak" style={{ left: `calc(${normalized}% - 1px)` }} />
      </div>
      <div className="vu-meta">
        <span>Estado ASQ {asqHex} · atualização 4 Hz</span>
        <strong className={overmodulation ? 'danger-text' : ''}>{overmodulation ? 'CORTE / sobremodulação' : 'Nível sem corte'}</strong>
      </div>
    </div>
  )
}

function ScanChart({ measurements, minKhz, maxKhz }: {
  measurements: Measurement[]
  minKhz: number
  maxKhz: number
}) {
  if (!measurements.length) return <div className="chart-empty">A varredura preencherá o espectro de ruído.</div>
  const width = 640
  const height = 132
  const maxNoise = Math.max(60, ...measurements.map((item) => item.noiseLevel))
  const points = measurements.map((item, index) => {
    const x = (index / Math.max(1, measurements.length - 1)) * width
    const y = height - (item.noiseLevel / maxNoise) * (height - 10)
    return `${x.toFixed(1)},${y.toFixed(1)}`
  }).join(' ')
  const span = maxKhz - minKhz
  const labels = [0, 1, 2, 3, 4].map((part) => {
    const khz = minKhz + (span * part) / 4
    return khz === maxKhz ? `${mhz(khz)} MHz` : mhz(khz)
  })
  return (
    <div className="scan-chart">
      <svg viewBox={`0 0 ${width} ${height}`} role="img" aria-label="Nível de ruído por frequência">
        {[0, 1, 2, 3].map((line) => <line key={line} x1="0" x2={width} y1={line * 40 + 6} y2={line * 40 + 6} className="grid-line" />)}
        <polyline points={points} className="noise-line" />
      </svg>
      <div className="chart-axis">{labels.map((label) => <span key={label}>{label}</span>)}</div>
    </div>
  )
}

function PhraseDialog({ phrases, onClose, onSave }: {
  phrases: string[]; onClose: () => void; onSave: (phrases: string[]) => Promise<void>
}) {
  const [draft, setDraft] = useState(phrases)
  const [saving, setSaving] = useState(false)
  const update = (index: number, value: string) => setDraft((current) => current.map((item, i) => i === index ? value : item))
  return (
    <div className="dialog-backdrop" role="presentation" onMouseDown={onClose}>
      <div className="dialog" role="dialog" aria-modal="true" aria-label="Gerenciar frases RDS" onMouseDown={(event) => event.stopPropagation()}>
        <header><div><span className="eyebrow">RDS</span><h2>Frases salvas</h2></div><button className="icon-button" onClick={onClose} aria-label="Fechar"><X size={19} /></button></header>
        <p>Até 12 frases de 32 caracteres. As credenciais Wi‑Fi nunca passam por esta lista.</p>
        <div className="phrase-list">
          {draft.map((phrase, index) => (
            <div className="phrase-row" key={index}>
              <span>{String(index + 1).padStart(2, '0')}</span>
              <input value={phrase} maxLength={32} onChange={(event) => update(index, event.target.value)} />
              <button className="icon-button" onClick={() => setDraft((current) => current.filter((_, i) => i !== index))} aria-label={`Excluir frase ${index + 1}`}><X size={16} /></button>
            </div>
          ))}
        </div>
        {draft.length < 12 && <button className="button ghost" onClick={() => setDraft((current) => [...current, ''])}>Adicionar frase</button>}
        <footer><button className="button secondary" onClick={onClose}>Cancelar</button><button className="button primary" disabled={saving} onClick={async () => { setSaving(true); await onSave(draft.filter((item) => item.trim())); setSaving(false); onClose() }}><Save size={16} /> Salvar frases</button></footer>
      </div>
    </div>
  )
}

export default function App() {
  const [device, setDevice] = useState<DeviceState | null>(null)
  const [draft, setDraft] = useState<Settings | null>(null)
  const [scan, setScan] = useState<ScanResult>(emptyScan)
  const [phrases, setPhrases] = useState<string[]>([])
  const [phraseDialog, setPhraseDialog] = useState(false)
  const [audio, setAudio] = useState({ level: -60, asq: 0, overmodulation: false })
  const [audioReadingEnabled, setAudioReadingEnabled] = useState(false)
  const [clock, setClock] = useState(new Date())
  const [busy, setBusy] = useState<string | null>(null)
  const [notice, setNotice] = useState<{ kind: 'ok' | 'error'; text: string } | null>(null)
  const [connectionStatus, setConnectionStatus] = useState<api.ConnectionStatus>('connecting')
  const scanCarregadoDoDispositivo = useRef(false)
  const ultimoUptimeMs = useRef<number | null>(null)
  const connectionStatusRef = useRef<api.ConnectionStatus>('connecting')
  const falhaConexaoNotificada = useRef(false)
  const usuarioEditandoRef = useRef(false)
  const aplicacaoAtivaRef = useRef(true)
  const audioReadingEnabledRef = useRef(false)
  const filaAjusteFrequenciaRef = useRef<Promise<void>>(Promise.resolve())

  const notify = (kind: 'ok' | 'error', text: string) => {
    setNotice({ kind, text })
    window.setTimeout(() => setNotice(null), 3500)
  }

  const atualizarConexao = (status: api.ConnectionStatus) => {
    connectionStatusRef.current = status
    setConnectionStatus(status)
  }

  const incorporarEstado = (next: DeviceState) => {
    if (ultimoUptimeMs.current !== null
        && next.system.uptimeMs < ultimoUptimeMs.current) {
      notify('ok', 'ESP32 reiniciado e reconectado')
    }
    ultimoUptimeMs.current = next.system.uptimeMs
    setDevice(next)
    if (!usuarioEditandoRef.current) setDraft(next.desired)
    if (!next.system.scanRunning && !next.system.scanFinished) {
      scanCarregadoDoDispositivo.current = false
      setScan(emptyScan)
    } else if (next.system.scanFinished
        && !scanCarregadoDoDispositivo.current) {
      scanCarregadoDoDispositivo.current = true
      void api.getScan().then((resultado) => {
        if (aplicacaoAtivaRef.current) setScan(resultado)
      }).catch(() => {
        scanCarregadoDoDispositivo.current = false
      })
    }
  }

  useEffect(() => {
    let active = true
    aplicacaoAtivaRef.current = true
    const load = async () => {
      try {
        const next = await api.getState()
        if (!active) return
        falhaConexaoNotificada.current = false
        atualizarConexao('online')
        incorporarEstado(next)
      } catch (error) {
        if (active) {
          atualizarConexao('offline')
          if (!falhaConexaoNotificada.current) {
            falhaConexaoNotificada.current = true
            notify('error', error instanceof Error ? error.message : 'Dispositivo indisponível')
          }
        }
      }
    }
    void load()
    void api.getPhrases().then((items) => active && setPhrases(items))
    const poll = window.setInterval(load, 2500)
    const timer = window.setInterval(() => setClock(new Date()), 1000)
    return () => { active = false; aplicacaoAtivaRef.current = false; window.clearInterval(poll); window.clearInterval(timer) }
  }, [])

  useEffect(() => {
    let active = true
    const unsubscribe = api.subscribeAudio(
      (item) => {
        if (!active || !audioReadingEnabledRef.current) return
        setAudio({
          level: item.levelDbfs,
          asq: item.asq,
          overmodulation: item.overmodulation,
        })
        setDevice((current) => current ? {
          ...current,
          applied: {
            ...current.applied,
            onAir: item.onAir,
            audioLevelDbfs: item.levelDbfs,
            asq: item.asq,
          },
        } : current)
      },
      (status) => {
        if (!active) return
        atualizarConexao(status)
        if (status === 'online') {
          void api.getState().then((next) => {
            if (active) incorporarEstado(next)
          })
          if (audioReadingEnabledRef.current && !api.usingMock) {
            void api.setAudioMonitoring(true)
          }
        }
      },
      (next) => { if (active) incorporarEstado(next) },
    )
    return () => {
      active = false
      unsubscribe()
    }
  }, [])

  useEffect(() => {
    audioReadingEnabledRef.current = audioReadingEnabled
    if (!audioReadingEnabled) {
      setAudio({ level: -60, asq: 0, overmodulation: false })
    }
    if (!api.usingMock) void api.setAudioMonitoring(audioReadingEnabled)
  }, [audioReadingEnabled])

  useEffect(() => {
    if (!device?.system.scanRunning && !scan.running) return
    const timer = window.setInterval(async () => {
      try {
        const next = await api.getScan()
        setScan(next)
        if (next.finished) scanCarregadoDoDispositivo.current = true
        setDevice((current) => current ? { ...current, system: { ...current.system, scanRunning: next.running, scanFinished: next.finished, scanProgress: next.progress } } : current)
      } catch { /* próxima atualização tenta novamente */ }
    }, 450)
    return () => window.clearInterval(timer)
  }, [device?.system.scanRunning, scan.running])

  const run = async (name: string, task: () => Promise<DeviceState | void>, success: string) => {
    setBusy(name)
    try {
      const next = await task()
      if (next) {
        usuarioEditandoRef.current = false
        setDevice(next)
        setDraft(next.desired)
      }
      notify('ok', success)
    } catch (error) {
      notify('error', error instanceof Error ? error.message : 'Não foi possível concluir a ação')
    } finally { setBusy(null) }
  }

  const rdsPreview = useMemo(() => {
    if (!draft) return ''
    const date = new Intl.DateTimeFormat('pt-BR').format(clock)
    const time = new Intl.DateTimeFormat('pt-BR', { hour: '2-digit', minute: '2-digit' }).format(clock)
    if (draft.rdsSource === 'hora') return time
    if (draft.rdsSource === 'data') return date
    if (draft.rdsSource === 'data_hora') return `${date} ${time}`
    if (draft.rdsSource === 'modelo') return draft.rdsTemplate.replaceAll('{hora}', time).replaceAll('{data}', date).slice(0, 32)
    return draft.rdsText.trim()
  }, [draft, clock])

  if (!device || !draft) {
    return <main className="boot"><Radio size={34} /><LoaderCircle className="spin" /><span>Conectando ao painel do transmissor…</span></main>
  }

  const topFrequencies = [...scan.measurements].sort((a, b) => a.noiseLevel - b.noiseLevel).slice(0, 5)
  const antennaPending = draft.antennaCap !== 0
    && draft.antennaCap !== device.applied.antennaCap
  const hasPendingRf = draft.frequencyKhz !== device.applied.frequencyKhz
    || draft.powerDbuv !== device.applied.powerDbuv
    || antennaPending
  const freqMinMhz = device.system.frequencyMinKhz / 100
  const freqMaxMhz = device.system.frequencyMaxKhz / 100
  const freqStepMhz = device.system.frequencyStepKhz / 100
  const set = <K extends keyof Settings>(key: K, value: Settings[K]) => {
    usuarioEditandoRef.current = true
    setDraft((current) => current ? { ...current, [key]: value } : current)
  }
  const ajustarFrequencia = (valorMhz: number) => {
    const frequenciaKhz = Math.round(valorMhz * 100)
    set('frequencyKhz', frequenciaKhz)
    if (frequenciaKhz < device.system.frequencyMinKhz
        || frequenciaKhz > device.system.frequencyMaxKhz
        || frequenciaKhz % device.system.frequencyStepKhz !== 0) {
      return
    }

    filaAjusteFrequenciaRef.current = filaAjusteFrequenciaRef.current
      .catch(() => undefined)
      .then(async () => {
        const next = await api.previewFrequency(frequenciaKhz)
        if (aplicacaoAtivaRef.current) setDevice(next)
      })
      .catch((error) => {
        notify('error', error instanceof Error ? error.message : 'Não foi possível ajustar a frequência')
      })
  }
  const aplicarAjustes = async () => {
    await filaAjusteFrequenciaRef.current
    return api.applySettings(draft)
  }
  const conectado = api.usingMock || connectionStatus === 'online'
  const noAr = conectado && device.applied.onAir
  const eventoSi4713 = {
    none: 'Nenhuma interrupção registrada',
    read_error: 'Interrupção detectada; falha ao consultar a causa',
    overmodulation: 'Sobremodulação detectada',
    audio_high: 'Nível de áudio cruzou o limite alto',
    audio_low: 'Nível de áudio cruzou o limite baixo',
    asq: 'Evento ASQ detectado',
  }[device.system.si4713LastInterrupt]

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand"><Antenna size={25} /><div><strong>Transmissor FM</strong><span>Si4713 · ESP32-S3</span></div></div>
        <div className="top-status">
          {api.usingMock && <span className="environment">SIMULAÇÃO</span>}
          <StatusDot ok={conectado}><Radio size={15} /> {conectado ? 'ESP online' : 'ESP sem contato'}</StatusDot>
          <StatusDot ok={conectado && device.system.wifiConnected}><Wifi size={15} /> Wi‑Fi</StatusDot>
          <StatusDot ok={conectado && device.system.si4713Available}><Zap size={15} /> Si4713</StatusDot>
          <span className="clock"><Clock3 size={15} /> {clock.toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit', second: '2-digit' })}</span>
        </div>
      </header>

      <main className="console">
        <section className={cn('broadcast-strip', noAr && 'on-air')}>
          <div className="air-state"><Radio size={31} /><div><span>Estado da transmissão</span><strong>{conectado ? (noAr ? 'NO AR' : 'FORA DO AR') : 'SEM CONEXÃO'}</strong></div></div>
          <div className="frequency"><strong>{mhz(device.applied.frequencyKhz)}</strong><span>MHz<br /><small>frequência aplicada</small></span></div>
          <div className="strip-metrics">
            <div><span>Potência</span><strong>{device.applied.powerDbuv} dBµV</strong></div>
            <div><span>Modo</span><strong>{draft.stereo ? 'Estéreo' : 'Mono'}</strong></div>
            <div><span>Pré‑ênfase</span><strong>{draft.preemphasisUs} µs</strong></div>
          </div>
        </section>

        <div className="primary-grid">
          <Panel title="RF / transmissão" icon={<SlidersHorizontal size={18} />} className="rf-panel">
            <div className="desired-heading"><span>Ajuste desejado</span><span>Aplicado</span></div>
            <div className="rf-fields">
              <NumericField label="Frequência" value={draft.frequencyKhz / 100} min={freqMinMhz} max={freqMaxMhz} step={freqStepMhz} suffix="MHz" onFocus={() => ajustarFrequencia(draft.frequencyKhz / 100)} onChange={ajustarFrequencia} />
              <span className={cn('applied-value', draft.frequencyKhz !== device.applied.frequencyKhz && 'pending')}>{mhz(device.applied.frequencyKhz)} MHz</span>
              <NumericField label="Potência" value={draft.powerDbuv} min={88} max={115} suffix="dBµV" onChange={(value) => set('powerDbuv', value)} />
              <span className={cn('applied-value', draft.powerDbuv !== device.applied.powerDbuv && 'pending')}>{device.applied.powerDbuv} dBµV</span>
              <AntennaCapField value={draft.antennaCap} effective={device.applied.antennaCap} onChange={(value) => set('antennaCap', value)} />
              <span className={cn('applied-value', antennaPending && 'pending')}>
                {draft.antennaCap === 0 && 'AUTO · '}{antennaPf(device.applied.antennaCap)} pF
              </span>
            </div>
            <div className="option-row">
              <fieldset><legend>Modo</legend><label><input type="radio" checked={draft.stereo} onChange={() => set('stereo', true)} /> Estéreo</label><label><input type="radio" checked={!draft.stereo} onChange={() => set('stereo', false)} /> Mono</label></fieldset>
              <fieldset><legend>Pré‑ênfase</legend><label><input type="radio" checked={draft.preemphasisUs === 50} onChange={() => set('preemphasisUs', 50)} /> 50 µs</label><label><input type="radio" checked={draft.preemphasisUs === 75} onChange={() => set('preemphasisUs', 75)} /> 75 µs</label></fieldset>
              <NumericField label="Desvio de áudio" value={draft.audioDeviationKhz} min={50} max={66} suffix="kHz" onChange={(value) => set('audioDeviationKhz', value)} />
            </div>
            <button className={cn('button full', hasPendingRf ? 'primary' : 'secondary')} disabled={busy !== null} onClick={() => run('apply', aplicarAjustes, 'Ajustes aplicados; frequência salva')}><Check size={17} /> Aplicar ajustes</button>
            <div className="tx-actions">
              <button className={cn('button tx-button', draft.txEnabled && 'stop')} disabled={busy !== null || !conectado} onClick={() => run('tx', () => api.setTransmission(!draft.txEnabled), draft.txEnabled ? 'Transmissão encerrada' : 'Transmissão solicitada')}>
                {draft.txEnabled ? <X size={18} /> : <Radio size={18} />} {draft.txEnabled ? 'Encerrar transmissão' : 'Iniciar transmissão'}
              </button>
              <button className="button secondary" onClick={() => { const next = { ...draft, muted: !draft.muted }; setDraft(next); void run('mute', () => api.applySettings(next), next.muted ? 'Áudio silenciado' : 'Áudio restaurado') }}>
                {draft.muted ? <VolumeX size={18} /> : <Volume2 size={18} />} {draft.muted ? 'Ativar áudio' : 'Silenciar áudio'}
              </button>
            </div>
            <button className="button secondary full rf-restart" disabled={busy !== null || !conectado || !draft.txEnabled} onClick={() => run('restart-rf', api.restartRf, 'Estado RF reaplicado; confirme a portadora no receptor')}><RotateCcw size={17} /> Reiniciar estágio RF</button>
          </Panel>

          <Panel title="Áudio" icon={<Activity size={18} />} className="audio-panel">
            <label className="switch-line"><span><strong>Leitura de áudio</strong><small>Leitura ASQ periódica por solicitação; desligada, só há consulta quando GP2 sinaliza um evento</small></span><input className="switch" type="checkbox" checked={audioReadingEnabled} onChange={(event) => setAudioReadingEnabled(event.target.checked)} /></label>
            {audioReadingEnabled ? (
              <VuMeter level={noAr ? audio.level : -60} asq={noAr ? audio.asq : 0} overmodulation={noAr && audio.overmodulation} />
            ) : (
              <div className="audio-disabled"><Activity size={22} /><span>Leitura de áudio desativada</span><small>Ative a chave acima para retomar o medidor de nível</small></div>
            )}
            <div className="audio-notes">
              <div><Gauge size={18} /><span><strong>Leitura contínua</strong>WebSocket dedicado, atualização a cada 250 ms{!audioReadingEnabled && ' · desativada'}</span></div>
              <div className={device.system.si4713InterruptPending ? 'danger-text' : ''}><Zap size={18} /><span><strong>GP2/INT · GPIO {device.system.si4713InterruptPin}</strong>{eventoSi4713}{device.system.si4713InterruptCount > 0 && ` · evento #${device.system.si4713InterruptCount}`}{device.system.si4713InterruptPending && ' · aguardando reconhecimento'}</span></div>
              {device.system.si4713InterruptPending && (
                <button className="button warning full" disabled={busy !== null} onClick={() => run('interrupt-ack', api.acknowledgeSi4713Interrupt, 'Alerta reconhecido; GP2 rearmado')}><Check size={17} /> Reconhecer e rearmar GP2</button>
              )}
              <div><CircleAlert size={18} /><span><strong>Leitura do Si4713</strong>A faixa vermelha começa em −12 dBFS; OVERMOD acende o contorno e o aviso de corte</span></div>
            </div>
          </Panel>

          <Panel title="RDS" icon={<Signal size={18} />} className="rds-panel">
            <label className="switch-line"><span><strong>RDS ativo</strong><small>PS e RadioText enviados pelo Si4713</small></span><input className="switch" type="checkbox" checked={draft.rdsEnabled} onChange={(event) => set('rdsEnabled', event.target.checked)} /></label>
            <div className="two-columns">
              <label className="field"><span>PS <small>{draft.rdsPs.trim().length}/8</small></span><input value={draft.rdsPs.trimEnd()} maxLength={8} onChange={(event) => set('rdsPs', event.target.value)} /></label>
              <label className="field"><span>PI hexadecimal</span><input value={draft.rdsPi.toString(16).toUpperCase().padStart(4, '0')} maxLength={4} onChange={(event) => set('rdsPi', Number.parseInt(event.target.value || '0', 16))} /></label>
            </div>
            <label className="field"><span>Fonte do RadioText</span><select value={draft.rdsSource} onChange={(event) => set('rdsSource', event.target.value as RdsSource)}>{Object.entries(sourceLabels).map(([value, label]) => <option key={value} value={value}>{label}</option>)}</select></label>
            {draft.rdsSource === 'frase' && <div className="phrase-select"><label className="field"><span>Frase salva</span><select value={draft.rdsText.trim()} onChange={(event) => set('rdsText', event.target.value)}>{phrases.map((phrase) => <option key={phrase}>{phrase}</option>)}</select></label><button className="icon-button bordered" onClick={() => setPhraseDialog(true)} aria-label="Gerenciar frases"><Settings2 size={18} /></button></div>}
            {(draft.rdsSource === 'manual' || draft.rdsSource === 'frase') && <label className="field"><span>RadioText <small>{draft.rdsText.trimEnd().length}/32</small></span><input value={draft.rdsText.trimEnd()} maxLength={32} onChange={(event) => set('rdsText', event.target.value)} /></label>}
            {draft.rdsSource === 'modelo' && <label className="field"><span>Modelo <small>tokens: {'{data}'} {'{hora}'}</small></span><input value={draft.rdsTemplate.trimEnd()} maxLength={32} onChange={(event) => set('rdsTemplate', event.target.value.toLowerCase())} /></label>}
            <div className="rds-preview"><span>Pré‑visualização ao vivo</span><strong>{rdsPreview || '—'}</strong></div>
            <button className="button primary full" disabled={busy !== null} onClick={() => run('rds', () => api.applySettings(draft), 'Configuração RDS aplicada')}><Check size={17} /> Aplicar RDS</button>
          </Panel>
        </div>

        <Panel title="Varredura de frequências livres" icon={<Search size={18} />} className="scan-panel">
          <div className="scan-layout">
            <div className="scan-control">
              <button className="button secondary full" disabled={scan.running || busy !== null} onClick={async () => { scanCarregadoDoDispositivo.current = false; await run('scan', () => api.startScan(), 'Varredura iniciada; a transmissão foi pausada'); setScan({ ...emptyScan, running: true }) }}><Search size={17} /> {scan.running ? 'Varrendo…' : 'Iniciar varredura'}</button>
              <p>A transmissão é pausada durante a leitura de {mhz(device.system.frequencyMinKhz)} a {mhz(device.system.frequencyMaxKhz)} MHz.</p>
              <div className="progress-label"><span>Progresso</span><strong>{scan.progress}%</strong></div>
              <div className="progress"><i style={{ width: `${scan.progress}%` }} /></div>
            </div>
            <div className="chart-wrap"><span className="subheading">Nível de ruído por frequência</span><ScanChart measurements={scan.measurements} minKhz={device.system.frequencyMinKhz} maxKhz={device.system.frequencyMaxKhz} /></div>
            <div className="ranking"><span className="subheading">Melhores canais</span>{topFrequencies.length ? <ol>{topFrequencies.map((item) => <li key={item.frequencyKhz}><strong>{mhz(item.frequencyKhz)} MHz</strong><span>ruído {item.noiseLevel}</span><button className="text-button" onClick={() => run('frequency', () => api.applyScannedFrequency(item.frequencyKhz), `${mhz(item.frequencyKhz)} MHz aplicada`)}>Aplicar</button></li>)}</ol> : <p className="muted">Resultados aparecerão aqui.</p>}</div>
            <aside className="recommendation"><span>Recomendada</span><strong>{scan.recommendedFrequencyKhz ? mhz(scan.recommendedFrequencyKhz) : '—'} <small>MHz</small></strong><p>{scan.recommendedFrequencyKhz ? `ruído ${scan.recommendedNoiseLevel}` : 'Aguardando varredura'}</p><button className="button primary full" disabled={!scan.finished || !scan.recommendedFrequencyKhz} onClick={() => run('frequency', () => api.applyScannedFrequency(scan.recommendedFrequencyKhz), `${mhz(scan.recommendedFrequencyKhz)} MHz aplicada diretamente`)}><Check size={17} /> Aplicar frequência</button></aside>
          </div>
        </Panel>

        <section className="system-strip">
          <header className="panel-title"><HardDriveDownload size={18} /><h2>Sistema / rede</h2></header>
          <div className="system-item"><Wifi size={20} /><span>Wi‑Fi<strong>{device.system.wifiConnected ? 'Conectado' : 'Desconectado'}</strong><small>{device.system.ip}</small></span></div>
          <div className="system-item"><Zap size={20} /><span>Si4713<strong>{!device.system.si4713Available ? (device.system.recovering ? 'Recuperando' : 'Indisponível') : 'Operacional'}</strong><small>{device.system.recoveries} rec. · {device.system.rfStateMismatches} divergências RF</small></span></div>
          <div className="system-item"><Clock3 size={20} /><span>Hora / NTP<strong>{device.system.timeValid ? 'Sincronizada' : 'Aguardando rede'}</strong><small>{clock.toLocaleDateString('pt-BR')}</small></span></div>
          <div className="system-item"><Settings2 size={20} /><span>Firmware<strong>v{device.system.firmwareVersion}</strong><small>ESP32-S3 N16R8</small></span></div>
          <div className="system-actions">
            <button className="button secondary" onClick={() => run('wifi', () => api.openWifiPortal(), 'Portal Wi‑Fi aberto em 192.168.4.1')}><ExternalLink size={16} /> Abrir portal Wi‑Fi</button>
            <button className="button secondary" onClick={() => run('save', () => api.saveSettings(), 'Configuração salva no dispositivo')}><Save size={16} /> Salvar no dispositivo</button>
            <button className="button warning" onClick={() => run('defaults', () => api.restoreDefaults(), 'Padrões restaurados; transmissão desligada')}><RotateCcw size={16} /> Restaurar padrões</button>
          </div>
        </section>
      </main>

      {notice && <div className={cn('toast', notice.kind)}>{notice.kind === 'ok' ? <Check size={18} /> : <CircleAlert size={18} />}{notice.text}</div>}
      {phraseDialog && <PhraseDialog phrases={phrases} onClose={() => setPhraseDialog(false)} onSave={async (items) => setPhrases(await api.putPhrases(items))} />}
    </div>
  )
}
