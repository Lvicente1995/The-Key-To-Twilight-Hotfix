"""Independent numeric check of the native anchor interpolation correction."""
import json,math,random
from pathlib import Path
random.seed(22092026)
def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,t):return tuple(x*t for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def length(v):return math.sqrt(dot(v,v))
def unit(v):return mul(v,1/length(v))
def cross(a,b):return(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def mix(a,b,t):return add(mul(a,1-t),mul(b,t))
def rotate(v,k,t):return add(add(mul(v,math.cos(t)),mul(cross(k,v),math.sin(t))),mul(k,dot(k,v)*(1-math.cos(t))))
def randomv():return tuple(random.uniform(-1,1) for _ in range(3))
anchor=(-30.0023358,-.58540146,-.18928126)
distance=1.3247127466
worst_uncorrected=worst_error=worst_stretch=0
count=0
for _ in range(2000):
    axis=unit(randomv());angle=random.uniform(-math.pi,math.pi)
    translation=mul(randomv(),500)
    initial=anchor;final=add(rotate(anchor,axis,angle),translation)
    initial_offset=mul(unit(randomv()),distance);final_offset=mul(unit(randomv()),distance)
    first=add(initial,initial_offset);last=add(final,final_offset)
    for step in range(21):
        t=step/20
        # Same decomposition as host interpolation: rotation interpolates on its
        # shortest arc while translation is linear.
        rigid_anchor=add(rotate(anchor,axis,angle*t),mul(translation,t))
        recorded_anchor=mix(initial,final,t)
        recorded_node=mix(first,last,t)
        correction=sub(rigid_anchor,recorded_anchor)
        corrected=add(recorded_node,correction)
        expected_offset=mix(initial_offset,final_offset,t)
        worst_uncorrected=max(worst_uncorrected,length(correction))
        worst_error=max(worst_error,length(sub(sub(corrected,rigid_anchor),expected_offset)))
        worst_stretch=max(worst_stretch,length(sub(corrected,rigid_anchor))-distance)
        count+=1
assert worst_error<1e-10
assert worst_stretch<1e-10
report={'cases':count,'maximum_uncorrected_anchor_discrepancy_cm':worst_uncorrected,'maximum_corrected_anchor_offset_error_cm':worst_error,'maximum_first_link_extension_cm':worst_stretch,'result':'PASS','note':'Checks the anchor correction algebra and rigid-motion interpolation numerically, not GPU rendering or exact closed-ring contact.'}
(Path(__file__).parent/'anchor-interpolation-result.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
