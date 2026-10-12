# captura em tempo real em t=10 s; nome via variável de ambiente SHOTNAME
set throttle on
set frameskip 0
after time 10 { screenshot $::env(SHOTNAME).png; exit }
