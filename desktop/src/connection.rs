mod serial;

use std::{
    io::ErrorKind,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
        mpsc::{self, RecvTimeoutError},
    },
    time::{Duration, Instant},
};

use thiserror::Error;

use self::serial::Serial;
use crate::{
    device::{ConfigSnapshot, ConfigSource, DeviceConfig, DeviceInfo, DeviceStatus},
    protocol::{self, Command, ProtocolError},
};

const POLL_INTERVAL: Duration = Duration::from_millis(300);

#[derive(Clone, Debug, PartialEq, Eq)]
pub(crate) struct Port {
    pub(crate) name: String,
    pub(crate) serial_number: Option<String>,
}

pub(crate) fn ports() -> Result<Vec<Port>, ConnectionError> {
    serial::ports()
}

#[derive(Clone, Copy)]
pub(crate) enum Operation {
    Save(DeviceConfig),
    Restore,
    Redetect,
    Reboot,
}

impl Operation {
    pub(crate) fn failure(self) -> &'static str {
        match self {
            Self::Save(_) => "Não foi possível confirmar a gravação.",
            Self::Restore => "Não foi possível confirmar a restauração.",
            Self::Redetect => "Não foi possível reverificar a manopla.",
            Self::Reboot => "Não foi possível confirmar o reinício.",
        }
    }
}

pub(crate) enum Event {
    Connected {
        info: DeviceInfo,
        snapshot: ConfigSnapshot,
        status: DeviceStatus,
    },
    Status(DeviceStatus),
    Saved(ConfigSnapshot),
    Restored(ConfigSnapshot),
    Redetected(DeviceStatus),
    Rebooting,
    Failed {
        operation: Option<Operation>,
        error: ConnectionError,
        terminal: bool,
    },
}

#[derive(Debug, Error)]
pub(crate) enum ConnectionError {
    #[error("O dispositivo foi desconectado.")]
    Disconnected,
    #[error("O dispositivo não respondeu a tempo.")]
    Timeout,
    #[error("A porta selecionada não está disponível.")]
    PortUnavailable,
    #[error("Permissão negada para acessar a porta serial.")]
    PermissionDenied,
    #[error("Falha na comunicação: {0}")]
    Io(std::io::Error),
    #[error("Falha na porta serial: {0}")]
    Serial(serialport::Error),
    #[error(transparent)]
    Protocol(#[from] ProtocolError),
    #[error("O dispositivo retornou uma configuração inválida: {0}")]
    InvalidConfig(crate::device::ConfigError),
    #[error("A configuração lida na flash não corresponde à gravação solicitada.")]
    Verification,
    #[error("Falha ao reler a configuração. {0}")]
    Readback(Box<Self>),
}

impl ConnectionError {
    fn ends_session(&self) -> bool {
        match self {
            Self::Protocol(error) => !matches!(
                error,
                ProtocolError::BadFrame
                    | ProtocolError::Unsupported
                    | ProtocolError::Locked
                    | ProtocolError::InvalidConfig
                    | ProtocolError::Flash
                    | ProtocolError::Busy
                    | ProtocolError::UnknownStatus(_)
            ),
            _ => true,
        }
    }

    pub(crate) fn is_locked(&self) -> bool {
        matches!(self, Self::Protocol(ProtocolError::Locked))
    }
}

impl From<std::io::Error> for ConnectionError {
    fn from(error: std::io::Error) -> Self {
        match error.kind() {
            ErrorKind::PermissionDenied => Self::PermissionDenied,
            ErrorKind::NotFound => Self::PortUnavailable,
            ErrorKind::TimedOut => Self::Timeout,
            ErrorKind::BrokenPipe | ErrorKind::NotConnected | ErrorKind::UnexpectedEof => {
                Self::Disconnected
            }
            _ => Self::Io(error),
        }
    }
}

impl From<serialport::Error> for ConnectionError {
    fn from(error: serialport::Error) -> Self {
        match error.kind() {
            serialport::ErrorKind::NoDevice => Self::PortUnavailable,
            serialport::ErrorKind::Io(kind) => std::io::Error::new(kind, error).into(),
            _ => Self::Serial(error),
        }
    }
}

pub(crate) struct Session {
    commands: mpsc::Sender<Operation>,
    stopped: Arc<AtomicBool>,
}

impl Session {
    pub(crate) fn start(
        port: Port,
    ) -> Result<(Self, async_channel::Receiver<Event>), ConnectionError> {
        let (commands, pending) = mpsc::channel();
        let (updates, events) = async_channel::unbounded();
        let stopped = Arc::new(AtomicBool::new(false));
        let worker_stopped = Arc::clone(&stopped);
        std::thread::Builder::new()
            .name("daz-shifter-session".into())
            .spawn(move || {
                if let Err(error) = run(port, &worker_stopped, &pending, &updates) {
                    let _ = updates.send_blocking(Event::Failed {
                        operation: None,
                        error,
                        terminal: true,
                    });
                }
            })?;
        Ok((Self { commands, stopped }, events))
    }

