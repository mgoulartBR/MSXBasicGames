# Mouse no WebMSX — v0.9

1. Abra https://webmsx.org/?MOUSE_MODE=1 para iniciar com mouse na porta 1.
2. Carregue `webmsx/KAW68PV [ASCII8].rom` em MSX2+ (V9958). O cartucho usa ASCII8 de 256 KiB.
3. Clique esquerdo seleciona e o direito dá uma ordem de movimento, coleta ou construção.

O mouse também pode ser ativado depois de o mapa carregar. No WebMSX, Alt+M alterna os modos do periférico. O jogo procura automaticamente as duas portas enquanto o mouse está ausente. Se trocar de porta, pressione M sem Alt para solicitar nova detecção.

Alt+Caps Lock prende/libera o ponteiro no WebMSX. Como o MSX usa movimento relativo, a seta do sistema pode não coincidir com a seta interna quando o ponteiro estiver livre. O modo preso evita essa diferença.

Para V9990, use `KAW9990 [ASCII8].rom` com a extensão V9990 habilitada e a saída externa selecionada. A ROM KAW9968 exige o V9968 real; KAW68PV é a prévia compatível com V9958.

O teste interativo no navegador foi realizado na v0.8. A v0.9 foi validada no openMSX, incluindo comandos de combate com eventos de mouse do sistema hospedeiro. A versão atual ainda deve ser conferida no WebMSX pelo usuário. Os nomes com [ASCII8] selecionam o mapper correto.
