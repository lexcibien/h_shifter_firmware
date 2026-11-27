use std::{
    io::{ErrorKind, Read, Write},
    sync::atomic::{AtomicBool, Ordering},
    time::{Duration, Instant},
};

use serialport::{SerialPort, SerialPortType, UsbPortInfo};

use super::{ConnectionError, Port};
use crate::protocol::{Command, FRAME_SIZE, MAGIC, ProtocolError, decode_response, encode_request};

const USB_VENDOR_ID: u16 = 0x2e8a;
const USB_PRODUCT_ID: u16 = 0x0003;
const BAUD_RATE: u32 = 115_200;
const IO_TIMEOUT: Duration = Duration::from_millis(100);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(4);

pub(super) fn ports() -> Result<Vec<Port>, ConnectionError> {
    let mut ports: Vec<_> = serialport::available_ports()?
        .into_iter()
        .filter_map(|port| {
            // IOKit lists both callout and dial-in names for each USB serial device.
            #[cfg(target_os = "macos")]
            if !port.port_name.starts_with("/dev/cu.") {
                return None;
            }
            let SerialPortType::UsbPort(usb) = port.port_type else {
                return None;
            };
            configuration_port(&usb).then_some(Port {
                name: port.port_name,
                serial_number: usb.serial_number,
            })
        })
        .collect();
    ports.sort_unstable_by(|a, b| a.name.cmp(&b.name));
    Ok(ports)
}

fn configuration_port(usb: &UsbPortInfo) -> bool {
    // Config CDC starts at interface 2 or 4. Older firmware starts at 3 or 5.
    // Some platforms report the adjacent data interface instead.
    usb.vid == USB_VENDOR_ID
        && usb.pid == USB_PRODUCT_ID
        && usb
            .interface
            .is_none_or(|interface| (2..=6).contains(&interface))
}

pub(super) struct Serial {
    port: Box<dyn SerialPort>,
    transaction: u16,
    frames: FrameBuffer,
}

impl Serial {
    pub(super) fn open(name: &str) -> Result<Self, ConnectionError> {
        let mut port = serialport::new(name, BAUD_RATE)
            .timeout(IO_TIMEOUT)
            .open()?;
        port.write_data_terminal_ready(true)?;
        port.clear(serialport::ClearBuffer::All)?;
        Ok(Self {
            port,
            transaction: 0,
            frames: FrameBuffer::default(),
        })
    }

    pub(super) fn request(
        &mut self,
        command: Command,
        payload: &[u8],
        stopped: &AtomicBool,
    ) -> Result<Vec<u8>, ConnectionError> {
        let deadline = Instant::now() + REQUEST_TIMEOUT;
        self.transaction = self.transaction.wrapping_add(1);
        let transaction = self.transaction;
        let frame = encode_request(command, transaction, payload)?;
        if stopped.load(Ordering::Relaxed) {
            return Err(ConnectionError::Disconnected);
        }
        self.port.write_all(&frame)?;

        let mut read = [0; 128];
        let mut invalid = None;
        loop {
            if stopped.load(Ordering::Relaxed) {
                return Err(ConnectionError::Disconnected);
            }
            while let Some(candidate) = self.frames.next() {
                match decode_response(candidate, command, transaction) {
                    Ok(payload) => {
                        let payload = payload.to_vec();
                        self.frames.consume(FRAME_SIZE);
                        return Ok(payload);
                    }
                    Err(
                        error @ (ProtocolError::Header
                        | ProtocolError::Crc
                        | ProtocolError::PayloadSize
                        | ProtocolError::Transaction),
                    ) => {
                        let bytes = if matches!(error, ProtocolError::Header | ProtocolError::Crc) {
                            1
                        } else {
                            FRAME_SIZE
                        };
                        self.frames.consume(bytes);
                        invalid = Some(error);
                    }
                    Err(error) => {
                        self.frames.consume(FRAME_SIZE);
                        return Err(error.into());
                    }
                }
            }
            if Instant::now() >= deadline {
                return Err(invalid.map_or(ConnectionError::Timeout, ConnectionError::from));
            }
            match self.port.read(&mut read) {
                Ok(0) => return Err(ConnectionError::Disconnected),
                Ok(count) => self.frames.push(&read[..count]),
                Err(error)
                    if matches!(
                        error.kind(),
                        ErrorKind::Interrupted | ErrorKind::WouldBlock | ErrorKind::TimedOut
                    ) => {}
                Err(error) => return Err(error.into()),
            }
        }
    }
}

