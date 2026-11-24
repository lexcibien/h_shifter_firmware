import assert from "node:assert/strict";
import test from "node:test";
import { crc32, decodeConfig, decodeFrame, encodeConfig, encodeFrame } from "./protocol.js";

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
