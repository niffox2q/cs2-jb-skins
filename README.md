### Модуль Skins для Jailbreak Core  
Добавляет систему случайных моделей (скинов) для игроков в зависимости от команды, а также отдельные модели для Начальника Тюрьмы.

При спавне игрока автоматически назначается случайная модель:
- CT — модели из `CTModels`
- T — модели из `TModels`
- Начальник Тюрьмы — модели из `WardenModels`

Модуль также:
- загружает модели из конфигурационного файла `addons/configs/Jailbreak/skins.ini`
- автоматически precache всех моделей
- применяет модель при смене Начальника Тюрьмы и при спавне игроков

### Установка:
Распаковать в `game/csgo/addons/`

### Требования:
- [Jailbreak Core](https://discord.gg/WkTwuKe8zy)
- [Utils](https://github.com/Pisex/cs2-menus)
- [ResourcePrecacher](https://github.com/Pisex/ResourcePrecacher/releases)
