"""Fase ORIGINAL "HANGAR" (inspirada no espírito do E1M1: sala inicial, corredor, grande salão com poço de ácido
e passarela, salas laterais e sala de saída). Nao copia a geometria de nenhum mapa da id Software.
Legenda: '#' parede de pedra/metal, 'L' parede luminosa (rocha de carne), '.' piso, 'A' ácido (piso que fere), 'P' início,
'E' inimigo, 'H' kit médico, 'X' saída."""
import math
W=26; HH=26           # largura x altura do mapa em células (<= 32)
T=64                  # unidades por célula
H=64                  # altura da parede
def _build():
    g=[['#']*W for _ in range(HH)]
    def carve(x0,z0,x1,z1,c='.'):
        for z in range(z0,z1+1):
            for x in range(x0,x1+1): g[z][x]=c
    carve(1,19,7,24)                    # sala inicial (SO)
    carve(3,12,4,18)                    # corredor norte (2 de largura)
    carve(5,7,20,17)                    # grande salão
    carve(9,10,16,14,'A')               # poço de ácido
    carve(12,10,13,14)                  # passarela sobre o ácido
    for x,z in [(7,9),(7,15),(18,9),(18,15),(11,8),(14,16)]: g[z][x]='#'        # pilares
    carve(1,6,3,10)                     # ala oeste
    carve(4,9,4,9)                      # passagem da ala oeste para o salão (entre x=4 e o salão)
    carve(15,18,16,18)                  # porta sul do salão
    carve(12,19,20,24)                  # sala sul
    carve(21,15,21,16)                  # porta leste
    carve(22,13,24,20)                  # sala leste
    carve(21,8,22,12)                   # corredor para a sala de saída
    carve(19,1,24,6)                    # sala de saída (NE)
    for x,z in [(1,21),(1,22),(7,20),(12,22),(20,21),(24,16),(24,3),(19,3),(10,6),(15,6),(5,17),(20,10)]:
        if g[z][x]=='#': g[z][x]='L'
    for x,z in [(3,22)]: g[z][x]='P'
    for x,z in [(6,11),(8,16),(19,12),(17,8),(14,22),(18,19),(23,17),(23,14),(21,4),(22,6)]: g[z][x]='E'
    for x,z in [(2,7),(18,22),(24,19),(20,2)]: g[z][x]='H'
    g[3][22]='X'
    return [''.join(r) for r in g]
MAP=_build()
def solid(x,z): return not(0<=z<HH and 0<=x<W) or MAP[z][x] in '#L'
def acid(x,z): return 0<=z<HH and 0<=x<W and MAP[z][x]=='A'
def entities():
    P=None;E=[];Hs=[];X=None
    for z,row in enumerate(MAP):
        for x,c in enumerate(row):
            ctr=(x*T+T//2,z*T+T//2)
            if c=='P':P=ctr
            elif c=='E':E.append(ctr)
            elif c=='H':Hs.append(ctr)
            elif c=='X':X=(x,z)
    return P,E,Hs,X

# --- verificacao: conectividade (BFS) e contagens
def check():
    P,E,Hs,X=entities(); start=(P[0]//T,P[1]//T); seen={start}; q=[start]
    while q:
        x,z=q.pop()
        for dx,dz in((1,0),(-1,0),(0,1),(0,-1)):
            n=(x+dx,z+dz)
            if n not in seen and not solid(*n): seen.add(n); q.append(n)
    opens=[(x,z) for z in range(HH) for x in range(W) if not solid(x,z)]
    unreachable=[c for c in opens if c not in seen]
    assert not unreachable,('celulas inalcancaveis',unreachable[:5])
    assert X in seen
    for e in E+Hs: assert (e[0]//T,e[1]//T) in seen
    return len(opens),len(E),len(Hs)
if __name__=='__main__':
    print('\n'.join(MAP)); print(check())
