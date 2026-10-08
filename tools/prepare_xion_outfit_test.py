"""Build a private test fixture from extracted Link skeleton metadata.

The supplied JSON is produced by the reference extractor from the user's game.
The resulting fixture contains game bind matrices and belongs in local work,
not the redistributable mod. No game installation is modified by this script.
"""
import argparse,hashlib,json,struct
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--reference',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
data=json.loads(args.reference.read_text())
outfits=[('Hero','Kmdl/al.bmd','Kmdl/al_face.bmd'),
         ('Ordon','Bmdl/bl.bmd','Bmdl/al_face.bmd'),
         ('Zora','Zmdl/zl.bmd','Zmdl/zl_face.bmd'),
         ('Magic','Mmdl/ml.bmd','Mmdl/al_face.bmd')]
blob=bytearray(b'XOUTFIT1'+struct.pack('<I',len(outfits)))
reference_body=data[outfits[0][1]];reference_face=data[outfits[0][2]]
records=[]
for name,body_path,face_path in outfits:
    body,face=data[body_path],data[face_path]
    assert body['joint_count']==35 and face['joint_count']==5
    errors={}
    for label,current,reference in [('body',body,reference_body),('face',face,reference_face)]:
        assert [(b['name'],b['parent']) for b in current['joints']]==[(b['name'],b['parent']) for b in reference['joints']]
        for field in ('local_rest','global_rest','inverse_global_rest'):
            delta=max(abs(a-b) for j,k in zip(current['joints'],reference['joints']) for row,refrow in zip(j[field],k[field]) for a,b in zip(row,refrow))
            assert delta<1e-6,(name,label,field,delta)
            errors[label+'_'+field+'_max_delta_from_hero']=delta
    encoded=name.encode();blob+=struct.pack('<I',len(encoded))+encoded
    for j,bone in enumerate(body['joints']+face['joints']):
        parent=bone['parent']
        if j>=35:parent=4 if parent is None else parent+35
        assert parent is None or parent<j
        blob+=struct.pack('<I',0 if parent is None else parent+1)
        blob+=struct.pack('<12f',*[v for row in bone['local_rest'][:3] for v in row])
    records.append(dict(outfit=name,body_source=body_path,face_source=face_path,body_joints=35,face_joints=5,
        names_and_parent_indices_match_hero=True,**errors,
        body_JNT_rest_times_native_EVP_inverse_max_error=body['rest_times_evp_inverse_max_error'],
        face_JNT_rest_times_native_EVP_inverse_max_error=face['rest_times_evp_inverse_max_error']))
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(blob)
args.report.parent.mkdir(parents=True,exist_ok=True)
args.report.write_text(json.dumps(dict(result='PASS',source_reference_sha256=hashlib.sha256(args.reference.read_bytes()).hexdigest(),
    scope='Offline extracted skeleton comparison. No runtime outfit testing is claimed.',outfits=records),indent=2))
print('PASS: all four outfits have matching 35 body + 5 face joint names, hierarchy and bind matrices.')
