10 ' JOGO DA VELHA PARA MSX-BASIC
20 CLS
30 PRINT "================================"
40 PRINT "     JOGO DA VELHA - MSX        "
50 PRINT " Dois jogadores: X e O          "
60 PRINT " Escolha a posicao de 1 a 9:    "
70 PRINT " (1 a 3 = linha 1; 4 a 6 = linha 2; 7 a 9 = linha 3)"
80 PRINT "================================"
90 DIM T$(9): FOR I=1 TO 9: T$(I)=" ": NEXT I
100 P$="X"
110 GOSUB 300
120 PRINT:PRINT "Jogador ";P$;" escolha a casa (1-9): ";
130 INPUT C
140 IF C<1 OR C>9 THEN PRINT "Posicao invalida!":GOTO 120
150 IF T$(C)<>" " THEN PRINT "Casa ocupada!":GOTO 120
160 T$(C)=P$
170 GOSUB 300
180 GOSUB 500
190 IF V$<>"" THEN PRINT "Jogador ";V$;" venceu!":GOTO 400
200 FOR I=1 TO 9: IF T$(I)=" " THEN GOTO 230
210 NEXT I
220 PRINT "Empate!":GOTO 400
230 IF P$="X" THEN P$="O" ELSE P$="X"
240 GOTO 120
300 ' DESENHA TABULEIRO
310 CLS
320 PRINT " JOGO DA VELHA"
330 PRINT
340 PRINT " ";T$(1);" | ";T$(2);" | ";T$(3)
350 PRINT "---+---+---"
360 PRINT " ";T$(4);" | ";T$(5);" | ";T$(6)
370 PRINT "---+---+---"
380 PRINT " ";T$(7);" | ";T$(8);" | ";T$(9)
390 RETURN
400 ' FIM DE JOGO
410 PRINT:PRINT "Deseja jogar novamente (S/N)?";
420 INPUT R$
430 IF R$="S" OR R$="s" THEN GOTO 90
440 END
500 ' VERIFICA VITORIA
510 V$=""
520 FOR I=1 TO 3
530 IF T$(I)=T$(I+3) AND T$(I)=T$(I+6) AND T$(I)<>" " THEN V$=T$(I):RETURN
540 NEXT I
550 FOR I=1 TO 7 STEP 3
560 IF T$(I)=T$(I+1) AND T$(I)=T$(I+2) AND T$(I)<>" " THEN V$=T$(I):RETURN
570 NEXT I
580 IF T$(1)=T$(5) AND T$(1)=T$(9) AND T$(1)<>" " THEN V$=T$(1):RETURN
590 IF T$(3)=T$(5) AND T$(3)=T$(7) AND T$(3)<>" " THEN V$=T$(3):RETURN
600 RETURN
