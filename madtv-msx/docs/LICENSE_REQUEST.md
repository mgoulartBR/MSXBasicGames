# Modelo de pedido de permissao a equipe do TVTower

(Rascunho para enviar por e-mail ou pelo forum/issue do projeto TVTower. Ajuste nome e contato.)

**Assunto:** Permission to use TVTower database content in a free MSX2 port of Mad TV

Hello TVTower team,

I am developing a non-commercial, open-source hobby port of the 1991 game *Mad TV* for the MSX2 computer (C + MSXgl). The game logic is my own; I derived the **content tables** (movie titles with year/genre/ratings/price modifiers, advertising contracts, news items, and — in the latest version — celebrity names and fame values used for movie casts) from your `database/Default/*.xml` files (version in my copy: 2024 database). The data is converted by a script (`tools/convert_db.py`) into compact tables, with titles/names simplified to ASCII and several fields rescaled, so the result is clearly an altered version.

I would like to ask for your permission to:
1. distribute the resulting ROM image and the generated data tables (source repository and binary), free of charge, with credit to the TVTower team and a note that the data was altered; and
2. keep the generated tables in the public repository.

I will credit TVTower (https://github.com/TVTower/TVTower) in the README, in the game's credits and in `LICENSES.md`, mark the data as modified, and take down or change anything you ask me to.

The repository does not contain any original Mad TV art or code.

Thank you very much for the project and for considering this.

Best regards,
<nome> — <contato>
