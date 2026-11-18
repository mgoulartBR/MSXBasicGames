#!/usr/bin/env python3
"""Linha do tempo da IA inimiga (economia, obras, treino, ondas) para ajustar o equilibrio sem jogar.
Compara os parametros antigos e os atuais supondo que o jogador derrota cada onda em N s. uso: ai_timeline.py"""
import sys
BW=[0,16,24,24,24,32,24,40];BG=[0,0,16,8,16,24,8,24];RW=[0,8,8,8,16,24];RG=[8,8,8,12,24,16]
ORD=[2,3,1,6,5,7,1,6]
def sim(first,cool,tr_lo,tr_hi,gold_mod,table,kill=35,end=900,extras=None):
    """extras: tabela de 12 tipos (2 por onda) dos reforcos leves (atores 10 e 11), ou None para so 2 soldados."""
    wood,gold=60,40;step=0;site=None;left=0;built=set();have=lambda t:t in built
    n=4 if extras else 2
    state=[0,0]+([2,2] if extras else []);timer=[0]*4;kind=[6,6,1,1];wave=0;launch=first;log=[];wave_end=None
    for t in range(1,end):
        wood+=1+have(3)
        gold+= (t&1) if gold_mod==2 else (1 if t%gold_mod else 0)
        wood=min(wood,400);gold=min(gold,400)
        if site is not None:
            left-=1
            if left==0:built.add(site);site=None
        elif step<8:
            ty=ORD[step]
            if wood>=BW[ty] and gold>=BG[ty]:wood-=BW[ty];gold-=BG[ty];site=ty;left=15;step+=1
        if wave_end is not None and t>=wave_end:
            for s in range(2):state[s]=2
            for s in range(2,n):
                if state[s]==1:state[s]=2
            wave_end=None
        for s in range(n):
            if state[s]==2:
                if s<2:k=table[min(wave-1,5)*2+s] if wave else table[s]
                else:
                    if state[0]==2 or state[1]==2:continue
                    k=extras[min(wave,5)*2+s-2]
                while k>1 and not have({5:7,4:5}.get(k,2)):k={5:4,4:2,3:1,2:1}[k]
                if not have(2):continue
                if wood>=RW[k] and gold>=RG[k]:wood-=RW[k];gold-=RG[k];kind[s]=k;timer[s]=tr_hi if k>=4 else tr_lo;state[s]=3
            elif state[s]==3:
                if timer[s]:timer[s]-=1
                else:state[s]=0
        if state[0]==0 and state[1]==0 and t>=launch:
            wave+=1;state[0]=state[1]=1
            for s in range(2,n):
                if state[s]==0:state[s]=1
            launch=t+cool;wave_end=t+kill;log.append((t,[kind[s] for s in range(n) if state[s]==1]))
    return log
NAME={1:'sw',2:'sp',3:'ar',4:'kn',5:'ram',6:'sk'}
def show(lbl,*a,**k):
    L=sim(*a,**k);print(lbl);print('  '+' | '.join('%d:%02d %s'%(t//60,t%60,'+'.join(NAME[x] for x in kd)) for t,kd in L[:9]))
old=[1,1,2,3,4,1,4,3,4,4,5,4]
new=[2,3,4,1,4,3,4,4,5,4,4,4]
extra=[1,3, 1,3, 1,3, 3,3, 3,3, 3,3]
for kill in (35,60):
    print('== humano derrota cada onda em %d s'%kill)
    show('antes   (2 soldados, parametros de 200 s)',200,60,20,30,2,old,kill=kill)
    show('2 soldados (parametros atuais)           ',150,40,15,25,3,new,kill=kill)
    show('4 soldados (com reforcos leves)          ',150,40,15,25,3,new,kill=kill,extras=extra)
