// Check the linked 6502 instructions, not just the C source. A previous cc65
// build silently omitted discarded volatile latch-reset expressions.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const binary = readFileSync("build/FELIX.bin");
const labels = readFileSync("build/felix.lbl", "utf8");
function bytes(name, size) {
  const match = labels.match(new RegExp(`^al ([0-9A-Fa-f]+) \\.${name}$`, "m"));
  assert(match, `Missing symbol ${name}`);
  const offset = parseInt(match[1], 16) - 0x0803;
  assert(offset >= 0 && offset + size <= binary.length);
  return [...binary.subarray(offset, offset + size)];
}
assert.deepEqual(bytes("_key_read", 17), [
  0xad, 0x00, 0xc0, // LDA keyboard latch
  0x10, 0x08,       // BPL no-new-key
  0x2c, 0x10, 0xc0, // BIT reset strobe (required side effect)
  0x29, 0x7f,       // AND ASCII
  0xa2, 0x00, 0x60, // LDX 0 / RTS
  0xa9, 0x00, 0xaa, 0x60 // LDA 0 / TAX / RTS
]);
assert.deepEqual(bytes("_key_flush", 4), [0x2c, 0x10, 0xc0, 0x60]);
assert.deepEqual(bytes("_key_held", 8), [0xad, 0x10, 0xc0, 0x29, 0x80, 0xa2, 0, 0x60]);
console.log("PASS: linked keyboard routines reset the strobe, ignore stale keys and read IIe key-held status.");