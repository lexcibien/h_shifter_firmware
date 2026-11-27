use crate::{
    connection::Operation,
    device::{ConfigSnapshot, DeviceConfig, DeviceInfo, DeviceStatus},
};

#[derive(Default)]
pub(crate) enum State {
    #[default]
    Disconnected,
    Connecting,
    Connected(Connected),
}

impl State {
    pub(crate) fn connected(&self) -> Option<&Connected> {
        match self {
            Self::Connected(device) => Some(device),
            _ => None,
        }
    }

    pub(super) fn connected_mut(&mut self) -> Option<&mut Connected> {
        match self {
            Self::Connected(device) => Some(device),
            _ => None,
        }
    }
}

pub(crate) struct Connected {
    pub(crate) info: DeviceInfo,
    pub(crate) snapshot: ConfigSnapshot,
    pub(crate) draft: DeviceConfig,
    pub(crate) status: DeviceStatus,
    pub(crate) pending: Option<Operation>,
}

impl Connected {
    pub(super) fn confirm_save(&mut self, snapshot: ConfigSnapshot) {
        self.snapshot = snapshot;
        self.pending = None;
    }

    pub(crate) fn dirty(&self) -> bool {
        self.draft != self.snapshot.config
    }

    pub(crate) fn can_send(&self) -> bool {
        self.status.unlocked && self.pending.is_none()
    }
}

pub(crate) struct Notice {
    pub(crate) text: String,
    pub(crate) error: bool,
}

#[cfg(test)]
mod tests {
    use crate::device::{Capabilities, ConfigSource, Project};

    use super::*;

    #[test]
    fn confirmed_save_should_preserve_a_newer_draft() {
        let info = DeviceInfo {
            project: Project::V1,
            max_gears: 6,
            capabilities: Capabilities(Capabilities::INITIAL_MODE | Capabilities::SEQUENTIAL_HOLD),
            version: [1, 0, 0],
        };
        let snapshot = ConfigSnapshot {
            config: DeviceConfig {
                forward_gears: 6,
                start_sequential: false,
                reverse_enabled: true,
                debounce_ms: 15,
                sequential_hold_ms: 1000,
                handle_threshold: 3430,
                h_color: 0xff_8000,
                sequential_color: 0xff_00ff,
            },
            generation: 0,
            source: ConfigSource::Defaults,
        };
        let status = DeviceStatus {
            unlocked: true,
            sequential: false,
            handle_connected: false,
            adc: 0,
            inputs: 0,
            hid: 0,
        };
        let submitted = DeviceConfig {
            forward_gears: 5,
            ..snapshot.config
        };
        let newer = DeviceConfig {
            forward_gears: 4,
            ..snapshot.config
        };
        let mut device = Connected {
            info,
            snapshot,
            draft: newer,
            status,
            pending: Some(Operation::Save(submitted)),
        };

        device.confirm_save(ConfigSnapshot {
            config: submitted,
            generation: 1,
            source: ConfigSource::Flash,
        });

        assert_eq!(device.draft, newer);
        assert!(device.dirty());
    }
}
