SET @STRING_ENTRY := 11120;

DELETE FROM `acore_string` WHERE
    (`entry` = @STRING_ENTRY+0 AND `content_default` = 'Your experience rates were set to {}.')
OR
    (`entry` = @STRING_ENTRY+1 AND `content_default` = 'Wrong value specified. Please specify a value between 0 and {}')
OR
    (`entry` = @STRING_ENTRY+2 AND `content_default` = 'The rate being applied to you is {}.\nThe current weekendxp configuration is:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}');

DELETE FROM `module_string` WHERE `id` IN  (0, 1, 2) AND `module` = "mod-weekend-xp";
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
("mod-weekend-xp", 0, 'Your experience rates were set to {}.'),
("mod-weekend-xp", 1, 'Wrong value specified. Please specify a value between 0 and {}'),
("mod-weekend-xp", 2, 'The rate being applied to you is {}.\nThe current weekendxp configuration is:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}');

-- Used Copilot to do the Translations.
DELETE FROM `module_string_locale` WHERE `module` = 'mod-weekend-xp' AND `id` IN (0, 1, 2);
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
('mod-weekend-xp', 0, 'koKR', '경험치 배율이 {}로 설정되었습니다.'),
('mod-weekend-xp', 1, 'koKR', '잘못된 값입니다. 0과 {} 사이의 값을 입력하세요.'),
('mod-weekend-xp', 2, 'koKR', '현재 적용 중인 배율은 {}입니다.\n현재 weekendxp 설정:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'frFR', 'Vos taux d''expérience ont été définis sur {}.'),
('mod-weekend-xp', 1, 'frFR', 'Valeur incorrecte. Veuillez spécifier une valeur entre 0 et {}'),
('mod-weekend-xp', 2, 'frFR', 'Le taux appliqué est {}.\nLa configuration weekendxp actuelle est :\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'deDE', 'Deine Erfahrungsraten wurden auf {} gesetzt.'),
('mod-weekend-xp', 1, 'deDE', 'Falscher Wert angegeben. Bitte einen Wert zwischen 0 und {} angeben.'),
('mod-weekend-xp', 2, 'deDE', 'Die auf dich angewendete Rate ist {}.\nDie aktuelle weekendxp-Konfiguration ist:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'zhCN', '你的经验倍率已设置为 {}。'),
('mod-weekend-xp', 1, 'zhCN', '指定的值无效。请指定 0 到 {} 之间的值。'),
('mod-weekend-xp', 2, 'zhCN', '当前应用的倍率为 {}。\n当前 weekendxp 配置：\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'zhTW', '你的經驗倍率已設定為 {}。'),
('mod-weekend-xp', 1, 'zhTW', '指定的值無效。請指定 0 到 {} 之間的值。'),
('mod-weekend-xp', 2, 'zhTW', '目前套用的倍率為 {}。\n目前 weekendxp 設定：\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'esES', 'Tus tasas de experiencia se han establecido en {}.'),
('mod-weekend-xp', 1, 'esES', 'Valor incorrecto. Por favor, especifica un valor entre 0 y {}'),
('mod-weekend-xp', 2, 'esES', 'La tasa aplicada es {}.\nLa configuración actual de weekendxp es:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'esMX', 'Tus tasas de experiencia se establecieron en {}.'),
('mod-weekend-xp', 1, 'esMX', 'Valor incorrecto. Por favor, especifica un valor entre 0 y {}'),
('mod-weekend-xp', 2, 'esMX', 'La tasa que se te aplica es {}.\nLa configuración actual de weekendxp es:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}'),
('mod-weekend-xp', 0, 'ruRU', 'Ваши темпы опыта установлены на {}.'),
('mod-weekend-xp', 1, 'ruRU', 'Указано неверное значение. Пожалуйста, укажите значение от 0 до {}'),
('mod-weekend-xp', 2, 'ruRU', 'Применяемый к вам множитель: {}.\nТекущая конфигурация weekendxp:\nAnnounce {}\nAlwaysEnabled {}\nQuestOnly {}\nMaxLevel {}\nxpAmount {}\nIndividualXPEnabled {}\nEnabled {}\nMaxAllowedRate {}');

DELETE FROM `command` WHERE `name` IN ('weekendxp rate');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('weekendxp rate', 0, 'Syntax: weekendxp rate $value \nSet your experience rate up to the allowed value.');

DELETE FROM `command` WHERE `name` IN ('weekendxp config');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('weekendxp config', 0, 'Syntax: weekendxp config\nDisplays the current configuration for the weekendxp mod.');
