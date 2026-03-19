#!/usr/bin/env python3
import io, time, requests
from PIL import Image, ImageSequence

BASE='http://192.168.0.48'
SCREEN_ID='weather-icon-preview'

ICONS=[
    ('sunny',59532,False),
    ('partly_cloudy',59534,False),
    ('cloudy',11220,True),
    ('rainy',2284,True),
    ('snowy',66067,True),
    ('lightning',49299,True),
    ('windy',2672,False),
    ('foggy',11220,True),
]

def api(method,path,**kw):
    r=requests.request(method,BASE+path,timeout=8,**kw)
    r.raise_for_status()
    try:
        return r.json()
    except Exception:
        return r.text

def ensure_canvas_screen():
    s=api('GET','/api/screens')
    ids={x['id']:x for x in s.get('screens',[])}
    if SCREEN_ID not in ids:
        api('POST','/api/screens',json={"id":SCREEN_ID,"type":"canvas","duration":180,"enabled":True})
    else:
        api('PUT',f'/api/screens/{SCREEN_ID}',json={"enabled":True,"duration":180})

    api('PUT','/api/screens/cycling',json={"enabled":False})

    for _ in range(10):
        s=api('GET','/api/screens')
        active=next((x for x in s['screens'] if x.get('active')),None)
        if active and active['id']==SCREEN_ID:
            return
        api('POST','/api/screens/next')
        time.sleep(0.35)

def to_dp(img):
    img=img.convert('RGBA').resize((8,8),Image.NEAREST)
    pts=[]
    for y in range(8):
        for x in range(8):
            r,g,b,a=img.getpixel((x,y))
            if a>30:
                pts.append([x,y,f"#{r:02X}{g:02X}{b:02X}"])
    return pts

def post_frame(dp):
    api('POST',f'/api/canvas/{SCREEN_ID}/draw',json={"cl":True,"dp":dp})

def load_frames(icon_id,animated):
    ext='gif' if animated else 'png'
    url=f'https://developer.lametric.com/content/apps/icon_thumbs/{icon_id}.{ext}'
    b=requests.get(url,timeout=8)
    b.raise_for_status()
    img=Image.open(io.BytesIO(b.content))
    frames=[]
    delays=[]
    if animated and getattr(img,'is_animated',False):
        for fr in ImageSequence.Iterator(img):
            frames.append(to_dp(fr))
            delays.append(max(0.12, (fr.info.get('duration',140)/1000.0)))
    else:
        frames=[to_dp(img)]
        delays=[2.2]
    return frames,delays

def run_preview(loop_count=2):
    ensure_canvas_screen()
    cache={}
    for loop in range(loop_count):
        for name,iid,anim in ICONS:
            print(f'preview {name} ({iid}) {"anim" if anim else "static"}')
            key=(iid,anim)
            if key not in cache:
                cache[key]=load_frames(iid,anim)
            frames,delays=cache[key]
            if len(frames)==1:
                post_frame(frames[0]); time.sleep(2.2)
            else:
                t_end=time.time()+3.4
                idx=0
                while time.time()<t_end:
                    post_frame(frames[idx])
                    time.sleep(delays[idx])
                    idx=(idx+1)%len(frames)
                time.sleep(0.4)

if __name__=='__main__':
    run_preview(loop_count=2)
