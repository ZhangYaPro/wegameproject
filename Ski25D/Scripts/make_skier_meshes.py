"""Original component meshes in centimetres, ready for articulated UE assembly."""
from pathlib import Path
import math
out=Path(__file__).resolve().parents[1]/'ContentSource'/'Skier'
out.mkdir(parents=True,exist_ok=True)
def save(name,v,f):
    lines=['# Original Ski25D mesh, centimetres']+['v %.5f %.5f %.5f'%p for p in v]
    lines += ['f '+' '.join(str(i+1) for i in face) for face in f]
    (out/(name+'.obj')).write_text('\n'.join(lines),encoding='ascii')
def loft(name,rings,n=24,power=3.2):
    v=[];f=[]
    for z,a,b,cx in rings:
        for i in range(n):
            t=2*math.pi*i/n;c=math.cos(t);s=math.sin(t)
            v.append((cx+a*math.copysign(abs(c)**(2/power),c),b*math.copysign(abs(s)**(2/power),s),z))
    for j in range(len(rings)-1):
        for i in range(n):
            a=j*n+i;b=j*n+(i+1)%n;f.extend([(a,b,a+n),(b,b+n,a+n)])
    v.extend([(rings[0][3],0,rings[0][0]),(rings[-1][3],0,rings[-1][0])]);lo=len(v)-2;hi=lo+1
    for i in range(n):f.extend([(lo,(i+1)%n,i),(hi,(len(rings)-1)*n+i,(len(rings)-1)*n+(i+1)%n)])
    save(name,v,f)
loft('Jacket',[(-50,36,39,0),(-43,47,46,0),(-18,48,47,-2),(13,44,50,-3),(35,39,48,0),(45,28,38,0),(50,22,26,0)])
loft('UpperArm',[(-50,34,36,0),(-43,46,48,0),(-12,47,48,0),(26,40,42,0),(47,32,34,0),(50,30,32,0)])
loft('Forearm',[(-50,37,39,0),(-40,43,45,0),(0,39,40,0),(40,30,32,0),(50,28,30,0)])
loft('Trousers',[(-50,44,47,0),(-37,49,50,0),(0,44,44,0),(40,36,37,0),(50,33,35,0)])
loft('Shin',[(-50,36,38,0),(-35,42,43,0),(15,39,40,0),(45,34,35,0),(50,32,34,0)])
loft('Boot',[(-50,43,43,9),(-40,50,48,12),(-22,50,47,12),(-4,38,43,1),(12,30,39,-10),(45,29,37,-11),(50,26,34,-11)],power=4)
loft('Helmet',[(-50,36,45,0),(-40,47,49,0),(-10,50,50,0),(20,45,47,-2),(40,31,34,-4),(49,10,13,-5),(50,1,1,-5)],power=2.2)
loft('Goggles',[(-50,28,40,0),(-40,45,48,0),(25,50,50,0),(45,40,45,0),(50,32,37,0)],power=4)
loft('Glove',[(-50,29,35,0),(-30,43,45,0),(22,46,46,0),(43,32,35,4),(50,15,18,4)],power=2.8)
loft('Collar',[(-50,42,45,0),(-30,49,50,0),(35,46,49,0),(50,40,43,0)])
# A continuous sidecut ski with a rounded, raised shovel, not a flat cube.
v=[];f=[]
stations=[(-112,2,4),(-106,5.5,2),(-90,6.5,0),(-45,5.7,0),(0,5.1,0),(55,6.1,0),(87,7.3,3),(104,6.5,10),(113,3.5,17),(117,.5,20)]
for x,w,z in stations:v.extend([(x,-w,z),(x,w,z),(x,-w,z-3),(x,w,z-3)])
for j in range(len(stations)-1):
    a=j*4;b=a+4
    f.extend([(a,b,a+1),(a+1,b,b+1),(a+2,a+3,b+2),(a+3,b+3,b+2),(a,a+2,b),(a+2,b+2,b),(a+1,b+1,a+3),(a+3,b+1,b+3)])
f.extend([(0,1,2),(1,3,2),(36,38,37),(37,38,39)])
save('Ski',v,f)
# Accurate 3:2 flag geometry: large star at (5,5), small stars face it.
save('FlagCloth',[(-50,0,-100/3),(50,0,-100/3),(50,0,100/3),(-50,0,100/3)],[(0,2,1),(0,3,2)])
v=[];f=[]
for x,y,r,angle in [(5,5,3,-math.pi/2)]+[(x,y,1,math.atan2(5-y,5-x)) for x,y in [(10,2),(12,4),(12,7),(10,9)]]:
    start=len(v);v.append(((x-15)*100/30,.3,(10-y)*100/30))
    for k in range(10):
        t=angle+k*math.pi/5;rad=r if k%2==0 else r*.381966
        v.append(((x+rad*math.cos(t)-15)*100/30,.3,(10-y-rad*math.sin(t))*100/30))
    for k in range(10):f.append((start,start+1+k,start+1+(k+1)%10))
save('FlagStars',v,f)
print('SKIER_MESHES_READY',len(list(out.glob('*.obj'))))