#[derive(Default)]
struct FrameBuffer {
    bytes: Vec<u8>,
}

impl FrameBuffer {
    fn push(&mut self, bytes: &[u8]) {
        self.bytes.extend_from_slice(bytes);
    }

    fn next(&mut self) -> Option<&[u8]> {
        match self
            .bytes
            .windows(MAGIC.len())
            .position(|pair| pair == MAGIC)
        {
            Some(start) => self.consume(start),
            None => {
                let keep = usize::from(self.bytes.last() == Some(&MAGIC[0]));
                self.consume(self.bytes.len() - keep);
            }
        }
        self.bytes.get(..FRAME_SIZE)
    }

    fn consume(&mut self, count: usize) {
        self.bytes.drain(..count);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn discovery_should_match_configuration_interfaces() {
        let mut usb = UsbPortInfo {
            vid: USB_VENDOR_ID,
            pid: USB_PRODUCT_ID,
            serial_number: None,
            manufacturer: None,
            product: None,
            interface: Some(0),
        };
        assert!(!configuration_port(&usb));
        usb.interface = Some(1);
        assert!(!configuration_port(&usb));
        for interface in 2..=6 {
            usb.interface = Some(interface);
            assert!(configuration_port(&usb));
        }
        usb.interface = None;
        assert!(configuration_port(&usb));
    }

    #[test]
    fn frame_buffer_should_keep_a_split_header_and_return_multiple_frames() {
        let first = encode_request(Command::GetInfo, 1, &[]).unwrap();
        let second = encode_request(Command::GetStatus, 2, &[]).unwrap();
        let mut buffer = FrameBuffer::default();
        buffer.push(&[0, b'H']);
        assert!(buffer.next().is_none());
        buffer.push(&first[1..40]);
        assert!(buffer.next().is_none());
        buffer.push(&[&first[40..], second.as_slice()].concat());
        assert_eq!(buffer.next(), Some(first.as_slice()));
        buffer.consume(FRAME_SIZE);
        assert_eq!(buffer.next(), Some(second.as_slice()));
    }

    #[test]
    fn frame_buffer_should_find_the_next_header_after_a_corrupted_candidate() {
        let frame = encode_request(Command::GetStatus, 1, &[]).unwrap();
        let mut buffer = FrameBuffer::default();
        buffer.push(b"HS noise");
        buffer.push(&frame);
        assert_eq!(buffer.next().map(|bytes| bytes[2]), Some(b' '));
        buffer.consume(1);
        assert_eq!(buffer.next(), Some(frame.as_slice()));
    }

    #[cfg(unix)]
    #[test]
    fn serial_request_should_read_a_fragmented_reply_after_a_stale_transaction() {
        use std::{sync::mpsc, thread};

        use crate::protocol::response_fixture;

        let (mut device, host) = serialport::TTYPort::pair().unwrap();
        device.set_timeout(Duration::from_secs(1)).unwrap();
        let (finished, completion) = mpsc::channel();
        thread::scope(|scope| {
            let emulator = scope.spawn(move || {
                let mut request = [0; FRAME_SIZE];
                device.read_exact(&mut request).unwrap();
                assert_eq!(request, encode_request(Command::GetStatus, 1, &[]).unwrap());

                let old = response_fixture(Command::GetStatus, 0, 0, &[0]);
                let current = response_fixture(Command::GetStatus, 1, 0, &[1, 2, 3]);
                device.write_all(b"noise").unwrap();
                device.write_all(&old).unwrap();
                device.write_all(&current[..1]).unwrap();
                thread::sleep(Duration::from_millis(20));
                device.write_all(&current[1..]).unwrap();
                completion.recv_timeout(REQUEST_TIMEOUT).unwrap();
            });
            let mut serial = Serial {
                port: Box::new(host),
                transaction: 0,
                frames: FrameBuffer::default(),
            };
            let reply = serial.request(Command::GetStatus, &[], &AtomicBool::new(false));
            let _ = finished.send(());
            emulator.join().unwrap();
            assert_eq!(reply.unwrap(), [1, 2, 3]);
        });
    }
}
