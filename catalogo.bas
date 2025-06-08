10 DIM ID(100),T$(100),C$(100),Y(100)
20 LET N=0
100 CLS
110 PRINT "CATALOGADOR DE COLECAO"
120 PRINT "1 - Adicionar novo item"
130 PRINT "2 - Listar todos"
140 PRINT "3 - Buscar por titulo"
150 PRINT "4 - Salvar em disco"
160 PRINT "5 - Carregar do disco"
170 PRINT "6 - Sair"
180 INPUT "Opcao";OP
190 ON OP GOTO 300,400,500,600,700,800

300 CLS
310 N=N+1
320 ID(N)=N
330 INPUT "Titulo";T$(N)
340 INPUT "Categoria";C$(N)
350 INPUT "Ano";Y(N)
360 PRINT "Item adicionado!"
370 GOTO 100

400 CLS
410 IF N=0 THEN PRINT "Nenhum item.": GOTO 440
420 FOR I=1 TO N
430 PRINT ID(I); " - "; T$(I); " ("; C$(I); ", "; Y(I); ")"
435 NEXT I
440 INPUT "ENTER para voltar";A$
450 GOTO 100

500 CLS
510 IF N=0 THEN PRINT "Nenhum item.": GOTO 550
520 INPUT "Digite parte do titulo para buscar";B$
530 FOR I=1 TO N
540 IF INSTR(T$(I),B$)>0 THEN PRINT ID(I); " - "; T$(I); " ("; C$(I); ", "; Y(I); ")"
545 NEXT I
550 INPUT "ENTER para voltar";A$
560 GOTO 100

600 OPEN "A:CATALOGO.TXT" FOR OUTPUT AS #1
610 FOR I=1 TO N
620 PRINT #1,ID(I);",";T$(I);",";C$(I);",";Y(I)
630 NEXT I
640 CLOSE #1
650 PRINT "Salvo com sucesso!"
660 GOTO 100

700 OPEN "A:CATALOGO.TXT" FOR INPUT AS #1
710 N = 0
720 IF EOF(1) THEN 760
730 N = N + 1
740 INPUT #1,ID(N),T$(N),C$(N),Y(N)
750 GOTO 720
760 CLOSE #1
770 PRINT "Dados carregados!"
780 GOTO 100

800 END
