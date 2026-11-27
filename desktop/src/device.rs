use thiserror::Error;

pub(crate) const SCHEMA_VERSION: u8 = 1;
pub(crate) const MIN_GEARS: u8 = 4;
pub(crate) const DEBOUNCE_MS: std::ops::RangeInclusive<u16> = 5..=50;
pub(crate) const SEQUENTIAL_HOLD_MS: std::ops::RangeInclusive<u16> = 250..=3000;
pub(crate) const HANDLE_THRESHOLD: std::ops::RangeInclusive<u16> = 500..=4000;
pub(crate) const MAX_RGB: u32 = 0xff_ffff;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum Project {
    V1,
    V2,
}

impl Project {
    pub(crate) fn label(self) -> &'static str {
        match self {
            Self::V1 => "V1",
            Self::V2 => "V2",
        }
    }

    pub(crate) fn unlock_instruction(self) -> &'static str {
        match self {
            Self::V1 => "Segure o botão sequencial e o switch direito por 1 segundo.",
            Self::V2 => "Coloque em sequencial e segure o switch direito por 1 segundo.",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct Capabilities(pub(crate) u16);

impl Capabilities {
    pub(crate) const INITIAL_MODE: u16 = 1 << 0;
    pub(crate) const SEQUENTIAL_HOLD: u16 = 1 << 1;
    pub(crate) const REVERSE: u16 = 1 << 2;
    pub(crate) const HANDLE: u16 = 1 << 3;
    pub(crate) const COLORS: u16 = 1 << 4;
    pub(crate) const DEBOUNCE: u16 = 1 << 5;

    pub(crate) fn has(self, capability: u16) -> bool {
        self.0 & capability != 0
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct DeviceInfo {
    pub(crate) project: Project,
    pub(crate) max_gears: u8,
    pub(crate) capabilities: Capabilities,
    pub(crate) version: [u8; 3],
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct DeviceConfig {
    pub(crate) forward_gears: u8,
    pub(crate) start_sequential: bool,
    pub(crate) reverse_enabled: bool,
    pub(crate) debounce_ms: u16,
    pub(crate) sequential_hold_ms: u16,
    pub(crate) handle_threshold: u16,
    pub(crate) h_color: u32,
    pub(crate) sequential_color: u32,
}

#[derive(Debug, Error)]
pub(crate) enum ConfigError {
    #[error("Quantidade de marchas fora do limite do dispositivo.")]
    Gears,
    #[error("O dispositivo controla o modo inicial pela chave física.")]
    InitialMode,
    #[error("Debounce fora do intervalo de 5 a 50 ms.")]
    Debounce,
    #[error("Tempo de acionamento sequencial fora do intervalo de 250 a 3000 ms.")]
    SequentialHold,
    #[error("Limiar da manopla fora do intervalo de 500 a 4000.")]
    HandleThreshold,
    #[error("As cores dos LEDs devem usar RGB de 24 bits.")]
    Color,
}

impl DeviceConfig {
    pub(crate) fn validate(&self, info: DeviceInfo) -> Result<(), ConfigError> {
        if !(MIN_GEARS..=info.max_gears).contains(&self.forward_gears) {
            return Err(ConfigError::Gears);
        }
        if self.start_sequential && !info.capabilities.has(Capabilities::INITIAL_MODE) {
            return Err(ConfigError::InitialMode);
        }
        if !DEBOUNCE_MS.contains(&self.debounce_ms) {
            return Err(ConfigError::Debounce);
        }
        if !SEQUENTIAL_HOLD_MS.contains(&self.sequential_hold_ms) {
            return Err(ConfigError::SequentialHold);
        }
        if !HANDLE_THRESHOLD.contains(&self.handle_threshold) {
            return Err(ConfigError::HandleThreshold);
        }
        if self.h_color > MAX_RGB || self.sequential_color > MAX_RGB {
            return Err(ConfigError::Color);
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum ConfigSource {
    Defaults,
    Flash,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct ConfigSnapshot {
    pub(crate) config: DeviceConfig,
    pub(crate) generation: u32,
    pub(crate) source: ConfigSource,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct DeviceStatus {
    pub(crate) unlocked: bool,
    pub(crate) sequential: bool,
    pub(crate) handle_connected: bool,
    pub(crate) adc: u16,
    pub(crate) inputs: u16,
    pub(crate) hid: u16,
}
