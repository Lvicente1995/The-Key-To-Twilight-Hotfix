#!/usr/bin/env python3
"""Merge per-platform .dusk bundles into one multi-platform bundle."""
import argparse, hashlib, subprocess, sys, tempfile, zipfile
from pathlib import Path
MOD_LIB_NAMES = ("mod.so", "mod.dll")
def fail(message):
    print(f"error: {message}", file=sys.stderr); sys.exit(1)
def entry_names(archive):
    names=[i.filename for i in archive.infolist() if not i.is_dir()]
    for name in names:
        if name.startswith("/") or ".." in name.split("/"): fail(f"unsafe entry name in {archive.filename}: {name}")
    return names
def main():
    p=argparse.ArgumentParser(); p.add_argument("inputs",nargs="+",type=Path); p.add_argument("-o","--output",required=True,type=Path); p.add_argument("--symgen",default="symgen"); args=p.parse_args()
    content_hashes={}; platform_sources={}; archives=[]
    for path in args.inputs:
        if not path.is_file(): fail(f"input does not exist: {path}")
        archive=zipfile.ZipFile(path); archives.append(archive); platforms=set()
        for name in entry_names(archive):
            if name.startswith("lib/"):
                parts=name.split("/")
                if len(parts)<3 or not parts[1]: fail(f"unexpected lib entry in {path}: {name}")
                platforms.add(parts[1]); continue
            digest=hashlib.sha256(archive.read(name)).hexdigest(); seen=content_hashes.get(name)
            if seen is None: content_hashes[name]=(digest,path)
            elif seen[0]!=digest: fail(f"'{name}' differs between {seen[1]} and {path}")
        if not platforms: fail(f"{path} contains no native libraries")
        for platform in platforms:
            if platform in platform_sources: fail(f"platform '{platform}' provided twice")
            platform_sources[platform]=path
    if "mod.json" not in content_hashes: fail("bundles contain no mod.json")
    with tempfile.TemporaryDirectory() as tmp:
        stage=Path(tmp)
        archives[0].extractall(stage,members=[n for n in entry_names(archives[0]) if not n.startswith("lib/")])
        for archive in archives: archive.extractall(stage,members=[n for n in entry_names(archive) if n.startswith("lib/")])
        mod_libs=[]
        for platform in sorted(platform_sources):
            libs=[p for p in (stage/"lib"/platform/n for n in MOD_LIB_NAMES) if p.is_file()]
            if len(libs)!=1: fail(f"expected exactly one mod library in lib/{platform}/")
            mod_libs.append(libs[0])
        result=subprocess.run([args.symgen,"modmeta","--check","--update-json",str(stage/"mod.json"),*map(str,mod_libs)])
        if result.returncode!=0: fail("symgen modmeta verification failed")
        args.output.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(args.output,"w",zipfile.ZIP_DEFLATED) as out:
            for file in sorted(p for p in stage.rglob("*") if p.is_file()):
                info=zipfile.ZipInfo(file.relative_to(stage).as_posix(),date_time=(1980,1,1,0,0,0)); info.compress_type=zipfile.ZIP_DEFLATED; out.writestr(info,file.read_bytes())
    print(f"wrote {args.output} with platforms: {', '.join(sorted(platform_sources))}")
if __name__=="__main__": main()