    pub(crate) fn submit(&self, operation: Operation) -> Result<(), ConnectionError> {
        self.commands
            .send(operation)
            .map_err(|_| ConnectionError::Disconnected)
    }
}

impl Drop for Session {
    fn drop(&mut self) {
        self.stopped.store(true, Ordering::Relaxed);
    }
}

impl Serial {
    fn read_config(
        &mut self,
        info: DeviceInfo,
        stopped: &AtomicBool,
    ) -> Result<ConfigSnapshot, ConnectionError> {
        let snapshot =
            protocol::decode_snapshot(&self.request(Command::GetConfig, &[], stopped)?)?;
        snapshot
            .config
            .validate(info)
            .map_err(ConnectionError::InvalidConfig)?;
        Ok(snapshot)
    }

    fn read_status(&mut self, stopped: &AtomicBool) -> Result<DeviceStatus, ConnectionError> {
        Ok(protocol::decode_status(&self.request(
            Command::GetStatus,
            &[],
            stopped,
        )?)?)
    }

    fn perform(
        &mut self,
        operation: Operation,
        info: DeviceInfo,
        stopped: &AtomicBool,
    ) -> Result<Event, ConnectionError> {
        match operation {
            Operation::Save(config) => {
                let payload = protocol::encode_config(config);
                let generation = protocol::decode_generation(&self.request(
                    Command::SaveConfig,
                    &payload,
                    stopped,
                )?)?;
                let snapshot = self
                    .read_config(info, stopped)
                    .map_err(|error| ConnectionError::Readback(Box::new(error)))?;
                if snapshot.source != ConfigSource::Flash
                    || snapshot.generation != generation
                    || snapshot.config != config
                {
                    return Err(ConnectionError::Verification);
                }
                Ok(Event::Saved(snapshot))
            }
            Operation::Restore => {
                let generation = protocol::decode_generation(&self.request(
                    Command::RestoreDefaults,
                    &[],
                    stopped,
                )?)?;
                let snapshot = self
                    .read_config(info, stopped)
                    .map_err(|error| ConnectionError::Readback(Box::new(error)))?;
                if snapshot.source != ConfigSource::Flash || snapshot.generation != generation {
                    return Err(ConnectionError::Verification);
                }
                Ok(Event::Restored(snapshot))
            }
            Operation::Redetect => {
                let reply = self.request(Command::RedetectHandle, &[], stopped)?;
                if !matches!(reply.as_slice(), [0 | 1, _, _]) {
                    return Err(ProtocolError::InvalidData.into());
                }
                Ok(Event::Redetected(self.read_status(stopped)?))
            }
            Operation::Reboot => {
                if !self.request(Command::Reboot, &[], stopped)?.is_empty() {
                    return Err(ProtocolError::PayloadSize.into());
                }
                Ok(Event::Rebooting)
            }
        }
    }
}

fn run(
    port: Port,
    stopped: &AtomicBool,
    pending: &mpsc::Receiver<Operation>,
    updates: &async_channel::Sender<Event>,
) -> Result<(), ConnectionError> {
    let mut serial = Serial::open(&port.name)?;
    let info = protocol::decode_info(&serial.request(Command::GetInfo, &[], stopped)?)?;
    let snapshot = serial.read_config(info, stopped)?;
    let status = serial.read_status(stopped)?;
    if updates
        .send_blocking(Event::Connected {
            info,
            snapshot,
            status,
        })
        .is_err()
    {
        return Ok(());
    }

    let mut next_poll = Instant::now() + POLL_INTERVAL;
    while !stopped.load(Ordering::Relaxed) {
        let event = match pending.recv_timeout(next_poll.saturating_duration_since(Instant::now()))
        {
            Ok(operation) if !stopped.load(Ordering::Relaxed) => {
                match serial.perform(operation, info, stopped) {
                    Ok(event) => event,
                    Err(error) => {
                        let terminal = error.ends_session();
                        Event::Failed {
                            operation: Some(operation),
                            error,
                            terminal,
                        }
                    }
                }
            }
            Err(RecvTimeoutError::Timeout) => {
                let status = serial.read_status(stopped)?;
                next_poll = Instant::now() + POLL_INTERVAL;
                Event::Status(status)
            }
            _ => break,
        };
        let terminal = matches!(
            event,
            Event::Rebooting | Event::Failed { terminal: true, .. }
        );
        if updates.send_blocking(event).is_err() || terminal {
            break;
        }
    }
    Ok(())
}
