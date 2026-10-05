/**
 * Build a bootable 140K ProDOS-order game disk without external disk tools.
 * Input is cc65 AppleSingle (retains the BIN load address) + cc65 loader.system.
 * Boot blocks and PRODOS are copied unmodified from the official 2.4.3 disk.
 */
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { createHash } from "node:crypto";

const [input, loaderPath, output, bootPath] = process.argv.slice(2);
if (!bootPath) {
  console.error("Usage: node tools/make-disk.mjs FELIX loader.system felix.po ProDOS_2_4_3.po");
  process.exit(1);
}
const bootDisk = readFileSync(bootPath);
const officialHash = "398d333cb2ab92df9f8bb2cf64b946f2567116910eb8359cf4bdee5d4194f0fa";
if (createHash("sha256").update(bootDisk).digest("hex") !== officialHash)
  throw new Error("Boot source is not the verified official ProDOS 2.4.3 disk");

function extractProDOS() {
  for (let block = 2; block; block = bootDisk.readUInt16LE(block * 512 + 2)) {
    for (let i = block === 2 ? 1 : 0; i < 13; i++) {
      const p = block * 512 + 4 + i * 39;
      if (!bootDisk[p]) continue;
      const name = bootDisk.toString("ascii", p + 1, p + 1 + (bootDisk[p] & 15));
      if (name !== "PRODOS") continue;
      if (bootDisk[p] >> 4 !== 2) throw new Error("Expected sapling PRODOS file");
      const key = bootDisk.readUInt16LE(p + 17);
      const size = bootDisk.readUIntLE(p + 21, 3);
      const pieces = [];
      for (let n = 0; n < Math.ceil(size / 512); n++) {
        const b = bootDisk[key * 512 + n] | bootDisk[key * 512 + 256 + n] << 8;
        pieces.push(bootDisk.subarray(b * 512, (b + 1) * 512));
      }
      return Buffer.concat(pieces).subarray(0, size);
    }
  }
  throw new Error("Official boot disk has no PRODOS file");
}
const prodos = extractProDOS();
const appleSingle = readFileSync(input);
if (appleSingle.readUInt32BE(0) !== 0x00051600)
  throw new Error("Expected cc65 AppleSingle executable");
let data, aux, type;
for (let n = 0; n < appleSingle.readUInt16BE(24); n++) {
  const p = 26 + n * 12;
  const id = appleSingle.readUInt32BE(p);
  const start = appleSingle.readUInt32BE(p + 4);
  const length = appleSingle.readUInt32BE(p + 8);
  if (start + length > appleSingle.length) throw new Error("Invalid AppleSingle entry");
  if (id === 1) data = appleSingle.subarray(start, start + length);
  if (id === 11) {
    type = appleSingle.readUInt16BE(start + 2);
    aux = appleSingle.readUInt32BE(start + 4);
  }
}
if (!data || type !== 6 || aux !== 0x0803)
  throw new Error(`Unexpected executable metadata: type=${type} aux=${aux}`);

const disk = Buffer.alloc(280 * 512);
bootDisk.copy(disk, 0, 0, 1024);
const used = new Set([0, 1, 2, 3, 4, 5, 6]);
let nextBlock = 7;
const allocate = () => {
  if (nextBlock >= 280) throw new Error("Disk is full");
  used.add(nextBlock);
  return nextBlock++;
};
const u24 = (offset, value) => disk.writeUIntLE(value, offset, 3);
const nameAt = (offset, name, storage) => {
  if (!/^[A-Z][A-Z0-9.]{0,14}$/.test(name)) throw new Error("Invalid ProDOS name");
  disk[offset] = (storage << 4) | name.length;
  disk.write(name, offset + 1, "ascii");
};

// Four linked volume-directory blocks, thirteen 39-byte entries per block.
for (let block = 2; block <= 5; block++) {
  disk.writeUInt16LE(block === 2 ? 0 : block - 1, block * 512);
  disk.writeUInt16LE(block === 5 ? 0 : block + 1, block * 512 + 2);
}
const volume = 2 * 512 + 4;
nameAt(volume, "TYPER.FELIX", 15);
disk[volume + 30] = 0xC3;
disk[volume + 31] = 39;
disk[volume + 32] = 13;
disk.writeUInt16LE(6, volume + 35);
disk.writeUInt16LE(280, volume + 37);

let count = 0;
function addFile(name, bytes, fileType, loadAddress = 0) {
  const blocks = Math.ceil(bytes.length / 512);
  if (blocks === 0 || blocks > 256 || count >= 12)
    throw new Error("This writer supports 12 nonempty seedling/sapling files");
  const key = allocate();
  const sapling = blocks > 1;
  if (sapling) {
    for (let n = 0; n < blocks; n++) {
      const block = allocate();
      disk[key * 512 + n] = block & 255;
      disk[key * 512 + 256 + n] = block >> 8;
      bytes.copy(disk, block * 512, n * 512, Math.min((n + 1) * 512, bytes.length));
    }
  } else bytes.copy(disk, key * 512);
  const p = volume + (++count) * 39;
  nameAt(p, name, sapling ? 2 : 1);
  disk[p + 16] = fileType;
  disk.writeUInt16LE(key, p + 17);
  disk.writeUInt16LE(blocks + (sapling ? 1 : 0), p + 19);
  u24(p + 21, bytes.length);
  disk[p + 30] = 0xC3;
  disk.writeUInt16LE(loadAddress, p + 31);
  disk.writeUInt16LE(2, p + 37);
}

// The first .SYSTEM file auto-runs after ProDOS boots.
addFile("FELIX.SYSTEM", readFileSync(loaderPath), 0xFF);
addFile("FELIX", data, type, aux);
addFile("PRODOS", prodos, 0xFF);
addFile("READ.ME", Buffer.from(
  "TYPING WITH TYPER FELIX\r\r" +
  "BOOTABLE GAME DISK - PRODOS 2.4.3\r" +
  "Insert in drive 1 and cold boot your Apple IIe.\r" +
  "Felix starts automatically. No commands needed.\r\r" +
  "Gazette requires an 80-column card.\r" +
  "Use normal 1 MHz speed. CAPS LOCK off for Gazette.\r" +
  "TAB pauses, ESC returns, CTRL-S toggles sound.\r", "ascii"), 0x04);
disk.writeUInt16LE(count, volume + 33);
disk.fill(0xFF, 6 * 512, 6 * 512 + 35);
for (const block of used)
  disk[6 * 512 + (block >> 3)] &= ~(0x80 >> (block & 7));
writeFileSync(output, disk);
writeFileSync(join(dirname(output), "FELIX.bin"), data);
writeFileSync(join(dirname(output), "FELIX.SYSTEM"), readFileSync(loaderPath));
console.log(`${output}: ${disk.length} bytes; ${used.size}/280 blocks used`);
console.log(`FELIX: BIN, load $${aux.toString(16)}, ${data.length} bytes`);