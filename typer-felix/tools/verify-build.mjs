// Static build/disk integrity checks; this does not emulate the Apple IIe.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const disk = readFileSync("build/felix.po");
assert.equal(disk.length, 143360);
const bootSource = readFileSync("vendor/ProDOS_2_4_3.po");
assert.deepEqual(disk.subarray(0, 1024), bootSource.subarray(0, 1024));
assert.equal(disk[0], 1, "Disk II boot sector must load one initial sector");
const block = (n) => disk.subarray(n * 512, (n + 1) * 512);
const volume = block(2).subarray(4, 43);
assert.equal(volume[0] >> 4, 15);
assert.equal(volume[31], 39);
assert.equal(volume[32], 13);
assert.equal(volume.readUInt16LE(33), 4);
assert.equal(volume.readUInt16LE(35), 6);
assert.equal(volume.readUInt16LE(37), 280);
const allocated = new Set([0, 1, 2, 3, 4, 5, 6]);
const files = new Map();
for (let n = 1; n <= 4; n++) {
  const entry = block(2).subarray(4 + n * 39, 4 + (n + 1) * 39);
  const name = entry.toString("ascii", 1, 1 + (entry[0] & 15));
  const key = entry.readUInt16LE(17);
  const eof = entry.readUIntLE(21, 3);
  const blocks = [];
  assert(!allocated.has(key));
  allocated.add(key);
  if (entry[0] >> 4 === 1) blocks.push(block(key));
  else {
    assert.equal(entry[0] >> 4, 2);
    for (let i = 0; i < Math.ceil(eof / 512); i++) {
      const b = block(key)[i] | block(key)[i + 256] << 8;
      assert(b >= 7 && b < 280);
      assert(!allocated.has(b));
      allocated.add(b);
      blocks.push(block(b));
    }
  }
  assert.equal(entry.readUInt16LE(19), blocks.length + ((entry[0] >> 4) === 2 ? 1 : 0));
  assert.equal(entry.readUInt16LE(37), 2);
  files.set(name, { bytes: Buffer.concat(blocks).subarray(0, eof), entry });
}
assert.deepEqual(files.get("FELIX").bytes, readFileSync("build/FELIX.bin"));
assert.deepEqual(files.get("FELIX.SYSTEM").bytes, readFileSync("build/FELIX.SYSTEM"));
assert.equal(files.get("FELIX").entry[16], 6);
assert.equal(files.get("FELIX").entry.readUInt16LE(31), 0x0803);
assert.equal(files.get("FELIX.SYSTEM").entry[16], 0xFF);
assert.equal([...files.keys()].find(name => name.endsWith(".SYSTEM")), "FELIX.SYSTEM");
assert.equal(files.get("PRODOS").entry[16], 0xFF);
assert.equal(files.get("PRODOS").bytes.length, 17128);
// Independently reconstruct the official kernel for byte-for-byte comparison.
let sourceKernel;
for (let b = 2; b; b = bootSource.readUInt16LE(b * 512 + 2)) {
  for (let i = b === 2 ? 1 : 0; i < 13; i++) {
    const p = b * 512 + 4 + i * 39;
    if (bootSource.toString("ascii", p + 1, p + 1 + (bootSource[p] & 15)) !== "PRODOS") continue;
    const key = bootSource.readUInt16LE(p + 17) * 512;
    const length = bootSource.readUIntLE(p + 21, 3);
    sourceKernel = Buffer.alloc(length);
    for (let n = 0; n < Math.ceil(length / 512); n++) {
      const dataBlock = bootSource[key + n] + 256 * bootSource[key + n + 256];
      bootSource.copy(sourceKernel, n * 512, dataBlock * 512,
        dataBlock * 512 + Math.min(512, length - n * 512));
    }
  }
}
assert(sourceKernel);
assert.deepEqual(files.get("PRODOS").bytes, sourceKernel);
for (let b = 0; b < 280; b++)
  assert.equal(Boolean(block(6)[b >> 3] & (0x80 >> (b & 7))), !allocated.has(b));
const map = readFileSync("build/felix.map", "utf8");
const code = map.match(/^CODE\s+([0-9A-F]+)\s+([0-9A-F]+)/m);
const bss = map.match(/^BSS\s+([0-9A-F]+)\s+([0-9A-F]+)/m);
assert(code && bss);
assert.equal(parseInt(code[1], 16), 0x4000);
assert(parseInt(bss[2], 16) < 0x8E00, "BSS must not overlap the software stack");
console.log("PASS: official boot blocks/kernel, automatic FELIX.SYSTEM selection, disk directory, BIN/SYS metadata, file contents, allocation bitmap, HGR/stack separation.");