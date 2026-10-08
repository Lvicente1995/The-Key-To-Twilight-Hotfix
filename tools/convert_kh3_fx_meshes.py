"""Convert selected KH3 cooked StaticMesh JSON exports to the mod's KKFXM002.

Inputs are UAssetGUI 1.1 JSON exports of the user's own selected UE4.17 assets.
This deliberately narrow parser rejects other layouts instead of guessing offsets.
Format reference: UEViewer's Unreal/UnrealMesh/UnMesh4.cpp (legacy UE4 buffers):
https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealMesh/UnMesh4.cpp
No game assets are included in this converter. Positions, UV0 and colors are retained.
KKFXM002 vertices are 24 bytes: float32 x,y,z,u,v followed by RGBA8, little-endian.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
from pathlib import Path
import struct

NAMES = (
    'jmd_cir_32x1_00', 'mmd_brd_2x6_02', 'zmd_grd_01', 'smd_so010_00',
    'smd_so020_rsn0', 'jmd_cir_12x2_00', 'jmd_dst_8_00', 'omd_grd_04x04_00',
)


class Reader:
    def __init__(self, data: bytes):
        self.data, self.at = data, 0

    def read(self, size: int) -> bytes:
        if size < 0 or self.at + size > len(self.data):
            raise ValueError(f'Buffer overrun at {self.at:#x}, requesting {size}')
        result = self.data[self.at:self.at + size]
        self.at += size
        return result

    def unpack(self, fmt: str):
        return struct.unpack('<' + fmt, self.read(struct.calcsize('<' + fmt)))

    def u32(self) -> int:
        return self.unpack('I')[0]

    def count(self, limit: int = 1_000_000) -> int:
        value = self.u32()
        require(value <= limit, f'Unexpected array length {value} at {self.at - 4:#x}')
        return value

    def boolean(self) -> bool:
        value = self.u32()
        require(value in (0, 1), 'Invalid serialized bool')
        return bool(value)

    def bulk(self, expected_stride: int) -> tuple[int, bytes]:
        stride, count = self.u32(), self.count()
        require(stride == expected_stride, f'Unexpected bulk stride {stride}')
        return count, self.read(stride * count)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def index_buffer(r: Reader) -> tuple[int, ...]:
    wide = r.boolean()
    count, data = r.bulk(1)
    stride = 4 if wide else 2
    require(count % stride == 0, 'Misaligned index buffer')
    return struct.unpack('<' + ('I' if wide else 'H') * (count // stride), data)


def weighted_sampler(r: Reader):
    r.read(r.count() * 4)
    r.read(r.count() * 4)
    r.read(4)


def parse_mesh(path: Path) -> tuple[list[tuple[float, ...]], tuple[int, ...], dict]:
    doc = json.loads(path.read_text(encoding='utf-8-sig'))
    exports = [e for e in doc['Exports'] if e.get('ObjectName', '').lower() == path.stem.lower()]
    require(len(exports) == 1, 'Expected exactly one named StaticMesh export')
    export = exports[0]
    extras = base64.b64decode(export['Extras'], validate=True)
    r = Reader(extras)
    require(r.read(2) == b'\x01\x00', 'Expected editor-stripped client mesh')
    require(r.boolean(), 'Expected cooked mesh')
    r.read(8)  # BodySetup and NavCollision object references.
    r.read(16)  # Lighting GUID.
    r.read(r.count(100) * 4)  # Socket object references.
    require(r.count(8) == 1, 'Only one-LOD source effects are supported')
    strip = r.read(2)
    require(strip == b'\x01\x00', 'Unexpected LOD strip flags')
    sections = [r.unpack('7I') for _ in range(r.count(64))]
    require(len(sections) == 1, 'Expected a single effect material section')
    r.read(4)  # Maximum LOD deviation.

    require(r.u32() == 12, 'Position stride must be three float32 values')
    vertex_count = r.count(65535)
    count, raw_positions = r.bulk(12)
    require(count == vertex_count and count > 0, 'Position count mismatch')
    positions = list(struct.iter_unpack('<3f', raw_positions))

    require(r.read(2) == b'\x01\x00', 'Unexpected UV strip flags')
    uv_channels, vertex_stride, uv_count = r.unpack('3I')
    full_uv, high_tangent = r.boolean(), r.boolean()
    require(1 <= uv_channels <= 8 and uv_count == vertex_count, 'UV count mismatch')
    tangent_bytes = 16 if high_tangent else 8
    uv_bytes = 8 if full_uv else 4
    require(vertex_stride == tangent_bytes + uv_channels * uv_bytes, 'Unexpected UV stride')
    count, raw_uv = r.bulk(vertex_stride)
    require(count == vertex_count, 'UV bulk count mismatch')
    uvs = [struct.unpack_from('<2f' if full_uv else '<2e', raw_uv,
                             i * vertex_stride + tangent_bytes) for i in range(count)]

    require(r.read(2) == b'\x01\x00', 'Unexpected color strip flags')
    color_stride, color_count = r.unpack('2I')
    require(color_count in (0, vertex_count), 'Color count mismatch')
    raw_colors = b''
    if color_count:
        require(color_stride == 4, 'Unexpected color stride')
        count, raw_colors = r.bulk(4)
        require(count == vertex_count, 'Color bulk count mismatch')
    indices = index_buffer(r)
    for _ in range(4):  # Reversed, depth, reversed depth, adjacency buffers.
        index_buffer(r)
    for _ in range(len(sections) + 1):
        weighted_sampler(r)
    require(r.read(2) == b'\x01\x00', 'Unexpected distance field strip flags')
    require(not r.boolean(), 'Distance field meshes are not expected here')
    render_bounds = r.unpack('7f')

    section = sections[0]
    require(section[1] == 0 and section[2] * 3 == len(indices), 'Section triangle count mismatch')
    require(len(indices) % 3 == 0 and max(indices) < vertex_count, 'Invalid triangle indices')
    require(section[3] == min(indices) and section[4] == max(indices), 'Section vertex range mismatch')
    colors = [(red, green, blue, alpha) for blue, green, red, alpha in
              struct.iter_unpack('4B', raw_colors)] if raw_colors else [(255, 255, 255, 255)] * vertex_count
    vertices = [(*p, *uv, *rgba) for p, uv, rgba in zip(positions, uvs, colors)]
    require(all(math.isfinite(v) for vertex in vertices for v in vertex), 'Nonfinite mesh value')
    low = [min(p[a] for p in positions) for a in range(3)]
    high = [max(p[a] for p in positions) for a in range(3)]
    for a in range(3):
        # Bounds can be conservatively padded by the asset's importer.
        require(low[a] >= render_bounds[a] - render_bounds[a + 3] - .001 and
                high[a] <= render_bounds[a] + render_bounds[a + 3] + .001,
                'Decoded vertices exceed serialized render bounds')

    # Verify UAssetGUI's opaque binary suffix against the original on-disk export.
    uasset, uexp = path.with_suffix('.uasset'), path.with_suffix('.uexp')
    source_hashes = {}
    if uasset.exists() and uexp.exists():
        original = uexp.read_bytes()
        start = export['SerialOffset'] + export['SerialSize'] - len(extras) - uasset.stat().st_size
        require(start >= 0 and original[start:start + len(extras)] == extras,
                'JSON extras differ from the original cooked asset')
        source_hashes = {p.suffix: hashlib.sha256(p.read_bytes()).hexdigest() for p in (uasset, uexp)}
    metadata = dict(name=path.stem.lower(), vertices=vertex_count, triangles=len(indices) // 3,
                    bounds_min=low, bounds_max=high,
                    uv_min=[min(uv[a] for uv in uvs) for a in range(2)],
                    uv_max=[max(uv[a] for uv in uvs) for a in range(2)],
                    uv_channels=uv_channels, source_uv_precision='float32' if full_uv else 'float16',
                    vertex_colors_present=bool(color_count),
                    vertex_color_byte_range=[min(raw_colors), max(raw_colors)] if raw_colors else None,
                    vertex_color_unique_bgra=sorted(set(struct.iter_unpack('4B', raw_colors))) if raw_colors else [],
                    render_bounds=list(render_bounds), section=list(section),
                    source_sha256=source_hashes)
    return vertices, indices, metadata


def verify_output(data: bytes, meshes):
    r = Reader(data)
    require(r.read(8) == b'KKFXM002' and r.u32() == len(meshes), 'Output header mismatch')
    for name, vertices, indices, _ in meshes:
        require(r.read(r.count(256)).decode('utf-8') == name, 'Output name mismatch')
        vc, ic = r.unpack('2I')
        require((vc, ic) == (len(vertices), len(indices)), 'Output count mismatch')
        result = list(struct.iter_unpack('<5f4B', r.read(vc * 24)))
        require(result == vertices, 'Output position/UV/color mismatch')
        require(r.unpack('H' * ic) == indices, 'Output index mismatch')
    require(r.at == len(data), 'Trailing output data')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input_directory', type=Path, help='Directory containing selected UAssetGUI JSON exports')
    parser.add_argument('output', type=Path, help='Destination res/kh3fx/meshes.bin')
    parser.add_argument('--report', type=Path, help='Optional conversion/validation JSON report')
    args = parser.parse_args()
    files = {}
    for path in args.input_directory.rglob('*.json'):
        name = path.stem.lower()
        if name in NAMES:
            require(name not in files, f'Duplicate source {name}')
            files[name] = path
    require(set(files) == set(NAMES), f'Missing required sources: {sorted(set(NAMES) - set(files))}')
    meshes = [(name, *parse_mesh(files[name])) for name in NAMES]
    data = bytearray(b'KKFXM002' + struct.pack('<I', len(meshes)))
    for name, vertices, indices, _ in meshes:
        encoded = name.encode('utf-8')
        data += struct.pack('<I', len(encoded)) + encoded
        data += struct.pack('<II', len(vertices), len(indices))
        data += b''.join(struct.pack('<5f4B', *v) for v in vertices)
        data += struct.pack('<' + 'H' * len(indices), *indices)
    verify_output(bytes(data), meshes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(format='KKFXM002', result='PASS', mesh_count=len(meshes),
                  bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                  checks=['source binary suffix equality', 'buffer strides and counts',
                          'section triangle and index bounds', 'finite positions and UVs',
                          'serialized render bounds', 'complete output round-trip equality'],
                  meshes=[m[3] for m in meshes])
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
