use thiserror::Error;

use crate::device::{
    Capabilities, ConfigSnapshot, ConfigSource, DeviceConfig, DeviceInfo, DeviceStatus, MIN_GEARS,
    Project, SCHEMA_VERSION,
};

pub(crate) const FRAME_SIZE: usize = 64;
const PAYLOAD_OFFSET: usize = 8;
const CRC_OFFSET: usize = 60;
const MAX_PAYLOAD: usize = CRC_OFFSET - PAYLOAD_OFFSET;
const VERSION: u8 = 1;
const CONFIG_SIZE: usize = 20;
pub(crate) const MAGIC: [u8; 2] = *b"HS";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub(super) enum Command {
    GetInfo = 1,
    GetConfig = 2,
    SaveConfig = 3,
    RestoreDefaults = 4,
    GetStatus = 5,
    RedetectHandle = 6,
    Reboot = 7,
}

#[derive(Debug, Error, PartialEq, Eq)]
pub(crate) enum ProtocolError {
    #[error("Tamanho de quadro inválido.")]
    FrameSize,
    #[error("Cabeçalho de quadro inválido.")]
    Header,
    #[error("Versão do protocolo incompatível.")]
    Version,
    #[error("CRC da resposta inválido.")]
    Crc,
    #[error("Tamanho de payload inválido.")]
    PayloadSize,
    #[error("Resposta de outro comando ou transação.")]
    Transaction,
    #[error("Schema de configuração incompatível.")]
    Schema,
    #[error("Dados inválidos na resposta do dispositivo.")]
    InvalidData,
    #[error("O firmware rejeitou o quadro.")]
    BadFrame,
    #[error("Comando não suportado pelo firmware.")]
    Unsupported,
    #[error("Configuração bloqueada pelo dispositivo.")]
    Locked,
    #[error("Configuração rejeitada pelo dispositivo.")]
    InvalidConfig,
    #[error("Falha ao gravar na flash.")]
    Flash,
    #[error("Dispositivo ocupado.")]
    Busy,
    #[error("O dispositivo retornou o status {0}.")]
    UnknownStatus(u8),
}

pub(super) fn encode_request(
    command: Command,
    transaction: u16,
    payload: &[u8],
) -> Result<[u8; FRAME_SIZE], ProtocolError> {
    if payload.len() > MAX_PAYLOAD {
        return Err(ProtocolError::PayloadSize);
    }
    let mut frame = [0; FRAME_SIZE];
    frame[..2].copy_from_slice(&MAGIC);
    frame[2] = VERSION;
    frame[3] = command as u8;
    frame[4..6].copy_from_slice(&transaction.to_le_bytes());
    frame[6] = payload.len() as u8;
    frame[PAYLOAD_OFFSET..PAYLOAD_OFFSET + payload.len()].copy_from_slice(payload);
    let crc = crc32fast::hash(&frame[..CRC_OFFSET]);
    frame[CRC_OFFSET..].copy_from_slice(&crc.to_le_bytes());
    Ok(frame)
}

pub(super) fn decode_response(
    frame: &[u8],
    command: Command,
    transaction: u16,
) -> Result<&[u8], ProtocolError> {
    let frame: &[u8; FRAME_SIZE] = frame.try_into().map_err(|_| ProtocolError::FrameSize)?;
    if frame[..2] != MAGIC || frame[7] != 0 {
        return Err(ProtocolError::Header);
    }
    if crc32fast::hash(&frame[..CRC_OFFSET]).to_le_bytes() != frame[CRC_OFFSET..] {
        return Err(ProtocolError::Crc);
    }
    if frame[2] != VERSION {
        return Err(ProtocolError::Version);
    }
    if frame[3] != command as u8 | 0x80 || frame[4..6] != transaction.to_le_bytes() {
        return Err(ProtocolError::Transaction);
    }
    let length = usize::from(frame[6]);
    if !(1..=MAX_PAYLOAD).contains(&length) {
        return Err(ProtocolError::PayloadSize);
    }
    match frame[PAYLOAD_OFFSET] {
        0 => Ok(&frame[PAYLOAD_OFFSET + 1..PAYLOAD_OFFSET + length]),
        1 => Err(ProtocolError::BadFrame),
        2 => Err(ProtocolError::Version),
        3 => Err(ProtocolError::Unsupported),
        4 => Err(ProtocolError::Locked),
        5 => Err(ProtocolError::InvalidConfig),
        6 => Err(ProtocolError::Flash),
        7 => Err(ProtocolError::Busy),
        other => Err(ProtocolError::UnknownStatus(other)),
    }
}

