"""
Generates chips/eeprom-24c256.chip.wasm directly from the WebAssembly
bytecode specification matching chips/eeprom-24c256.chip.c (Wokwi Custom
Chip I2C API v1), so no external clang-wasm32/wasi-sdk installation is
required.
"""

import os

def uleb128(val: int) -> bytes:
    out = bytearray()
    while True:
        b = val & 0x7F
        val >>= 7
        if val != 0:
            out.append(b | 0x80)
        else:
            out.append(b)
            break
    return bytes(out)

def sleb128(val: int) -> bytes:
    out = bytearray()
    more = True
    while more:
        b = val & 0x7F
        val >>= 7
        if (val == 0 and (b & 0x40) == 0) or (val == -1 and (b & 0x40) != 0):
            more = False
        else:
            b |= 0x80
        out.append(b)
    return bytes(out)

def wasm_str(s: str) -> bytes:
    raw = s.encode("utf-8")
    return uleb128(len(raw)) + raw

def section(sec_id: int, payload: bytes) -> bytes:
    return bytes([sec_id]) + uleb128(len(payload)) + payload

def vec(items: list[bytes]) -> bytes:
    return uleb128(len(items)) + b"".join(items)

def i32_const(v: int) -> bytes:
    return b"\x41" + sleb128(v)

def i32_load(offset: int = 0) -> bytes:
    return b"\x28" + uleb128(2) + uleb128(offset)

def i32_load8_u(offset: int = 0) -> bytes:
    return b"\x2d" + uleb128(0) + uleb128(offset)

def i32_store(offset: int = 0) -> bytes:
    return b"\x36" + uleb128(2) + uleb128(offset)

def i32_store8(offset: int = 0) -> bytes:
    return b"\x3a" + uleb128(0) + uleb128(offset)

def local_get(idx: int) -> bytes:
    return b"\x20" + uleb128(idx)

def local_set(idx: int) -> bytes:
    return b"\x21" + uleb128(idx)

def call(func_idx: int) -> bytes:
    return b"\x10" + uleb128(func_idx)

I32 = 0x7F
FUNC = 0x60

def functype(params: list[int], results: list[int]) -> bytes:
    return bytes([FUNC, len(params)] + params + [len(results)] + results)

# 1. Type section
types = [
    functype([I32, I32], [I32]),       # 0: (i32, i32) -> i32  [pinInit, on_i2c_write]
    functype([I32], [I32]),            # 1: (i32) -> i32       [i2cInit, on_i2c_read]
    functype([], [I32]),               # 2: () -> i32          [__wokwi_api_version_1]
    functype([], []),                  # 3: () -> void         [chipInit]
    functype([I32, I32, I32], [I32]),  # 4: (i32,i32,i32)->i32 [on_i2c_connect]
    functype([I32], []),               # 5: (i32) -> void      [on_i2c_disconnect]
]
sec_type = section(1, vec(types))

# 2. Import section
imports = [
    wasm_str("env") + wasm_str("pinInit") + bytes([0x00, 0]),  # func 0
    wasm_str("env") + wasm_str("i2cInit") + bytes([0x00, 1]),  # func 1
]
sec_import = section(2, vec(imports))

# 3. Function section (funcs 2..7)
func_types = [2, 3, 4, 1, 0, 5]
sec_func = section(3, uleb128(len(func_types)) + bytes(func_types))

# 4. Table section (table 0: funcref, min 5, max 5)
sec_table = section(4, uleb128(1) + bytes([0x70, 0x01, 5, 5]))

# 5. Memory section (memory 0: min 2 pages = 128KB)
sec_memory = section(5, uleb128(1) + bytes([0x00, 2]))

# 7. Export section
exports = [
    wasm_str("memory") + bytes([0x02, 0]),
    wasm_str("__indirect_function_table") + bytes([0x01, 0]),
    wasm_str("__wokwi_api_version_1") + bytes([0x00, 2]),
    wasm_str("chipInit") + bytes([0x00, 3]),
]
sec_export = section(7, vec(exports))

# 9. Element section (table 0, offset 1 -> funcs [4, 5, 6, 7])
elem_entry = (
    bytes([0x00])
    + i32_const(1)
    + b"\x0b"
    + uleb128(4)
    + bytes([4, 5, 6, 7])
)
sec_elem = section(9, uleb128(1) + elem_entry)

# 10. Code section
def make_func(locals_i32_count: int, body: bytes) -> bytes:
    if locals_i32_count == 0:
        loc = uleb128(0)
    else:
        loc = uleb128(1) + uleb128(locals_i32_count) + bytes([I32])
    code = loc + body + b"\x0b"
    return uleb128(len(code)) + code

# Func 2: __wokwi_api_version_1() -> 1
f2 = make_func(0, i32_const(1))

