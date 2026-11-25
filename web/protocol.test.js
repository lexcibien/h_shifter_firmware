import assert from "node:assert/strict";
import test from "node:test";
import { WebUsbClient, crc32, decodeConfig, decodeFrame, encodeConfig, encodeFrame } from "./protocol.js";

function responseFrame(command, transaction, payload = new Uint8Array()) {
  const response = encodeFrame(command, transaction, payload);
  response[3] |= 0x80;
  response[6] = payload.length + 1;
  response[8] = 0;
  response.set(payload, 9);
  new DataView(response.buffer).setUint32(60, crc32(response.subarray(0, 60)), true);
  return new DataView(response.buffer);
}

test("CRC-32 uses the IEEE reference vector", () => {
  assert.equal(crc32(new TextEncoder().encode("123456789")), 0xcbf43926);
});

test("frames preserve command, transaction, payload and validate CRC", () => {
  const request = encodeFrame(3, 0x1234, Uint8Array.of(1, 2, 3));
  request[3] = 0x83;
  request[6] = 4;
  request[8] = 0;
  request[9] = 1;
  request[10] = 2;
  request[11] = 3;
  new DataView(request.buffer).setUint32(60, crc32(request.subarray(0, 60)), true);
  assert.deepEqual([...decodeFrame(new DataView(request.buffer), 3, 0x1234)], [1, 2, 3]);
  request[20] ^= 1;
  assert.throws(() => decodeFrame(new DataView(request.buffer), 3, 0x1234), /inválida/);
});

test("configuration codec round-trips every public field", () => {
  const expected = {
    forwardGearCount: 6,
    defaultSequentialMode: true,
    reverseEnabled: false,
    debounceMs: 22,
    sequentialHoldMs: 1450,
    handleDetectThreshold: 3200,
    hModeColor: 0x112233,
    sequentialModeColor: 0xaabbcc,
  };
  assert.deepEqual(decodeConfig(encodeConfig(expected)), expected);
});

test("client ignores TinyUSB zero-length terminator before a response", async () => {
  const client = new WebUsbClient();
  const reads = [new DataView(new ArrayBuffer(0)), responseFrame(1, 1, Uint8Array.of(7, 8))];
  client.device = {
    opened: true,
    transferOut: async (_endpoint, frame) => ({ status: "ok", bytesWritten: frame.length }),
    transferIn: async () => ({ status: "ok", data: reads.shift() }),
  };

  assert.deepEqual([...await client.request(1)], [7, 8]);
  assert.equal(reads.length, 0);
});

test("a stalled WebUSB response times out, closes the device and cancels queued commands", async ({ mock }) => {
  mock.timers.enable({ apis: ["setTimeout"] });
  const client = new WebUsbClient();
  const reading = Promise.withResolvers();
  const close = mock.fn(async () => {});
  const transferOut = mock.fn(async (_endpoint, frame) => ({ status: "ok", bytesWritten: frame.length }));
  client.device = {
    opened: true,
    close,
    transferOut,
    transferIn() {
      reading.resolve();
      return new Promise(() => {});
    },
  };

  const save = assert.rejects(client.request(3), /câmbio não respondeu/);
  const status = assert.rejects(client.request(5), /Dispositivo desconectado/);
  await reading.promise;
  mock.timers.runAll();
  await Promise.all([save, status]);
  assert.equal(close.mock.callCount(), 1);
  assert.equal(transferOut.mock.callCount(), 1);
});
