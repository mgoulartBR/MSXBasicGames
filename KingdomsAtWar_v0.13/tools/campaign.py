#!/usr/bin/env python3
"""Campanha dos humanos (5 missoes): parametros e textos (briefing, objetivo) em 4 idiomas.
Cada missao vira 6 textos consecutivos (titulo, 4 linhas do briefing, objetivo) em tools/i18n.py e um registro de 16 bytes em
src/mission_table.h (lido pelo banco 53). Objetivos: DESTROY (derrubar a fortaleza inimiga), SURVIVE (segundos), KILLS,
GOLD (ouro em caixa), BUILD (tipo | quantidade<<8)."""
OBJ={'DESTROY':0,'SURVIVE':1,'KILLS':2,'GOLD':3,'BUILD':4}
# wood, gold, primeira onda (s), intervalo (s), reforcos leves (0/1), IA ligada, objetivo, parametro, limite (min, 0 = sem)
MISSIONS=[
 dict(wood=60,gold=40,first=240,gap=60,extras=0,ai=1,obj='BUILD',param=2|(1<<8),limit=0,
  title=('MISSÃO 1: O PRIMEIRO AVISO','MISSION 1: THE FIRST WARNING','MISIÓN 1: EL PRIMER AVISO','だい1わ: さいしょのしらせ'),
  brief=('Sombras marcham do deserto. Construa um quartel para treinar soldados antes que cheguem.',
         'Shadows march from the desert. Build a barracks to train soldiers before they arrive.',
         'Sombras marchan desde el desierto. Construye un cuartel para entrenar soldados antes de que lleguen.',
         'さばくから かげが せめてくる。 へいしゃを たてて へいたいを そだてよ。'),
  goal=('META: CONSTRUIR UM QUARTEL','GOAL: BUILD A BARRACKS','META: CONSTRUIR UN CUARTEL','もくひょう: へいしゃをたてる')),
 dict(wood=40,gold=30,first=60,gap=45,extras=0,ai=1,obj='SURVIVE',param=360,limit=0,
  title=('MISSÃO 2: A PRIMEIRA ONDA','MISSION 2: THE FIRST WAVE','MISIÓN 2: LA PRIMERA OLA','だい2わ: さいしょのなみ'),
  brief=('Os mortos atacam o reino. Segure as muralhas por seis minutos até a ajuda chegar.',
         'The dead attack the kingdom. Hold the walls for six minutes until help arrives.',
         'Los muertos atacan el reino. Resiste seis minutos hasta que llegue la ayuda.',
         'しものたちが おそってくる。 たすけが くるまで 6ぷん まもりぬけ。'),
  goal=('META: SOBREVIVER 6 MINUTOS','GOAL: SURVIVE 6 MINUTES','META: SOBREVIVIR 6 MINUTOS','もくひょう: 6ぷんたえろ')),
 dict(wood=80,gold=60,first=90,gap=45,extras=1,ai=1,obj='KILLS',param=12,limit=0,
  title=('MISSÃO 3: CONTRA-ATAQUE','MISSION 3: COUNTERATTACK','MISIÓN 3: CONTRAATAQUE','だい3わ: はんげき'),
  brief=('Reunimos tropas. Esmague as ondas inimigas e prove que o reino ainda vive.',
         'We have gathered troops. Crush the enemy waves and prove the kingdom still lives.',
         'Hemos reunido tropas. Aplasta las oleadas enemigas y demuestra que el reino vive.',
         'へいが あつまった。 てきのなみを うちたおし おうこくの ちからを しめせ。'),
  goal=('META: DERROTAR 12 INIMIGOS','GOAL: DEFEAT 12 ENEMIES','META: DERROTAR 12 ENEMIGOS','もくひょう: 12たいたおせ')),
 dict(wood=120,gold=80,first=200,gap=50,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 4: A FORTALEZA DE AREIA','MISSION 4: THE SAND KEEP','MISIÓN 4: LA FORTALEZA DE ARENA','だい4わ: すなのとりで'),
  brief=('Os batedores acharam a fortaleza dos mortos no sul. Marche e derrube seus portões.',
         'Scouts found the keep of the dead in the south. March and break down its gates.',
         'Los exploradores hallaron la fortaleza de los muertos al sur. Marcha y derriba sus puertas.',
         'みなみに しものたちの とりでが ある。 すすんで もんを うちやぶれ。'),
  goal=('META: DESTRUIR A FORTALEZA','GOAL: DESTROY THE ENEMY KEEP','META: DESTRUIR LA FORTALEZA','もくひょう: てきのしろをたおせ')),
 dict(wood=150,gold=100,first=150,gap=40,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 5: O REI INSEPULTO','MISSION 5: THE UNBURIED KING','MISIÓN 5: EL REY INSEPULTO','だい5わ: ほうむられざるおう'),
  brief=('O rei insepulto desperta. Reúna todo o exército e acabe com a maldição de uma vez.',
         'The unburied king awakens. Gather the whole army and end the curse once and for all.',
         'El rey insepulto despierta. Reúne todo el ejército y acaba con la maldición.',
         'ほうむられざるおうが めざめた。 ぜんぐんで のろいを たちきれ。'),
  goal=('META: DESTRUIR A FORTALEZA','GOAL: DESTROY THE ENEMY KEEP','META: DESTRUIR LA FORTALEZA','もくひょう: てきのしろをたおせ')),
]
def table():
    out=bytearray()
    for i,m in enumerate(MISSIONS):
        r=bytearray(16)
        r[0]=m['wood'];r[1]=m['gold'];r[2:4]=m['first'].to_bytes(2,'little');r[4]=m['gap'];r[5]=m['extras'];r[6]=m['ai']
        r[7]=OBJ[m['obj']];r[8:10]=m['param'].to_bytes(2,'little');r[10]=m['limit']
        # r[11] = id do primeiro texto (preenchido pelo i18n.py)
        out+=r
    return out
