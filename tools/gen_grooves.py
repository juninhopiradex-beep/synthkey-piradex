import sys
G=[]  # (style, kind, name, bpm, bars, swing, dsl)
def drums(style,name,bpm,lanes,swing=0.0,bars=2):
    parts=[]
    for ch,p in lanes.items():
        p=p.replace('|','')
        assert len(p)==bars*16,(style,name,ch,len(p))
        parts.append(ch+':'+p)
    G.append((style,'Bateria',name,bpm,bars,swing,' '.join(parts)))
def notes(style,kind,name,bpm,bars,dsl,swing=0.0):
    G.append((style,kind,name,bpm,bars,swing,dsl))
def n(s,pitches,l,v=100):
    ps='+'.join(str(x) for x in pitches) if isinstance(pitches,(list,tuple)) else str(pitches)
    return f"{s}:{ps}:{l}:{v}"
def chordbars(chords,rhythm,v=85,bar=16):
    out=[]
    for i,ch in enumerate(chords):
        for (s,l) in rhythm: out.append(n(i*bar+s,ch,l,v))
    return ' '.join(out)
def arp(chords,steps,l=3,v=80,bar=16):
    out=[]
    for i,ch in enumerate(chords):
        for k,s in enumerate(steps): out.append(n(i*bar+s,ch[k%len(ch)] ,l,v))
    return ' '.join(out)

Cm=[0,3,7]; Ab=[-4,0,3]; Bb=[-2,2,5]; Gm=[-5,-2,2]
Cm7=[0,3,7,10]; Fm7=[-7,-4,0,3]; Ab7=[-4,0,3,7]; Gm7=[-5,-2,2,5]; Bb7=[-2,2,5,8]; Eb7=[-9,-5,-2,2]
Cm9=[0,3,7,10,14]; Fm9=[-7,-4,0,3,7]; Abm9=[-4,0,3,7,10]; Gm9=[-5,-2,2,5,9]

# ------------------------------------------------------------ TRAP
drums('trap','Trap Clássico',140,{'k':'x.......x.x.....|..x...x...x.....','s':'........x.......|........x.......','c':'........x.......|........x.......','h':'x.x.x.x.x.x.x.x.|x.x.x.xxx.x.xxxx','o':'..............x.|................'})
drums('trap','Trap Rolos Rápidos',140,{'k':'x..x......x.....|x.......x.x.x...','s':'........x.......|........x.......','r':'................|..............x.','h':'xxxxxxxxxxxxxxxx|x.x.xxxxx.xxxxxx'})
drums('trap','Trap Dark Half-Time',140,{'k':'x..........x....|..x.....x..x....','s':'........x.......|........x.......','h':'x.x.x.x.x.x.x.x.|x.x.x.x.x.xxx.xx','o':'.......x.......x|.......x........'})
notes('trap','Baixo','808 Sub Trap',140,2,"0:0:6:115 6:0:2:100 10:3:4:105 16:-2:6:115 22:-2:2:100 26:0:4:105 30:-5:2:100")
notes('trap','Baixo','808 Deslizante',140,2,"0:0:10:115 12:3:2:95 14:5:2:95 16:-2:8:115 24:0:4:100 28:-4:4:100")
notes('trap','Acordes','Acordes Sombrios',140,4,chordbars([Cm,Ab,Bb,Gm],[(0,16)],85))
notes('trap','Acordes','Acordes Staccato',140,4,chordbars([Cm,Ab,Bb,Gm],[(0,3),(4,2),(8,3)],88))
notes('trap','Melodia','Melodia Sino Dark',140,4,"0:7:3:90 4:10:3:80 8:12:4:85 14:10:2:75 16:8:3:90 20:7:3:80 24:3:4:85 32:5:3:90 36:7:3:80 40:10:4:85 46:7:2:75 48:3:3:90 52:2:3:80 56:0:8:85")

