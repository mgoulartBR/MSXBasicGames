10 CLEAR
20 DIM I$(10): I=0
30 S=1: CH=0: ER=0
40 GOSUB 1000
50 PRINT ">"; : INPUT C$

70 IF C$="AJUDA" THEN GOSUB 2000:GOTO 50
80 IF C$="OLHAR" THEN GOSUB 1000:GOTO 50
90 IF C$="INVENTARIO" THEN GOSUB 3000:GOTO 50
100 IF LEFT$(C$,3)="IR " THEN D$=MID$(C$,4):GOSUB 4000:GOTO 50
110 IF LEFT$(C$,6)="PEGAR " THEN T$=MID$(C$,7):GOSUB 5000:GOTO 50
120 IF LEFT$(C$,5)="USAR " THEN T$=MID$(C$,6):GOSUB 6000:GOTO 50
130 PRINT "Nao entendi o comando. Digite 'AJUDA' para ajuda.":GOTO 50

1000 'DESCRIÇÃO DAS SALAS
1010 CLS
1020 IF S=1 THEN PRINT "Voce esta em uma cela escura. Uma porta trancada esta ao norte.":IF CH=0 THEN PRINT "Ha uma chave enferrujada no chao.":RETURN
1030 IF S=2 THEN PRINT "Voce esta em um corredor iluminado por tochas.":PRINT "Passagens: sul e leste.":RETURN
1040 IF S=3 THEN PRINT "Uma sala com paredes cobertas por runas.":PRINT "Ao norte, uma porta com um enigma gravado.":PRINT "Charada: 'Duas vezes tres mais um sao quantos dedos de uma mao?'":RETURN
1050 IF S=4 THEN PRINT "Voce entrou numa sala reluzente. O Cristal do Sol brilha diante de voce!":PRINT:PRINT "*** VOCE VENCEU O JOGO! ***":END
1060 RETURN

2000 'AJUDA
2010 PRINT "Comandos:"
2015 PRINT "Digite os comandos em letras MAIUSCULAS"
2020 PRINT "- IR <DIRECAO> (ex: IR NORTE)"
2030 PRINT "- PEGAR <ITEM>"
2040 PRINT "- USAR <ITEM>"
2050 PRINT "- OLHAR"
2060 PRINT "- INVENTARIO"
2070 RETURN

3000 'INVENTARIO
3010 IF I=0 THEN PRINT "Voce nao tem nada.":RETURN
3020 PRINT "Voce tem:"
3030 FOR J=1 TO I: PRINT "- ";I$(J): NEXT J
3040 RETURN

4000 'MOVIMENTO
4010 IF S=1 AND D$="NORTE" THEN IF CH THEN S=2: RETURN ELSE PRINT "A porta esta trancada.":RETURN
4020 IF S=2 AND D$="SUL" THEN S=1: RETURN
4030 IF S=2 AND D$="LESTE" THEN S=3: RETURN
4040 IF S=3 AND D$="OESTE" THEN S=2: RETURN
4050 IF S=3 AND D$="NORTE" THEN IF ER THEN S=4: RETURN ELSE GOSUB 7000:RETURN
4060 PRINT "Nao ha caminho nessa direcao.":RETURN

5000 'PEGAR ITENS
5010 IF S=1 AND T$="CHAVE" AND CH=0 THEN I=I+1:I$(I)="CHAVE":CH=1:PRINT "Voce pegou a chave.":RETURN
5020 PRINT "Nao ha isso aqui.":RETURN

6000 'USAR ITENS
6010 IF T$="CHAVE" AND S=1 THEN IF CH THEN PRINT "Voce destrancou a porta ao norte.":RETURN
6020 PRINT "Nao funcionou.":RETURN

7000 'ENIGMA
7010 PRINT "Charada: 'Duas vezes tres mais um sao quantos dedos de uma mao?'"
7020 INPUT "Sua resposta: ";R$
7030 IF R$="7" THEN PRINT "A porta ao norte se abriu com um clique!":ER=1:RETURN
7040 PRINT "Nada acontece... talvez a resposta esteja errada.":RETURN
