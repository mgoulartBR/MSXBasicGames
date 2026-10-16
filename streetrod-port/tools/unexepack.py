#!/usr/bin/env python3
"""Unpacker for Microsoft EXEPACK ('RB') executables. Usage: unexepack.py in.exe out.exe"""
import struct, sys

def unpack(d):
    mz = list(struct.unpack('<14H', d[:28]))
    hdr = mz[4] * 16
    cs, ip = mz[11], mz[10]
    base = hdr + cs * 16
    # EXEPACK header sits at CS:0000 (16 or 18 bytes)
    r_ip, r_cs, _, _, r_sp, r_ss, dest_len, skip = struct.unpack('<8H', d[base:base+16])
    sig = struct.unpack('<H', d[base+16:base+18])[0]
    if sig != 0x4252:
        # older 16-byte form: signature at +14
        r_ip, r_cs, _, _, r_sp, r_ss, dest_len = struct.unpack('<7H', d[base:base+14])
        sig = struct.unpack('<H', d[base+14:base+16])[0]
        assert sig == 0x4252, 'no RB signature'
        skip = 1
    print(f'real CS:IP={r_cs:04x}:{r_ip:04x} SS:SP={r_ss:04x}:{r_sp:04x} dest_len={dest_len:#x} paras')
    end = base            # compressed data ends right before the exepack header
    start = hdr
    buf = bytearray(d[start:end])
    # strip trailing 0xFF padding
    while buf and buf[-1] == 0xFF: buf.pop()
    out = bytearray()
    pos = len(buf)
    # decompress backwards: last byte is final command
    dst = bytearray()          # built in reverse
    while pos > 0:
        pos -= 1; cmd = buf[pos]
        pos -= 2; ln = buf[pos] | (buf[pos+1] << 8)
        op = cmd & 0xFE
        if op == 0xB0:          # fill
            pos -= 1; dst += bytes([buf[pos]]) * ln
        elif op == 0xB2:        # copy literal
            dst += bytes(reversed(buf[pos-ln:pos])); pos -= ln
        else:
            raise SystemExit(f'bad cmd {cmd:#x} at {pos}')
        if cmd & 1: break
    code = bytes(buf[:pos]) + bytes(reversed(dst))   # untouched prefix + expanded tail
    # relocation table follows the exepack stub
    return mz, code, (r_cs, r_ip, r_ss, r_sp), base

if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read()
    mz, code, regs, base = unpack(d)
    open(sys.argv[2], 'wb').write(code)
    print('unpacked image bytes:', len(code))
