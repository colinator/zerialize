# ZERA (Zerialize Envelope + Arena) Protocol

`ZERA` is zerialize’s built-in, dependency-free binary protocol. It is designed for:

- A JSON-compatible dynamic data model (maps with string keys, arrays, primitive scalars, strings, blobs).
- Lazy / “touch only what you read” traversal.
- Zero-copy access to variable-sized payloads (strings, blobs) when the backing buffer is kept alive.
- A 16-byte–aligned arena so tensor payloads can be safely *viewed* as `T*` when alignment permits.
- A “checked reader” that validates offsets/lengths and rejects corrupt buffers.

Implementation: `include/zerialize/protocols/zera.hpp`  

## Data Model

ZERA’s model is the least-common-denominator across zerialize protocols:

- `null`
- `bool`
- `int64` (canonical signed integer)
- `uint64` (supported by the current implementation as an extension tag; see below)
- `float64` (canonical float)
- `string` (UTF-8 bytes, not null-terminated)
- `array` (ordered list of values)
- `object` (map from UTF-8 string keys → values)
- `blob` (bytes; represented as a typed array of `u8` with shape `[N]`)

### Notes on tensors

Zerialize’s tensor helpers (xtensor/Eigen) use the library’s generic “tensor triple” model:

`[dtype_code, shape, blob]`

ZERA stores the `blob` as a true byte payload in the arena (no base64).

## Buffer Layout (v1)

Each message is a single contiguous byte buffer:

`[Header][Envelope][Padding][Arena]`

The *Envelope* contains fixed-size value references and small structured payloads (arrays, objects, shapes).  
The *Arena* contains variable-sized payloads (strings, blobs/typed array data).

All multi-byte integers in ZERA are **little-endian**.

### Diagram

