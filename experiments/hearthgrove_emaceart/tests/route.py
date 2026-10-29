"""Build keyboard/mouse replay from a supplied private scene, without teleporting."""
import math,heapq

def build(cfg):
 solids=cfg['solids']
 def free(x,z,rad=.5):
  return abs(x)<29.65-rad and abs(z)<29.65-rad and all(max(abs(x-a)-hx,0)**2+max(abs(z-b)-hz,0)**2>=rad**2 for a,b,hx,hz in solids)
 def astar(start,end):
  a=tuple(round(v*2) for v in start);b=tuple(round(v*2)for v in end);q=[(0,a)];came={a:None};cost={a:0}
  while q:
   _,p=heapq.heappop(q)
   if p==b:break
   for dx,dz in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,1),(1,-1),(-1,-1)]:
    n=(p[0]+dx,p[1]+dz)
    if not all(free((p[0]+dx*t)/2,(p[1]+dz*t)/2)for t in [.25,.5,.75,1]):continue
    v=cost[p]+math.hypot(dx,dz)
    if v<cost.get(n,1e9):cost[n]=v;came[n]=p;heapq.heappush(q,(v+math.hypot(n[0]-b[0],n[1]-b[1]),n))
  else:raise RuntimeError('no route')
  out=[];p=b
  while p is not None:out.append((p[0]/2,p[1]/2));p=came[p]
  out.reverse();ret=[out[0]]
  for i in range(1,len(out)-1):
   if (out[i][0]-out[i-1][0],out[i][1]-out[i-1][1])!=(out[i+1][0]-out[i][0],out[i+1][1]-out[i][1]):ret.append(out[i])
  return ret+[out[-1]]
 actions=[];frame=5;x,z=cfg['spawn'][0],cfg['spawn'][2];yaw=0
 # Quantized mouse events and actual movement at the engine's 30Hz replay rate.
 def go(end):
  nonlocal frame,x,z,yaw
  path=astar((x,z),end)
  for gx,gz in path[1:]:
   dx,dz=gx-x,gz-z;distance=math.hypot(dx,dz);target=math.atan2(dx,-dz)
   delta=math.remainder(target-yaw,math.tau);mouse=round(delta/.0025);yaw=math.remainder(yaw+mouse*.0025,math.tau)
   actions.append((frame,'mouse',f'{mouse} 0'));frame+=1
   n=round(distance/(2.2/30));actions.extend([(frame,'down','W'),(frame+n,'up','W')]);x+=math.sin(yaw)*n*2.2/30;z-=math.cos(yaw)*n*2.2/30;frame+=n+2
  pass
 def key(k):
  nonlocal frame
  actions.extend([(frame,'down',k),(frame+1,'up',k)]);frame+=25
 rest=cfg['rest_spots'][-1];go((rest[0],rest[2]));key('E');go((-1.5,4.5));key('E');key('2');key('1');key('1');key('3');key('Return');key('F5');frame+=30
 return actions,frame
