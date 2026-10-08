from pathlib import Path
import struct,json
root=Path(__file__).resolve().parents[1]
b=(root/'res/kingdom_key.mesh').read_bytes()
assert b[:8]==b'KKMESH02'
parts=struct.unpack_from('<I',b,8)[0];offset=24;report=[]
for _ in range(parts):
    n=struct.unpack_from('<I',b,offset)[0];offset+=4
    name=b[offset:offset+n].decode();offset+=n
    count=struct.unpack_from('<I',b,offset+24)[0];offset+=28
    raw=b[offset:offset+count*48];offset+=count*48
    uniques=[];lookup={};indices=[]
    for p in range(0,len(raw),48):
        key=raw[p:p+48]
        if key not in lookup:lookup[key]=len(uniques);uniques.append(key)
        indices.append(lookup[key])
    assert 0<len(uniques)<=65535
    assert len(indices)%3==0
    assert max(indices)<len(uniques)
    assert b''.join(uniques[i] for i in indices)==raw
    report.append({'name':name,'corners':count,'triangles':count//3,'unique_vertices':len(uniques),'max_index':max(indices)})
assert offset==len(b)
u=sum(p['unique_vertices'] for p in report);i=sum(p['corners'] for p in report)
result={'result':'PASS','parts':report,'total_unique_vertices':u,'total_triangles':i//3,'unique_vertex_bytes_color_pass':u*32,'unique_vertex_bytes_shadow_pass':u*12,'index_bytes_per_pass':i*2,'reconstruction':'Bit-exact match of all source triangles and all 12-float vertex attributes after indexing; triangle order and seams preserved.'}
(Path(__file__).parent/'indexed-mesh-result.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='parts'},indent=2))