<svg xmlns="http://www.w3.org/2000/svg" viewBox="-24.75 -27.166 969.5 531.916" width="969.5" height="531.916">
  <rect x="-24.75" y="-27.166" width="969.5" height="531.916" fill="white" />
  <style>
    .mono { font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace; }
    .title { font-size: 18px; font-weight: 700; fill: #111827; }
    .sub { font-size: 12px; fill: #374151; }
    .muted { fill: #6b7280; }
    .card { fill: #ffffff; stroke: #111827; stroke-width: 1.5; }
    .bar { fill: #ffffff; stroke: #111827; stroke-width: 1.5; }
    .seg-h { fill: #e0f2fe; stroke: #111827; stroke-width: 1.2; }
    .seg-e { fill: #dcfce7; stroke: #111827; stroke-width: 1.2; }
    .seg-p { fill: #fef9c3; stroke: #111827; stroke-width: 1.2; }
    .seg-a { fill: #ede9fe; stroke: #111827; stroke-width: 1.2; }
    .note { font-size: 12px; fill: #111827; }
    .arrow { stroke: #111827; stroke-width: 1.4; fill: none; marker-end: url(#m); }
  </style>
  <defs>
    <marker id="m" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="#111827" />
    </marker>
  </defs>
  <text class="title" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial" y="10">ZERA v1: Envelope + Arena (single contiguous buffer)</text>
  <text class="sub muted" y="22" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Layout: [Header][Envelope][Envelope→Arena Pad][Arena]. Arena allocations are individually aligned.</text>
  <g transform="translate(0, 44)">
    <rect class="bar" x="0" y="0" width="920" height="66" rx="12" ry="12" />
    <rect class="seg-h" x="10" y="10" width="160" height="46" rx="10" ry="10" />
    <rect class="seg-e" x="180" y="10" width="380" height="46" rx="10" ry="10" />
    <rect class="seg-p" x="570" y="10" width="90" height="46" rx="10" ry="10" />
    <rect class="seg-a" x="670" y="10" width="240" height="46" rx="10" ry="10" />
    <text class="mono" x="26" y="38" style="font-size:13px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Header (20 B)</text>
    <text class="mono" x="310" y="38" style="font-size:13px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Envelope (env_size)</text>
    <text class="mono" x="593" y="38" style="font-size:13px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Pad</text>
    <text class="mono" x="744" y="38" style="font-size:13px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Arena (bytes)</text>
    <text class="sub muted" x="10" y="86" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">buffer start →</text>
    <text class="sub muted" x="858" y="86" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">… end</text>
  </g>
  <g transform="translate(0, 140)">
    <rect class="card" x="0" y="0" width="445" height="170" rx="12" ry="12" />
    <text class="title" x="18" y="28" style="font-size:16px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Header fields (20 B)</text>
    <text class="mono" x="18" y="56" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u32 magic   = 'ZENV'</text>
    <text class="mono" x="18" y="76" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u16 version = 1</text>
    <text class="mono" x="18" y="96" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u16 flags   (bit0 = little-endian)</text>
    <text class="mono" x="18" y="116" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u32 root_ofs  (from envelope start)</text>
    <text class="mono" x="18" y="136" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u32 env_size  (bytes)</text>
    <text class="mono" x="18" y="156" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">u32 arena_ofs (from buffer start, 16-aligned)</text>
    <rect class="card" x="475" y="0" width="445" height="170" rx="12" ry="12" />
    <text class="title" x="493" y="28" style="font-size:16px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">What lives where</text>
    <rect class="seg-e" x="493" y="44" width="190" height="46" rx="10" ry="10" />
    <text class="mono" x="507" y="72" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Envelope</text>
    <text class="sub muted" x="507" y="90" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">ValueRef16, arrays, objects, shapes</text>
    <rect class="seg-a" x="710" y="44" width="190" height="46" rx="10" ry="10" />
    <text class="mono" x="724" y="72" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Arena</text>
    <text class="sub muted" x="724" y="90" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">strings, blobs, tensor payload bytes</text>
    <path class="arrow" d="M 688 67 C 705 67, 705 67, 710 67" />
    <text class="sub muted" x="493" y="122" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Envelope stores compact references; payload bytes live in the arena.</text>
    <text class="sub muted" x="493" y="142" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">The envelope→arena gap is optional zero padding to align arena_ofs.</text>
    <text class="sub muted" x="493" y="162" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Inside the arena, each allocation is aligned independently.</text>
  </g>
  <g transform="translate(0, 330)">
    <rect class="card" x="0" y="0" width="920" height="150" rx="12" ry="12" />
    <text class="title" x="18" y="28" style="font-size:16px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Arena allocations (illustrative)</text>
    <text class="sub muted" x="18" y="48" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">`arena_alloc(len, align)` may insert padding before each allocation start.</text>
    <rect class="bar" x="18" y="70" width="884" height="54" rx="12" ry="12" />
    <rect class="seg-a" x="32" y="82" width="150" height="30" rx="8" ry="8" />
    <text class="mono" x="44" y="103" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">string</text>
    <rect class="seg-p" x="190" y="82" width="70" height="30" rx="8" ry="8" />
    <text class="mono" x="200" y="103" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">pad</text>
    <rect class="seg-a" x="268" y="82" width="220" height="30" rx="8" ry="8" />
    <text class="mono" x="280" y="103" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">blob / tensor bytes</text>
    <rect class="seg-p" x="496" y="82" width="70" height="30" rx="8" ry="8" />
    <text class="mono" x="506" y="103" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">pad</text>
    <rect class="seg-a" x="574" y="82" width="170" height="30" rx="8" ry="8" />
    <text class="mono" x="586" y="103" style="font-size:12px;" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">string</text>
    <text class="sub muted" x="18" y="142" font-family="ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial">Padding bytes are not separately tagged; they are just zeros inserted for alignment.</text>
  </g>
</svg>

## Header (20 bytes)

Byte layout:

- `u32 magic` = `'ZENV'` (`0x564E455A`)
- `u16 version` = `1`
- `u16 flags` = bit0 must be `1` (little-endian), others must be `0`
- `u32 root_ofs` = offset (from envelope start) to the root `ValueRef16`
- `u32 env_size` = envelope length in bytes
- `u32 arena_ofs` = offset (from buffer start) to arena start

Invariants:

- `arena_ofs` must be aligned to `ARENA_BASE_ALIGN` (`16`).
- `arena_ofs >= header_size + env_size` (optional zero padding between envelope and arena).

## `ValueRef16` (16 bytes)

Every value in the envelope is represented by a fixed 16-byte reference:

- `u8  tag`
- `u8  flags` (bit0 = inline payload; used by `STRING` only)
- `u16 aux` (small per-tag data; e.g. bool value, dtype, inline length)
- `u32 a`
- `u32 b`
- `u32 c`

### Tags

The v1 spec defines:

- `0 NULL`
- `1 BOOL`
- `2 I64`
- `3 F64`
- `4 STRING`
- `5 ARRAY`
- `6 OBJECT`
- `7 TYPED_ARRAY`

The current implementation in `include/zerialize/protocols/zera.hpp` also supports:

- `8 U64` (to preserve full `uint64_t` values without truncation)

### Scalar encoding

- `BOOL`: `aux = 0 or 1`
- `I64`: 64-bit payload bits stored in `(a,b)` (`a=low32`, `b=high32`)
- `U64`: 64-bit payload bits stored in `(a,b)` (`a=low32`, `b=high32`)
- `F64`: IEEE754 bits stored in `(a,b)`; reader `memcpy`s bits into `double`

Scalars may not be naturally aligned in the envelope. Readers should use `memcpy` or recomposition rather than unaligned typed loads.

### Strings

Two encodings:

- **Inline strings**: `flags&1==1`, `aux = byte_len (0..12)`, bytes stored in the 12-byte region spanning fields `(a,b,c)` in increasing address order.
- **Arena-backed strings**: `flags==0`, `a = arena_ofs`, `b = byte_len`.

### Arrays

`ARRAY` values point to an `ArrayPayload` in the envelope:

- `u32 count`
- `ValueRef16 elems[count]` (contiguous)

### Objects

`OBJECT` values point to an `ObjectPayload` in the envelope:

- `u32 count`
- then `count` entries back-to-back:
  - `u16 key_len`
  - `u16 reserved` (0)
  - `u8  key_bytes[key_len]`
  - `ValueRef16 value`

Lookups are a linear scan over entries. This keeps the format small and simple in v1.

### Typed arrays and blobs

`TYPED_ARRAY` is used by ZERA primarily to store a binary payload in the arena plus a shape in the envelope:

- `aux = dtype` enum
- `a = arena_ofs` to raw data
- `b = byte_len`
- `c = envelope_ofs` to `ShapePayload`

`ShapePayload` is:

- `u32 rank` (v1 recommends `<= 8`)
- `u64 dims[rank]`

A “blob” is `TYPED_ARRAY` with dtype `u8` and rank 1, where `dims[0] == byte_len`.

## Arena and Alignment

The arena is a raw byte region containing no per-segment headers.

- The arena base (`arena_ofs`) is 16-byte aligned (`ARENA_BASE_ALIGN = 16`).
- The writer aligns each arena allocation so that the payload address meets the required alignment.

This is crucial for tensor views:

- The tensor libraries can only safely reinterpret raw bytes as `T*` if the address is aligned to `alignof(T)`.
- If it’s not aligned, zerialize will copy into an owning buffer.

To make this decision visible, zerialize provides `tensor::TensorViewInfo`:

- `include/zerialize/tensor/view_info.hpp`
- Exposed via `XTensorView::viewInfo()` and `EigenMatrixView::viewInfo()`.

## Why ZERA is “this way”

The core choices are about balancing simplicity, safety, and performance:

- **Fixed `ValueRef16`** enables O(1) skipping and compact array storage.
- **One contiguous buffer** is cache-friendly and easy to transmit/store.
- **Envelope + arena** keeps the envelope dense while allowing aligned payloads.
- **u32 offsets/lengths** keep references compact and fast; v1 messages are < 4 GiB.
- **Linear-scan objects** keep v1 implementation simple and predictable; indexing can be added later if needed.
- **Checked reader** makes it reasonable to use ZERA on untrusted input.

## Interop and Translation

ZERA implements the same zerialize `Reader`/`Writer` surface as the other protocols, so you can:

- Serialize with `serialize<zerialize::Zera>(...)`
- Translate to/from other protocols with `translate<OtherProto>(zer_reader)` / `translate<zerialize::Zera>(other_reader)`

## Limitations and Future Work

- `OBJECT` lookup is linear-time (v1 by design).
- The v1 spec includes general typed arrays, but current library usage focuses on `u8` blobs for tensor payload bytes.
- In-place mutation is intentionally constrained (no growth/relocation in v1).
