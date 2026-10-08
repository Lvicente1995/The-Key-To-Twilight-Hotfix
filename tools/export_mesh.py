"""Rebuild the Kingdom Key native mesh using only the Python standard library.

Usage: python tools/export_mesh.py
       python tools/export_mesh.py --input model.json --output model.mesh --format 2

Coordinates in the input are grip-centered Blender axes: blade +Z, thickness Y.
The mapping to Ordon sword coordinates is (scale*z + grip_x, scale*y, -scale*x).
"""
import argparse
import json
import math
from pathlib import Path
import struct
import sys


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def normalized(value):
    length = math.sqrt(sum(x * x for x in value))
    if length < 1e-9:
        raise ValueError('Cannot normalize a zero tangent')
    return tuple(x / length for x in value)


def rotate(value):
    return value[2], value[1], -value[0]


def srgb(value):
    value = max(0.0, min(1.0, value))
    return 12.92 * value if value <= 0.0031308 else 1.055 * value ** (1 / 2.4) - 0.055


def export(source, destination, version=2, scale=24.0, grip_x=-6.50472):
    with source.open(encoding='utf-8') as stream:
        model = json.load(stream)

    def position(value):
        x, y, z = rotate(value)
        return x * scale + grip_x, y * scale, z * scale

    colors = [tuple(srgb(c) for c in mat['rgba'][:3]) + (1.0,)
              for mat in model['materials']]
    pbr = [(mat['metallic'], mat['roughness']) for mat in model['materials']]
    centers = [position(point) for point in model['chain']['centers']]
    charm_center = position(model['chain']['charm_center'])
    rigid, chain, charm = [], [], []
    for mesh in model['meshes']:
        (rigid if mesh['bone'] == 'Weapon' else charm if mesh['bone'] == 'Charm' else chain).append(mesh)
    chain.sort(key=lambda mesh: mesh['name'])
    if len(chain) != len(centers):
        raise ValueError('Chain mesh and pivot counts differ')
    parts = [('rigid', (0., 0., 0.), (1., 0., 0.), rigid)]
    for index, mesh in enumerate(chain):
        next_center = centers[index + 1] if index + 1 < len(centers) else charm_center
        parts.append((f'chain_{index + 1:02d}', centers[index],
                      normalized(subtract(next_center, centers[index])), [mesh]))
    parts.append(('charm', charm_center, normalized(subtract(charm_center, centers[-1])), charm))
    magic = b'KKMESH02' if version == 2 else b'KKMESH01'
    vertex_struct = struct.Struct('<12f' if version == 2 else '<10f')
    metadata = {'format': magic.decode(), 'scale': scale, 'grip_x': grip_x,
                'anchor': position(model['chain']['anchor']), 'vertex_stride': vertex_struct.size,
                'parts': [], 'materials': model['materials']}
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open('wb') as stream:
        stream.write(magic)
        stream.write(struct.pack('<I3f', len(parts), *metadata['anchor']))
        for name, pivot, tangent, meshes in parts:
            name_bytes = name.encode('utf-8')
            count = sum(len(mesh['triangles']) * 3 for mesh in meshes)
            stream.write(struct.pack('<I', len(name_bytes)))
            stream.write(name_bytes)
            stream.write(struct.pack('<6fI', *pivot, *tangent, count))
            for mesh in meshes:
                for triangle in mesh['triangles']:
                    for index in triangle:
                        vertex = mesh['vertices'][index]
                        pos = subtract(position(vertex[:3]), pivot)
                        normal = rotate(vertex[3:6])
                        material_index = int(vertex[8])
                        row = (*pos, *normal, *colors[material_index])
                        if version == 2:
                            row += pbr[material_index]
                        stream.write(vertex_struct.pack(*row))
            metadata['parts'].append({'name': name, 'pivot': pivot, 'tangent': tangent,
                                      'vertices': count, 'triangles': count // 3})

    total, normal_error = 0, 0.0
    with destination.open('rb') as stream:
        assert stream.read(8) == magic
        count, *anchor = struct.unpack('<I3f', stream.read(16))
        assert count == len(parts)
        for expected in metadata['parts']:
            length, = struct.unpack('<I', stream.read(4))
            assert stream.read(length).decode('utf-8') == expected['name']
            *transform, vertex_count = struct.unpack('<6fI', stream.read(28))
            assert vertex_count == expected['vertices'] and vertex_count % 3 == 0
            for _ in range(vertex_count):
                vertex = vertex_struct.unpack(stream.read(vertex_struct.size))
                assert all(math.isfinite(component) for component in vertex)
                normal_error = max(normal_error, abs(math.sqrt(sum(x*x for x in vertex[3:6])) - 1))
                if version == 2:
                    assert 0 <= vertex[10] <= 1 and 0 <= vertex[11] <= 1
            total += vertex_count
        assert not stream.read(1)
    assert total == sum(len(mesh['triangles']) * 3 for mesh in model['meshes'])
    assert normal_error < 1e-5
    expected_bytes = 24 + sum(4 + len(part[0].encode('utf-8')) + 28 for part in parts) + total * vertex_struct.size
    assert destination.stat().st_size == expected_bytes
    metadata['validation'] = {'triangles': total // 3, 'file_size': expected_bytes,
                              'normal_length_max_error': normal_error, 'passed': True}
    return metadata


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=here / 'kingdom-key-mesh.json')
    parser.add_argument('--output', type=Path, default=here.parent / 'res' / 'kingdom_key.mesh')
    parser.add_argument('--format', type=int, choices=(1, 2), default=2)
    parser.add_argument('--scale', type=float, default=24.0)
    parser.add_argument('--grip-x', type=float, default=-6.50472)
    parser.add_argument('--info', type=Path, help='Optional JSON export/validation report')
    arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    args = parser.parse_args(arguments)
    if args.scale <= 0:
        parser.error('--scale must be positive')
    result = export(args.input, args.output, args.format, args.scale, args.grip_x)
    if args.info:
        args.info.parent.mkdir(parents=True, exist_ok=True)
        args.info.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(f"Wrote {args.output}: {result['format']}, {result['validation']['triangles']:,} triangles, "
          f"{result['validation']['file_size']:,} bytes; readback validation passed.")


if __name__ == '__main__':
    main()
