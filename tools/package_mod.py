"""Package a compiled Windows x64 mod into a standard .dusk ZIP bundle.

Usage: python tools/package_mod.py --project . --dll build/lib/windows-amd64/mod.dll
                                  --output build/mods/kingdom_key.dusk
Requires Python 3.8+; no external packages or game disc are needed.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import tempfile
import zipfile


def package(project, dll, output):
    # Some restricted Windows environments permit normal file reads but deny
    # the directory handle used by strict realpath. Validate the types below;
    # actual resource reads still report permission/missing-file errors.
    project = project.resolve()
    dll = dll.resolve()
    if not project.is_dir() or not dll.is_file():
        raise ValueError('The project directory and compiled library must exist')
    manifest_path = project / 'mod.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    for field in ('id', 'name', 'version'):
        if not isinstance(manifest.get(field), str) or not manifest[field]:
            raise ValueError('mod.json is missing a nonempty ' + field)
    binary = dll.read_bytes()
    if len(binary) < 64 or binary[:2] != b'MZ':
        raise ValueError('The compiled library is not a Windows PE DLL')
    pe_offset = struct.unpack_from('<I', binary, 60)[0]
    if pe_offset + 24 > len(binary) or binary[pe_offset:pe_offset + 4] != b'PE\0\0':
        raise ValueError('The compiled library has an invalid PE header')
    machine = struct.unpack_from('<H', binary, pe_offset + 4)[0]
    characteristics = struct.unpack_from('<H', binary, pe_offset + 22)[0]
    if machine != 0x8664 or not characteristics & 0x2000:
        raise ValueError('The compiled library must be a Windows x64 DLL')
    resources = project / 'res'
    if not resources.is_dir():
        raise ValueError('The project is missing its res directory')
    entries = [('mod.json', manifest_path), ('lib/windows-amd64/mod.dll', dll)]
    for path in sorted(resources.rglob('*')):
        if path.is_symlink():
            raise ValueError('Resource symlinks are not supported: ' + str(path))
        if path.is_file():
            entries.append(('res/' + path.relative_to(resources).as_posix(), path))
    if 'res/kingdom_key.mesh' not in {name for name, _ in entries}:
        raise ValueError('The project is missing res/kingdom_key.mesh')
    output = output.resolve()
    if output.suffix.lower() != '.dusk':
        raise ValueError('The output filename must end in .dusk')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, suffix='.tmp', delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        with zipfile.ZipFile(temporary_path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
            for name, path in entries:
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                bundle.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
        with zipfile.ZipFile(temporary_path) as bundle:
            if bundle.testzip() is not None or bundle.namelist() != [name for name, _ in entries]:
                raise ValueError('Bundle verification failed')
        os.replace(temporary_path, output)
    finally:
        if temporary_path.exists():
            temporary_path.unlink()
    print('Packaged {}: Windows x64, {} entries, {:,} bytes'.format(output, len(entries), output.stat().st_size))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        package(args.project, args.dll, args.output)
    except (OSError, ValueError, struct.error, zipfile.BadZipFile) as error:
        parser.exit(1, 'Packaging failed: {}\n'.format(error))


if __name__ == '__main__':
    main()