# Func 3: chipInit()
# Memory map:
#   0x0010: "SCL\0"
#   0x0018: "SDA\0"
#   0x0020: i2c_config_t (64 bytes)
#   0x0100: addr (u32)
#   0x0104: pending_addr_hi (u32)
#   0x0108: write_byte_index (u32)
#   0x1000..0x8FFF: mem[32768]
f3_body = (
    # config.user_data = 0x0100
    i32_const(0x0020) + i32_const(0x0100) + i32_store(0)
    # config.address = 0x50
    + i32_const(0x0020) + i32_const(0x50) + i32_store(4)
    # config.scl = pinInit("SCL", INPUT_PULLUP=2)
    + i32_const(0x0020) + i32_const(0x0010) + i32_const(2) + call(0) + i32_store(8)
    # config.sda = pinInit("SDA", INPUT_PULLUP=2)
    + i32_const(0x0020) + i32_const(0x0018) + i32_const(2) + call(0) + i32_store(12)
    # config.connect = 1
    + i32_const(0x0020) + i32_const(1) + i32_store(16)
    # config.read = 2
    + i32_const(0x0020) + i32_const(2) + i32_store(20)
    # config.write = 3
    + i32_const(0x0020) + i32_const(3) + i32_store(24)
    # config.disconnect = 4
    + i32_const(0x0020) + i32_const(4) + i32_store(28)
    # i2cInit(&config)
    + i32_const(0x0020) + call(1) + b"\x1a"  # drop
)
f3 = make_func(0, f3_body)

# Func 4: on_i2c_connect(user_data, address, read) -> i32
# if (!read) write_byte_index = 0; return 1;
f4_body = (
    local_get(2)
    + b"\x45"              # i32.eqz
    + b"\x04\x40"          # if (void)
    + i32_const(0x0108)
    + i32_const(0)
    + i32_store(0)
    + b"\x0b"              # end if
    + i32_const(1)
)
f4 = make_func(0, f4_body)

# Func 5: on_i2c_read(user_data) -> i32
# local 1 = current addr; local 2 = byte read
f5_body = (
    i32_const(0x0100) + i32_load(0) + i32_const(0x7FFF) + b"\x71"  # i32.and
    + local_set(1)
    + local_get(1) + i32_load8_u(0x1000)
    + local_set(2)
    + i32_const(0x0100)
    + local_get(1) + i32_const(1) + b"\x6a" + i32_const(0x7FFF) + b"\x71"
    + i32_store(0)
    + local_get(2)
)
f5 = make_func(2, f5_body)

# Func 6: on_i2c_write(user_data, data) -> i32
# local 2 = write_byte_index, local 3 = addr
f6_body = (
    i32_const(0x0108) + i32_load(0) + local_set(2)
    # if (write_byte_index == 0)
    + local_get(2) + b"\x45"  # i32.eqz
    + b"\x04\x40"             # if
        + i32_const(0x0104)
        + local_get(1) + i32_const(0xFF) + b"\x71"
        + i32_store(0)
        + i32_const(0x0108) + i32_const(1) + i32_store(0)
    + b"\x05"                 # else
        # if (write_byte_index == 1)
        + local_get(2) + i32_const(1) + b"\x46"  # i32.eq
        + b"\x04\x40"         # if
            + i32_const(0x0100)
            + i32_const(0x0104) + i32_load(0) + i32_const(8) + b"\x74"  # shl 8
            + local_get(1) + i32_const(0xFF) + b"\x71"
            + b"\x72"         # i32.or
            + i32_const(0x7FFF) + b"\x71"
            + i32_store(0)
            + i32_const(0x0108) + i32_const(2) + i32_store(0)
        + b"\x05"             # else (write_byte_index >= 2: data byte)
            + i32_const(0x0100) + i32_load(0) + i32_const(0x7FFF) + b"\x71"
            + local_set(3)
            + local_get(3)
            + local_get(1)
            + i32_store8(0x1000)
            + i32_const(0x0100)
            + local_get(3) + i32_const(1) + b"\x6a" + i32_const(0x7FFF) + b"\x71"
            + i32_store(0)
        + b"\x0b"             # end inner if
    + b"\x0b"                 # end outer if
    + i32_const(1)
)
f6 = make_func(2, f6_body)

# Func 7: on_i2c_disconnect(user_data) -> void
f7 = make_func(0, b"")

sec_code = section(10, vec([f2, f3, f4, f5, f6, f7]))

# 11. Data section
def data_seg(offset: int, raw: bytes) -> bytes:
    return bytes([0x00]) + i32_const(offset) + b"\x0b" + uleb128(len(raw)) + raw

sec_data = section(11, vec([
    data_seg(0x0010, b"SCL\x00"),
    data_seg(0x0018, b"SDA\x00"),
]))

wasm_bytes = (
    b"\x00asm\x01\x00\x00\x00"
    + sec_type
    + sec_import
    + sec_func
    + sec_table
    + sec_memory
    + sec_export
    + sec_elem
    + sec_code
    + sec_data
)

out_path = os.path.join(os.path.dirname(__file__), "eeprom-24c256.chip.wasm")
with open(out_path, "wb") as f:
    f.write(wasm_bytes)

print(f"Wrote {len(wasm_bytes)} bytes to {out_path}")