pub(super) fn decode_info(payload: &[u8]) -> Result<DeviceInfo, ProtocolError> {
    let &[
        schema,
        project,
        max_gears,
        cap_lo,
        cap_hi,
        major,
        minor,
        patch,
    ] = payload
    else {
        return Err(ProtocolError::PayloadSize);
    };
    if schema != SCHEMA_VERSION {
        return Err(ProtocolError::Schema);
    }
    let project = match project {
        1 => Project::V1,
        2 => Project::V2,
        _ => return Err(ProtocolError::InvalidData),
    };
    if max_gears < MIN_GEARS {
        return Err(ProtocolError::InvalidData);
    }
    Ok(DeviceInfo {
        project,
        max_gears,
        capabilities: Capabilities(u16::from_le_bytes([cap_lo, cap_hi])),
        version: [major, minor, patch],
    })
}

pub(super) fn encode_config(config: DeviceConfig) -> [u8; CONFIG_SIZE] {
    let mut bytes = [0; CONFIG_SIZE];
    bytes[0] = SCHEMA_VERSION;
    bytes[1] = config.forward_gears;
    bytes[2] = u8::from(config.start_sequential);
    bytes[3] = u8::from(config.reverse_enabled);
    bytes[4..6].copy_from_slice(&config.debounce_ms.to_le_bytes());
    bytes[6..8].copy_from_slice(&config.sequential_hold_ms.to_le_bytes());
    bytes[8..10].copy_from_slice(&config.handle_threshold.to_le_bytes());
    bytes[12..16].copy_from_slice(&config.h_color.to_le_bytes());
    bytes[16..20].copy_from_slice(&config.sequential_color.to_le_bytes());
    bytes
}

pub(super) fn decode_config(payload: &[u8]) -> Result<DeviceConfig, ProtocolError> {
    let &[
        schema,
        forward_gears,
        initial,
        reverse,
        d0,
        d1,
        s0,
        s1,
        t0,
        t1,
        r0,
        r1,
        h0,
        h1,
        h2,
        h3,
        c0,
        c1,
        c2,
        c3,
    ] = payload
    else {
        return Err(ProtocolError::PayloadSize);
    };
    if schema != SCHEMA_VERSION {
        return Err(ProtocolError::Schema);
    }
    if initial > 1 || reverse > 1 || r0 != 0 || r1 != 0 {
        return Err(ProtocolError::InvalidData);
    }
    Ok(DeviceConfig {
        forward_gears,
        start_sequential: initial != 0,
        reverse_enabled: reverse != 0,
        debounce_ms: u16::from_le_bytes([d0, d1]),
        sequential_hold_ms: u16::from_le_bytes([s0, s1]),
        handle_threshold: u16::from_le_bytes([t0, t1]),
        h_color: u32::from_le_bytes([h0, h1, h2, h3]),
        sequential_color: u32::from_le_bytes([c0, c1, c2, c3]),
    })
}

pub(super) fn decode_snapshot(payload: &[u8]) -> Result<ConfigSnapshot, ProtocolError> {
    let [config @ .., g0, g1, g2, g3, source] = payload else {
        return Err(ProtocolError::PayloadSize);
    };
    let source = match source {
        0 => ConfigSource::Defaults,
        1 => ConfigSource::Flash,
        _ => return Err(ProtocolError::InvalidData),
    };
    Ok(ConfigSnapshot {
        config: decode_config(config)?,
        generation: u32::from_le_bytes([*g0, *g1, *g2, *g3]),
        source,
    })
}