# ------------------------------------------------------------ R&B
drums('rnb','R&B Suave',88,{'k':'x.....x...x.....|x.......x.x.....','s':'....x.......x...|....x.......x...','h':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.','r':'..o.......o.....|..o.......o.....'},0.12)
drums('rnb','R&B Quebrado',88,{'k':'x.....x..x......|..x.....x..x....','s':'....x.......x...|....x.....x.x...','h':'x.xxx.x.x.xxx.x.|x.xxx.x.x.xxx.xx','c':'................|............x...'},0.12)
drums('rnb','Neo Soul Pocket',84,{'k':'x..x..x...x.....|x..x......x..x..','s':'....x.......x...|....x.......x...','h':'x.o.x.o.x.o.x.o.|x.o.x.o.x.o.x.xo','m':'o.o.o.o.o.o.o.o.|o.o.o.o.o.o.o.o.'},0.15)
notes('rnb','Baixo','Baixo R&B Suave',88,2,"0:0:4:100 6:0:2:85 8:3:4:95 14:-2:2:85 16:-4:6:100 24:-2:3:90 28:-5:4:95",0.1)
notes('rnb','Baixo','Baixo Sincopado',88,2,"0:0:3:100 3:0:1:80 6:-2:2:95 8:3:4:95 16:5:3:100 19:5:1:80 22:3:2:95 24:0:6:95",0.1)
notes('rnb','Acordes','Progressão Min9',88,4,chordbars([Cm9,Fm9,Abm9,Gm9],[(0,16)],78))
notes('rnb','Acordes','Rhodes Ritmado',88,4,chordbars([Cm7,Fm7,Ab7,Gm7],[(0,6),(6,4),(12,4)],82),0.1)
notes('rnb','Melodia','Melodia Soul',88,4,"0:12:4:90 4:10:2:80 6:7:4:85 12:10:3:80 16:8:4:90 20:7:2:80 22:3:4:85 28:5:3:80 32:7:4:90 36:10:2:80 38:12:4:85 44:14:3:85 48:12:6:90 56:10:8:80",0.1)

# ------------------------------------------------------------ AFROBEAT
drums('afrobeat','Afrobeat Clássico',102,{'k':'x..x..x...x..x..|x..x..x...x.....','s':'....x.......x...|....x.......x...','h':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.','m':'..x...x...x...x.|..x...x...x...x.','j':'..x..x..x..x..x.|..x..x..x..x..x.'},0.08)
drums('afrobeat','Afro Percussão',102,{'k':'x.......x.......|x.......x.......','g':'x..x..x...x.x...|x..x..x...x.x...','j':'..x..x..x..x..x.|..x..x..x..x..x.','b':'.x..x..x.x..x...|.x..x..x.x..x...','m':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.','r':'....x.......x...|....x.......x...'},0.08)
drums('afrobeat','Highlife Swing',108,{'k':'x.....x.x.......|x.....x.x.......','s':'........x.......|........x.......','h':'x.xxx.xxx.xxx.xx|x.xxx.xxx.xxx.xx','o':'..x...x...x...x.|..x...x...x...x.'},0.18)
notes('afrobeat','Baixo','Baixo Afrobeat',102,2,"0:0:3:100 4:0:2:90 6:3:2:95 8:5:3:100 12:3:2:90 14:0:2:95 16:0:3:100 20:0:2:90 22:-2:2:95 24:-4:3:100 28:-2:2:90 30:0:2:95",0.08)
notes('afrobeat','Baixo','Baixo Groove Curto',102,2,"0:0:2:105 3:0:2:90 6:3:2:95 10:5:2:100 12:3:2:90 16:0:2:105 19:0:2:90 22:-2:2:95 26:-4:2:100 28:-2:4:90",0.08)
notes('afrobeat','Acordes','Guitarra Highlife',102,4,' '.join([n(i*16+s,ch,2,84) for i,ch in enumerate([Cm7,Cm7,[-7,-3,0,3],Cm7]) for s in (2,6,10,14)]),0.08)
notes('afrobeat','Acordes','Progressão Afro',102,4,chordbars([Cm7,Cm7,[-7,-3,0,3],Gm7],[(0,8),(8,8)],82),0.08)
notes('afrobeat','Melodia','Melodia Kalimba',102,4,"0:12:2:90 2:15:2:80 4:19:2:85 6:15:2:80 8:17:2:90 10:15:2:80 12:12:4:85 16:15:2:90 18:17:2:80 20:19:2:85 22:22:2:90 24:19:4:85 32:17:2:90 34:15:2:80 36:12:2:85 38:15:2:80 40:17:4:90 48:19:2:90 50:17:2:80 52:15:2:85 54:12:2:80 56:12:8:90",0.08)

# ------------------------------------------------------------ AMAPIANO
drums('amapiano','Amapiano Clássico',112,{'k':'x...x...x...x...|x...x...x...x...','s':'................|............x...','c':'....x.......x...|....x.......x...','h':'..x...x...x...x.|..x...x...x...x.','m':'x.xxx.xxx.xxx.xx|x.xxx.xxx.xxx.xx'},0.1)
drums('amapiano','Private School',110,{'k':'x.......x.......|x.......x.......','r':'....x.......x...|....x.......x...','h':'..x...x...x...x.|..x...x...x...x.','m':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.xo','o':'.......x.......x|.......x.......x'},0.08)
drums('amapiano','Shaker Groove',113,{'k':'x...x...x...x...|x...x...x...x...','s':'....x.......x...|....x.......x.x.','m':'xoxxxoxxxoxxxoxx|xoxxxoxxxoxxxoxx','o':'..x...x...x...x.|..x...x...x...x.','t':'................|..............x.'},0.12)
notes('amapiano','Baixo','Log Drum Clássico',112,2,"0:0:3:110 3:0:2:95 6:3:3:105 10:0:2:100 12:-2:3:105 16:0:3:110 19:0:2:95 22:5:3:105 26:3:2:100 28:0:3:105",0.1)
notes('amapiano','Baixo','Log Drum Melódico',112,2,"0:0:2:100 2:7:2:95 4:5:2:100 6:3:2:95 8:0:2:100 10:7:2:95 12:5:2:100 14:3:2:95 16:-2:2:100 18:5:2:95 20:3:2:100 22:2:2:95 24:-2:2:100 26:5:2:95 28:3:2:100 30:2:2:95",0.1)
notes('amapiano','Acordes','Piano Amapiano Min9',112,4,chordbars([Cm9,Fm9,Abm9,Gm9],[(0,3),(3,2),(6,3),(10,2),(12,4)],80),0.1)
notes('amapiano','Acordes','Piano Gospel',112,4,chordbars([Cm7,Fm7,Bb7,Eb7],[(0,6),(6,2),(8,4),(14,2)],80),0.1)
notes('amapiano','Melodia','Melodia Piano',112,4,"0:12:3:90 4:15:2:80 6:14:3:85 10:12:2:80 12:10:4:85 16:8:3:90 20:12:2:80 22:10:3:85 26:8:2:80 28:7:4:85 32:12:3:90 36:15:2:80 38:19:3:85 42:17:2:80 44:15:4:85 48:14:3:90 52:12:2:80 54:10:3:85 58:7:6:85",0.1)

# ------------------------------------------------------------ DRILL
drums('drill','Drill UK',144,{'k':'x.........x.....|..x......x......','s':'........x.......|........x.......','h':'x.xx.x.xx.x.xx.x|x.xx.x.xx.xx.x.x','o':'...............x|................'})
drums('drill','Drill Slide Hats',144,{'k':'x......x..x.....|x.....x...x.....','s':'........x.......|........x.......','h':'xxx.xxx.xxx.xxx.|xx.xx.xxxx.xx.xx','r':'................|..............x.'})
drums('drill','Chicago Drill',140,{'k':'x..x......x.....|x.......x.x.....','s':'........x.......|........x.......','c':'........x.......|........x.......','h':'x.x.x.x.x.x.xxxx|x.x.x.x.xxxxx.x.'})
notes('drill','Baixo','808 Slide Drill',144,2,"0:0:8:115 8:1:4:100 12:0:4:100 16:-2:8:115 24:-4:8:105")
notes('drill','Baixo','808 Dark Walk',144,2,"0:0:6:115 6:0:2:95 10:1:4:105 16:-2:6:115 22:-2:2:95 26:-4:4:105 30:-5:2:95")
notes('drill','Acordes','Acordes Frígios',144,4,chordbars([Cm,[1,5,8],Cm,Ab],[(0,16)],85))
notes('drill','Acordes','Cordas Tensas',144,4,chordbars([Cm,[1,5,8],Cm,[-2,2,5]],[(0,3),(3,3),(6,3),(10,3)],88))
notes('drill','Melodia','Melodia Sino Drill',144,4,"0:12:2:90 3:13:2:80 6:12:2:85 8:8:4:85 14:7:2:80 16:12:2:90 19:13:2:80 22:15:2:85 24:13:4:85 32:12:2:90 35:13:2:80 38:12:2:85 40:8:4:85 46:7:2:80 48:8:2:90 51:7:2:80 54:3:2:85 56:0:8:85")

# ------------------------------------------------------------ KIZOMBA
drums('kizomba','Kizomba Clássico',92,{'k':'x.....x.x.......|x.....x.x.......','s':'....x.......x...|....x.......x...','g':'x..x..x.x..x..x.|x..x..x.x..x..x.','m':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.'},0.06)
drums('kizomba','Zouk Love',90,{'k':'x.......x.......|x.......x.......','s':'....x.......x...|....x.......x...','h':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.','g':'..x...x...x...x.|..x...x...x...x.','r':'.......x.......x|.......x.......x'},0.05)
drums('kizomba','Kizomba Sensual',88,{'k':'x.....x...x.....|x.....x...x...x.','s':'....x.......x...|....x.......x...','j':'..x..x..x..x..x.|..x..x..x..x..x.','m':'o.o.o.o.o.o.o.o.|o.o.o.o.o.o.o.o.','h':'..............x.|..............x.'},0.06)
notes('kizomba','Baixo','Baixo Kizomba',92,2,"0:0:4:100 4:0:2:85 6:3:2:90 8:5:4:95 12:3:2:85 14:0:2:90 16:-4:4:100 20:-4:2:85 22:-2:2:90 24:0:4:95 28:-2:2:85 30:-5:2:90",0.06)
notes('kizomba','Baixo','Baixo Zouk Walking',92,2,"0:0:2:100 2:3:2:85 4:5:2:90 6:7:2:90 8:5:2:90 10:3:2:85 12:2:2:85 14:0:2:90 16:-4:2:100 18:-2:2:85 20:0:2:90 22:3:2:90 24:0:2:90 26:-2:2:85 28:-4:2:85 30:-5:2:90",0.06)
notes('kizomba','Acordes','Acordes Kizomba',92,4,chordbars([Cm7,Fm7,Bb7,Eb7],[(0,16)],78))
notes('kizomba','Acordes','Zouk Dedilhado',92,4,arp([Cm7,Fm7,Bb7,Eb7],[0,2,4,6,8,10,12,14],3,76))
notes('kizomba','Melodia','Melodia Romântica',92,4,"0:12:6:90 8:10:4:80 12:7:4:85 16:8:6:90 24:7:4:80 28:3:4:85 32:5:4:90 36:7:4:80 40:10:6:85 48:12:8:90 58:10:6:80",0.06)

# ------------------------------------------------------------ KUDURO
drums('kuduro','Kuduro Clássico',140,{'k':'x...x...x...x...|x...x...x...x...','s':'................|................','c':'....x.......x...|....x.......x...','h':'..x...x...x...x.|..x...x...x...x.','o':'..x...x...x...x.|..x...x...x...x.','t':'................|..........x.x.xx'})
drums('kuduro','Kuduro Rápido',146,{'k':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.','s':'....x.......x.x.|....x.......x.xx','h':'xxxxxxxxxxxxxxxx|xxxxxxxxxxxxxxxx'})
drums('kuduro','Kuduro Tribal',140,{'k':'x...x...x...x...|x...x...x...x...','g':'x..x..x...x.x...|x..x..x...x.x...','j':'..x..x..x..x..x.|..x..x..x..x..x.','b':'.x..x..x.x..x...|.x..x..x.x..x...','c':'....x.......x...|....x.......x...','m':'x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.'},0.04)
notes('kuduro','Baixo','Baixo Kuduro',140,2,"0:0:2:110 2:0:2:95 4:0:2:105 6:3:2:100 8:5:2:105 10:3:2:95 12:0:2:105 14:-2:2:95 16:0:2:110 18:0:2:95 20:0:2:105 22:3:2:100 24:7:2:105 26:5:2:95 28:3:2:105 30:0:2:95")
notes('kuduro','Baixo','Baixo Ácido',140,2,' '.join(n(i,[0,0,12,0,3,0,15,7][i%8] if i<16 else [0,0,12,0,-2,-2,10,5][i%8],1,95 if i%2 else 110) for i in range(32)))
notes('kuduro','Acordes','Stabs Kuduro',140,4,' '.join(n(i*16+s,ch,2,100) for i,ch in enumerate([Cm,Cm,Ab,Bb]) for s in (2,6,10,14)))
notes('kuduro','Acordes','Progressão Dance',140,4,chordbars([Cm,Ab,Bb,Gm],[(0,8),(8,8)],90))
notes('kuduro','Melodia','Melodia Sirene',140,4,"0:12:4:100 4:15:4:95 8:19:8:100 16:17:4:100 20:15:4:95 24:12:8:100 32:12:4:100 36:15:4:95 40:19:4:100 44:22:4:100 48:24:12:105")

def esc(s): return s.replace('\\','\\\\').replace('"','\\"')
out=[]
for st,kind,name,bpm,bars,sw,dsl in G:
    out.append(f'    {{ "{st}", "{kind}", TXT("{esc(name)}"), {bpm}, {bars}, {sw}f, "{dsl}" }},')
open(sys.argv[1],'w',encoding='utf-8').write('\n'.join(out)+'\n')
print(len(G),'grooves')
