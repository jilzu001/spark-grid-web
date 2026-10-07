import random
from pathlib import Path
out=Path('data/mobile');out.mkdir(exist_ok=True)
counts=[1,1,2,2,3,3,4,5,6,8]
spawns=[(9,9),(9,3),(7,11),(7,5),(5,9),(9,7),(5,5),(7,1)]
for level,count in enumerate(counts,1):
    rng=random.Random(8200+level)
    board=[[1]*19 for _ in range(13)]
    for y in range(1,12):
        for x in range(1,10):
            board[y][x]=1 if x%2==0 and y%2==0 else (2 if rng.random()<.10+.035*(level-1) else 0)
    for x,y in [(1,1),(2,1),(3,1),(1,2),(1,3),(2,3),(3,3)]:board[y][x]=0
    for x,y in spawns[:count]:
        board[y][x]=0
        for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)]:
            if 0<x+dx<10 and 0<y+dy<12 and board[y+dy][x+dx]!=1:board[y+dy][x+dx]=0
    board[11][9]=3
    text='19 13\n'+'\n'.join(' '.join(map(str,row)) for row in board)+'\n1 1 '+str(count)+'\n'+'\n'.join(f'{x} {y}' for x,y in spawns[:count])+'\n'
    (out/f'stage{level:02}.txt').write_text(text,encoding='ascii')