pub(super) fn decode_generation(payload: &[u8]) -> Result<u32, ProtocolError> {
    let bytes = payload.try_into().map_err(|_| ProtocolError::PayloadSize)?;
    Ok(u32::from_le_bytes(bytes))
}

pub(super) fn decode_status(payload: &[u8]) -> Result<DeviceStatus, ProtocolError> {
    let &[flags, a0, a1, i0, i1, h0, h1] = payload else {
        return Err(ProtocolError::PayloadSize);
    };
    Ok(DeviceStatus {
        unlocked: flags & 1 != 0,
        sequential: flags & 2 != 0,
        handle_connected: flags & 4 != 0,
        adc: u16::from_le_bytes([a0, a1]),
        inputs: u16::from_le_bytes([i0, i1]),
        hid: u16::from_le_bytes([h0, h1]),
    })
}

#[cfg(test)]
pub(super) fn response_fixture(
    command: Command,
    transaction: u16,
    status: u8,
    payload: &[u8],
) -> [u8; FRAME_SIZE] {
    let bytes: Vec<_> = std::iter::once(status)
        .chain(payload.iter().copied())
        .collect();
    let mut frame = encode_request(command, transaction, &bytes).unwrap();
    frame[3] |= 0x80;
    let crc = crc32fast::hash(&frame[..CRC_OFFSET]);
    frame[CRC_OFFSET..].copy_from_slice(&crc.to_le_bytes());
    frame
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn save_request_should_match_the_v1_wire_format() {
        let expected = [
            0x48, 0x53, 0x01, 0x03, 0x34, 0x12, 0x14, 0x00, 0x01, 0x06, 0x01, 0x00, 0x16, 0x00,
            0xaa, 0x05, 0x80, 0x0c, 0x00, 0x00, 0x33, 0x22, 0x11, 0x00, 0xcc, 0xbb, 0xaa, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x55, 0x89, 0x17, 0xfb,
        ];
        let config = DeviceConfig {
            forward_gears: 6,
            start_sequential: true,
            reverse_enabled: false,
            debounce_ms: 22,
            sequential_hold_ms: 1450,
            handle_threshold: 3200,
            h_color: 0x11_2233,
            sequential_color: 0xaa_bbcc,
        };

        let frame = encode_request(Command::SaveConfig, 0x1234, &encode_config(config)).unwrap();
        assert_eq!(frame, expected);
    }

    #[test]
    fn info_response_should_reject_a_truncated_frame() {
        let response = response_fixture(Command::GetInfo, 1, 0, &[1, 1, 6, 63, 0, 1, 0, 0]);
        assert_eq!(
            decode_response(&response[..63], Command::GetInfo, 1),
            Err(ProtocolError::FrameSize)
        );
    }

    #[test]
    fn info_response_should_reject_a_bad_crc() {
        let mut response = response_fixture(Command::GetInfo, 1, 0, &[1, 1, 6, 63, 0, 1, 0, 0]);
        response[20] ^= 1;
        assert_eq!(
            decode_response(&response, Command::GetInfo, 1),
            Err(ProtocolError::Crc)
        );
    }

    #[test]
    fn info_response_should_reject_an_old_transaction() {
        let response = response_fixture(Command::GetInfo, 1, 0, &[1, 1, 6, 63, 0, 1, 0, 0]);
        assert_eq!(
            decode_response(&response, Command::GetInfo, 2),
            Err(ProtocolError::Transaction)
        );
    }

    #[test]
    fn save_response_should_report_the_firmware_lock() {
        let response = response_fixture(Command::SaveConfig, 1, 4, &[]);
        assert_eq!(
            decode_response(&response, Command::SaveConfig, 1),
            Err(ProtocolError::Locked)
        );
    }

    #[test]
    fn config_response_should_reject_a_missing_source_byte() {
        assert_eq!(decode_snapshot(&[0; 24]), Err(ProtocolError::PayloadSize));
    }
}
