#!/usr/bin/env python3
"""Campanha dos humanos (8 missoes): parametros e textos (briefing, objetivo) em 4 idiomas.
Cada missao vira 6 textos consecutivos (titulo, 4 linhas do briefing, objetivo) em tools/i18n.py e um registro de 16 bytes em
src/mission_table.h (lido pelo banco 53). Objetivos: DESTROY (derrubar a fortaleza inimiga), SURVIVE (segundos), KILLS,
GOLD (ouro em caixa), BUILD (tipo | quantidade<<8)."""
OBJ={'DESTROY':0,'SURVIVE':1,'KILLS':2,'GOLD':3,'BUILD':4}
# wood, gold, primeira onda (s), intervalo (s), reforcos leves (0/1), IA ligada, objetivo, parametro, limite (min, 0 = sem)
MISSIONS=[
 dict(map=1,wood=60,gold=40,first=240,gap=60,extras=0,ai=1,obj='BUILD',param=2|(1<<8),limit=0,
  title=('MISSÃO 1: O PRIMEIRO AVISO','MISSION 1: THE FIRST WARNING','MISIÓN 1: EL PRIMER AVISO','だい1わ: さいしょのしらせ'),
  brief=('Sombras marcham do deserto. Construa um quartel para treinar soldados antes que cheguem.',
         'Shadows march from the desert. Build a barracks to train soldiers before they arrive.',
         'Sombras marchan desde el desierto. Construye un cuartel para entrenar soldados antes de que lleguen.',
         'さばくから かげが せめてくる。 へいしゃを たてて へいたいを そだてよ。'),
  goal=('META: CONSTRUIR UM QUARTEL','GOAL: BUILD A BARRACKS','META: CONSTRUIR UN CUARTEL','もくひょう: へいしゃをたてる')),
 dict(map=2,wood=40,gold=30,first=60,gap=45,extras=0,ai=1,obj='SURVIVE',param=360,limit=0,
  title=('MISSÃO 2: A PRIMEIRA ONDA','MISSION 2: THE FIRST WAVE','MISIÓN 2: LA PRIMERA OLA','だい2わ: さいしょのなみ'),
  brief=('Os mortos atacam o reino. Segure as muralhas por seis minutos até a ajuda chegar.',
         'The dead attack the kingdom. Hold the walls for six minutes until help arrives.',
         'Los muertos atacan el reino. Resiste seis minutos hasta que llegue la ayuda.',
         'しものたちが おそってくる。 たすけが くるまで 6ぷん まもりぬけ。'),
  goal=('META: SOBREVIVER 6 MINUTOS','GOAL: SURVIVE 6 MINUTES','META: SOBREVIVIR 6 MINUTOS','もくひょう: 6ぷんたえろ')),
 dict(map=3,wood=80,gold=60,first=90,gap=45,extras=1,ai=1,obj='KILLS',param=12,limit=0,
  title=('MISSÃO 3: CONTRA-ATAQUE','MISSION 3: COUNTERATTACK','MISIÓN 3: CONTRAATAQUE','だい3わ: はんげき'),
  brief=('Reunimos tropas. Esmague as ondas inimigas e prove que o reino ainda vive.',
         'We have gathered troops. Crush the enemy waves and prove the kingdom still lives.',
         'Hemos reunido tropas. Aplasta las oleadas enemigas y demuestra que el reino vive.',
         'へいが あつまった。 てきのなみを うちたおし おうこくの ちからを しめせ。'),
  goal=('META: DERROTAR 12 INIMIGOS','GOAL: DEFEAT 12 ENEMIES','META: DERROTAR 12 ENEMIGOS','もくひょう: 12たいたおせ')),
 dict(map=4,wood=100,gold=60,first=100,gap=45,extras=1,ai=1,obj='GOLD',param=500,limit=0,
  title=('MISSÃO 4: OURO PARA A GUERRA', 'MISSION 4: GOLD FOR WAR', 'MISIÓN 4: ORO PARA LA GUERRA', 'だい4わ: たたかいのこがね'),
  brief=('Os cofres do reino estão vazios. Extraia ouro das minas para financiar a campanha.', 'The kingdom coffers are empty. Mine gold to fund the campaign.', 'Las arcas del reino están vacías. Extrae oro de las minas para financiar la campaña.', 'くらは からっぽだ。 こうざんで きんを ほり いくさの しきんを あつめよ。'),
  goal=('META: JUNTAR 500 DE OURO', 'GOAL: HOLD 500 GOLD', 'META: JUNTAR 500 DE ORO', 'もくひょう: きん500をあつめろ')),
 dict(map=5,wood=120,gold=80,first=200,gap=50,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 5: A FORTALEZA DE AREIA','MISSION 5: THE SAND KEEP','MISIÓN 5: LA FORTALEZA DE ARENA','だい5わ: すなのとりで'),
  brief=('Os batedores acharam a fortaleza dos mortos no sul. Marche e derrube seus portões.',
         'Scouts found the keep of the dead in the south. March and break down its gates.',
         'Los exploradores hallaron la fortaleza de los muertos al sur. Marcha y derriba sus puertas.',
         'みなみに しものたちの とりでが ある。 すすんで もんを うちやぶれ。'),
  goal=('META: DESTRUIR A FORTALEZA','GOAL: DESTROY THE ENEMY KEEP','META: DESTRUIR LA FORTALEZA','もくひょう: てきのしろをたおせ')),
 dict(map=6,wood=150,gold=100,first=120,gap=40,extras=1,ai=1,obj='SURVIVE',param=600,limit=0,
  title=('MISSÃO 6: A LONGA NOITE', 'MISSION 6: THE LONG NIGHT', 'MISIÓN 6: LA LARGA NOCHE', 'だい6わ: ながいよる'),
  brief=('A noite cai sobre o deserto e os mortos não param. Resista dez minutos até o amanhecer.', 'Night falls over the desert and the dead never stop. Endure ten minutes until dawn.', 'La noche cae sobre el desierto y los muertos no cesan. Resiste diez minutos hasta el amanecer.', 'さばくに よるが おり しものは とまらない。 よあけまで 10ぷん たえぬけ。'),
  goal=('META: SOBREVIVER 10 MINUTOS', 'GOAL: SURVIVE 10 MINUTES', 'META: SOBREVIVIR 10 MINUTOS', 'もくひょう: 10ぷんたえろ')),
 dict(map=7,wood=180,gold=120,first=100,gap=35,extras=1,ai=1,obj='KILLS',param=25,limit=0,
  title=('MISSÃO 7: O CERCO', 'MISSION 7: THE SIEGE', 'MISIÓN 7: EL CERCO', 'だい7わ: ほうい'),
  brief=('Os mortos cercam as fronteiras. Rompa o cerco derrotando seus guerreiros.', 'The dead surround the borders. Break the siege by defeating their warriors.', 'Los muertos rodean las fronteras. Rompe el cerco derrotando a sus guerreros.', 'しものたちが こっきょうを かこんだ。 せんしを たおして ほういを やぶれ。'),
  goal=('META: DERROTAR 25 INIMIGOS', 'GOAL: DEFEAT 25 ENEMIES', 'META: DERROTAR 25 ENEMIGOS', 'もくひょう: 25たいたおせ')),
 dict(map=8,wood=150,gold=100,first=150,gap=40,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 8: O REI INSEPULTO','MISSION 8: THE UNBURIED KING','MISIÓN 8: EL REY INSEPULTO','だい8わ: ほうむられざるおう'),
  brief=('O rei insepulto desperta. Reúna todo o exército e acabe com a maldição de uma vez.',
         'The unburied king awakens. Gather the whole army and end the curse once and for all.',
         'El rey insepulto despierta. Reúne todo el ejército y acaba con la maldición.',
         'ほうむられざるおうが めざめた。 ぜんぐんで のろいを たちきれ。'),
  goal=('META: DESTRUIR A FORTALEZA','GOAL: DESTROY THE ENEMY KEEP','META: DESTRUIR LA FORTALEZA','もくひょう: てきのしろをたおせ')),
]
MISSIONS_U=[
 dict(map=1,wood=60,gold=40,first=240,gap=60,extras=0,ai=1,obj='BUILD',param=258,limit=0,
  title=('MISSÃO 1: O DESPERTAR', 'MISSION 1: THE AWAKENING', 'MISIÓN 1: EL DESPERTAR', 'だい1わ: めざめ'),
  brief=('As areias se abrem e os mortos despertam. Erga uma cripta antes que os vivos reajam.', 'The sands part and the dead awaken. Raise a crypt before the living react.', 'Las arenas se abren y los muertos despiertan. Alza una cripta antes de que actúen los vivos.', 'すなが ひらき しものたちが めざめた。 いきるものが うごくまえに クリプトを たてよ。'),
  goal=('META: CONSTRUIR UMA CRIPTA', 'GOAL: BUILD A CRYPT', 'META: CONSTRUIR UNA CRIPTA', 'もくひょう: クリプトをたてる')),
 dict(map=2,wood=40,gold=30,first=60,gap=45,extras=0,ai=1,obj='KILLS',param=10,limit=0,
  title=('MISSÃO 2: A COLHEITA DE ALMAS', 'MISSION 2: THE HARVEST OF SOULS', 'MISIÓN 2: LA COSECHA DE ALMAS', 'だい2わ: たましいのしゅうかく'),
  brief=('Os vivos mandam patrulhas ao deserto. Cada soldado vencido fortalece os mortos.', 'The living send patrols into the desert. Every fallen soldier strengthens the dead.', 'Los vivos envían patrullas al desierto. Cada soldado caído fortalece a los muertos.', 'いきるものの はいかが さばくに きた。 たおすたびに ほうむられざるぐんは つよくなる。'),
  goal=('META: DERROTAR 10 INIMIGOS', 'GOAL: DEFEAT 10 ENEMIES', 'META: DERROTAR 10 ENEMIGOS', 'もくひょう: 10たいたおせ')),
 dict(map=3,wood=80,gold=60,first=90,gap=45,extras=1,ai=1,obj='SURVIVE',param=360,limit=0,
  title=('MISSÃO 3: A CONTRA-MARCHA', 'MISSION 3: THE COUNTER-MARCH', 'MISIÓN 3: LA CONTRAMARCHA', 'だい3わ: ぎゃくしゅう'),
  brief=('Cavaleiros caem sobre nossas tumbas. Resista seis minutos até a noite trazer reforços.', 'Knights fall on our tombs. Hold six minutes until night brings reinforcements.', 'Caballeros caen sobre nuestras tumbas. Resiste seis minutos hasta la llegada de la noche.', 'きしだんが はかに おそいかかる。 よるが ふえいを つれてくるまで 6ぷん たえよ。'),
  goal=('META: SOBREVIVER 6 MINUTOS', 'GOAL: SURVIVE 6 MINUTES', 'META: SOBREVIVIR 6 MINUTOS', 'もくひょう: 6ぷんたえろ')),
 dict(map=4,wood=100,gold=60,first=100,gap=45,extras=1,ai=1,obj='GOLD',param=500,limit=0,
  title=('MISSÃO 4: O TRIBUTO DE OURO', 'MISSION 4: THE TRIBUTE OF GOLD', 'MISIÓN 4: EL TRIBUTO DE ORO', 'だい4わ: こがねのみつぎ'),
  brief=('O faraó exige ouro dos vales conquistados. Escravos às minas até encher o tesouro.', 'The pharaoh demands gold from the conquered valleys. Slaves to the mines until the vault is full.', 'El faraón exige oro de los valles conquistados. Esclavos a las minas hasta llenar el tesoro.', 'ファラオは せいふくした たにから きんを もとめる。 どれいに こうざんを ほらせ ほうこを みたせ。'),
  goal=('META: JUNTAR 500 DE OURO', 'GOAL: HOLD 500 GOLD', 'META: JUNTAR 500 DE ORO', 'もくひょう: きん500をあつめろ')),
 dict(map=5,wood=120,gold=80,first=200,gap=50,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 5: A MARCHA SOBRE O REINO', 'MISSION 5: THE MARCH ON THE KINGDOM', 'MISIÓN 5: LA MARCHA SOBRE EL REINO', 'だい5わ: おうこくへの しんぐん'),
  brief=('O deserto ficou pequeno. Marche ao norte e derrube a fortaleza dos vivos.', 'The desert grew too small. March north and tear down the fortress of the living.', 'El desierto se quedó pequeño. Marcha al norte y derriba la fortaleza de los vivos.', 'さばくは せまくなった。 きたへ すすみ いきるものの とりでを たおして おうこくへの みちを ひらけ。'),
  goal=('META: DESTRUIR A FORTALEZA', 'GOAL: DESTROY THE ENEMY KEEP', 'META: DESTRUIR LA FORTALEZA', 'もくひょう: てきのしろをたおせ')),
 dict(map=6,wood=150,gold=100,first=120,gap=40,extras=1,ai=1,obj='SURVIVE',param=600,limit=0,
  title=('MISSÃO 6: A NOITE SEM FIM', 'MISSION 6: THE ENDLESS NIGHT', 'MISIÓN 6: LA NOCHE SIN FIN', 'だい6わ: おわらぬよる'),
  brief=('Os sinos do reino tocam a rebate. Segure a posição por dez minutos de escuridão.', 'The kingdom bells ring the alarm. Hold your ground for ten minutes of darkness.', 'Las campanas del reino tocan a rebato. Mantén la posición diez minutos de oscuridad.', 'おうこくの かねが なりひびく。 やみが だいちを おおう 10ぷんかん もちこたえよ。'),
  goal=('META: SOBREVIVER 10 MINUTOS', 'GOAL: SURVIVE 10 MINUTES', 'META: SOBREVIVIR 10 MINUTOS', 'もくひょう: 10ぷんたえろ')),
 dict(map=7,wood=180,gold=120,first=100,gap=35,extras=1,ai=1,obj='KILLS',param=25,limit=0,
  title=('MISSÃO 7: O CERCO DA CAPITAL', 'MISSION 7: THE SIEGE OF THE CAPITAL', 'MISIÓN 7: EL CERCO DE LA CAPITAL', 'だい7わ: しゅとの ほうい'),
  brief=('As muralhas da capital se erguem. Quebre seus defensores e aperte o cerco.', 'The walls of the capital rise. Break its defenders and tighten the siege.', 'Las murallas de la capital se alzan. Quiebra a sus defensores y cierra el cerco.', 'しゅとの じょうへきが めのまえに ある。 しゅびへいを うちくだき ほういを せばめよ。'),
  goal=('META: DERROTAR 25 INIMIGOS', 'GOAL: DEFEAT 25 ENEMIES', 'META: DERROTAR 25 ENEMIGOS', 'もくひょう: 25たいたおせ')),
 dict(map=8,wood=150,gold=100,first=150,gap=40,extras=1,ai=1,obj='DESTROY',param=0,limit=0,
  title=('MISSÃO 8: O TRONO DOS VIVOS', 'MISSION 8: THE THRONE OF THE LIVING', 'MISIÓN 8: EL TRONO DE LOS VIVOS', 'だい8わ: いきるものの おうざ'),
  brief=('O último rei dos vivos treme no trono. Reúna a legião e apague a última chama.', 'The last king of the living trembles on his throne. Gather the legion and snuff the last flame.', 'El último rey de los vivos tiembla en su trono. Reúne la legión y apaga la última llama.', 'さいごの いきるものの おうが おうざで ふるえる。 ぜんぐんで おうこくの さいごの ひを けせ。'),
  goal=('META: DESTRUIR A FORTALEZA', 'GOAL: DESTROY THE ENEMY KEEP', 'META: DESTRUIR LA FORTALEZA', 'もくひょう: てきのしろをたおせ')),
]
ALL=MISSIONS+MISSIONS_U   # 8 dos humanos seguidos de 8 dos Insepultos
def table():
    out=bytearray()
    for i,m in enumerate(ALL):
        r=bytearray(16)
        r[0]=m['wood'];r[1]=m['gold'];r[2:4]=m['first'].to_bytes(2,'little');r[4]=m['gap'];r[5]=m['extras'];r[6]=m['ai']
        r[7]=OBJ[m['obj']];r[8:10]=m['param'].to_bytes(2,'little');r[10]=m['limit'];r[12]=m['map']
        # r[11] = id do primeiro texto (preenchido pelo i18n.py)
        out+=r
    return out
