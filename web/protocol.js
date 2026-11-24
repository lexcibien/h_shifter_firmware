export const USB_VENDOR_ID = 0x2e8a;
export const USB_PRODUCT_ID = 0x0003;

export const Command = Object.freeze({
  GET_INFO: 0x01,
  GET_CONFIG: 0x02,
  SAVE_CONFIG: 0x03,
  RESTORE_DEFAULTS: 0x04,
  GET_STATUS: 0x05,
  REDETECT_HANDLE: 0x06,
  REBOOT: 0x07,
});

const STATUS_NAMES = ["OK", "Quadro inválido", "Versão incompatível", "Comando não suportado", "Interface bloqueada", "Configuração inválida", "Falha ao gravar a flash", "Dispositivo ocupado"];
const FRAME_SIZE = 64;
const PAYLOAD_OFFSET = 8;
const CRC_OFFSET = 60;

export function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
    }
  }
  return (crc ^ 0xffffffff) >>> 0;
}

export function encodeFrame(command, transaction, payload = new Uint8Array()) {
  if (payload.length > 52) throw new RangeError("Payload WebUSB maior que 52 bytes");
  const frame = new Uint8Array(FRAME_SIZE);
  const view = new DataView(frame.buffer);
  frame[0] = 0x48;
  frame[1] = 0x53;
  frame[2] = 1;
  frame[3] = command;
  view.setUint16(4, transaction, true);
  frame[6] = payload.length;
  frame.set(payload, PAYLOAD_OFFSET);
  view.setUint32(CRC_OFFSET, crc32(frame.subarray(0, CRC_OFFSET)), true);
  return frame;
}

export function decodeFrame(buffer, expectedCommand, expectedTransaction) {
  const frame = new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);
  if (frame.length !== FRAME_SIZE) throw new Error("Resposta WebUSB incompleta");
  const view = new DataView(frame.buffer, frame.byteOffset, frame.byteLength);
  if (frame[0] !== 0x48 || frame[1] !== 0x53 || frame[2] !== 1 || frame[7] !== 0 ||
      view.getUint32(CRC_OFFSET, true) !== crc32(frame.subarray(0, CRC_OFFSET))) {
    throw new Error("Resposta WebUSB inválida");
  }
  if (frame[3] !== (expectedCommand | 0x80) || view.getUint16(4, true) !== expectedTransaction) {
    throw new Error("Resposta WebUSB fora de sequência");
  }
  const length = frame[6];
  if (length < 1 || length > 52) throw new Error("Tamanho de resposta inválido");
  const status = frame[PAYLOAD_OFFSET];
  if (status !== 0) throw new Error(STATUS_NAMES[status] ?? `Erro do dispositivo (${status})`);
  return frame.slice(PAYLOAD_OFFSET + 1, PAYLOAD_OFFSET + length);
}

export function encodeConfig(config) {
  const bytes = new Uint8Array(20);
  const view = new DataView(bytes.buffer);
  bytes[0] = 1;
  bytes[1] = config.forwardGearCount;
  bytes[2] = Number(config.defaultSequentialMode);
  bytes[3] = Number(config.reverseEnabled);
  view.setUint16(4, config.debounceMs, true);
  view.setUint16(6, config.sequentialHoldMs, true);
  view.setUint16(8, config.handleDetectThreshold, true);
  view.setUint32(12, config.hModeColor, true);
  view.setUint32(16, config.sequentialModeColor, true);
  return bytes;
}

export function decodeConfig(bytes) {
  if (bytes.length < 20 || bytes[0] !== 1) throw new Error("Schema de configuração incompatível");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    forwardGearCount: bytes[1],
    defaultSequentialMode: bytes[2] !== 0,
    reverseEnabled: bytes[3] !== 0,
    debounceMs: view.getUint16(4, true),
    sequentialHoldMs: view.getUint16(6, true),
    handleDetectThreshold: view.getUint16(8, true),
    hModeColor: view.getUint32(12, true),
    sequentialModeColor: view.getUint32(16, true),
  };
}

export class WebUsbClient {
  device = null;
  interfaceNumber = 0;
  endpointIn = 0;
  endpointOut = 0;
  transaction = 0;
  requestQueue = Promise.resolve();

  async connect(device) {
    this.device = device;
    if (!device.opened) await device.open();
    if (!device.configuration) await device.selectConfiguration(1);
    const candidate = device.configuration.interfaces.find(({ alternates }) =>
      alternates.some((alternate) => alternate.interfaceClass === 0xff && alternate.endpoints.length === 2));
    if (!candidate) throw new Error("Interface WebUSB não encontrada; atualize o firmware");
    const alternate = candidate.alternates.find((item) => item.interfaceClass === 0xff && item.endpoints.length === 2);
    this.interfaceNumber = candidate.interfaceNumber;
    this.endpointIn = alternate.endpoints.find(({ direction }) => direction === "in").endpointNumber;
    this.endpointOut = alternate.endpoints.find(({ direction }) => direction === "out").endpointNumber;
    await device.claimInterface(this.interfaceNumber);
  }

  request(command, payload = new Uint8Array()) {
    const operation = this.requestQueue.then(() => this.performRequest(command, payload));
    this.requestQueue = operation.catch(() => {});
    return operation;
  }

  async performRequest(command, payload) {
    this.transaction = (this.transaction + 1) & 0xffff;
    const frame = encodeFrame(command, this.transaction, payload);
    const sent = await this.device.transferOut(this.endpointOut, frame);
    if (sent.status !== "ok" || sent.bytesWritten !== frame.length) throw new Error("Falha ao enviar comando");

    // TinyUSB terminates a full-size bulk transfer with a zero-length packet.
    // Depending on timing, Chromium may return that terminator on the next
    // transferIn() instead of consuming it with the preceding 64-byte frame.
    // Discard at most one such packet before waiting for the current response.
    for (let attempt = 0; attempt < 2; attempt += 1) {
      const received = await this.device.transferIn(this.endpointIn, FRAME_SIZE);
      if (received.status !== "ok" || !received.data) throw new Error("Falha ao receber resposta");
      if (received.data.byteLength === 0 && attempt === 0) continue;
      return decodeFrame(received.data, command, this.transaction);
    }

    throw new Error("Resposta WebUSB não recebida");
  }

  async close() {
    if (!this.device?.opened) return;
    await this.device.releaseInterface(this.interfaceNumber).catch(() => {});
    await this.device.close();
  }
}
