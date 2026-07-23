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

DELETE FROM `module_string` WHERE `id` IN (3, 4, 5, 6, 7, 8) AND `module` = "mod-weekend-xp";
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
("mod-weekend-xp", 3, 'No Joyous Journeys schedule is set. The event follows the XPWeekend.IsJoyousJourneysActive config (currently: {}).'),
("mod-weekend-xp", 4, 'Joyous Journeys is scheduled from {} to {} (inclusive). Currently active: {}.'),
("mod-weekend-xp", 5, 'Joyous Journeys schedule cleared. The event now follows the XPWeekend.IsJoyousJourneysActive config.'),
("mod-weekend-xp", 6, 'Invalid date ''{}''. Use the YYYY-MM-DD format.'),
("mod-weekend-xp", 7, 'The end date must not be before the start date.'),
("mod-weekend-xp", 8, '|cff00ccffThe Joyous Journeys event is active until {}! Experience gains have been increased. Type .weekendxp j off to disable it.|r');

DELETE FROM `module_string_locale` WHERE `module` = 'mod-weekend-xp' AND `id` IN (3, 4, 5, 6, 7, 8);
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
('mod-weekend-xp', 3, 'koKR', '설정된 기쁨의 여정 일정이 없습니다. 이벤트는 XPWeekend.IsJoyousJourneysActive 설정을 따릅니다 (현재: {}).'),
('mod-weekend-xp', 4, 'koKR', '기쁨의 여정이 {}부터 {}까지 (양일 포함) 예정되어 있습니다. 현재 활성화 여부: {}.'),
('mod-weekend-xp', 5, 'koKR', '기쁨의 여정 일정이 삭제되었습니다. 이벤트는 이제 XPWeekend.IsJoyousJourneysActive 설정을 따릅니다.'),
('mod-weekend-xp', 6, 'koKR', '잘못된 날짜 ''{}''입니다. YYYY-MM-DD 형식을 사용하세요.'),
('mod-weekend-xp', 7, 'koKR', '종료 날짜는 시작 날짜보다 빠를 수 없습니다.'),
('mod-weekend-xp', 8, 'koKR', '|cff00ccff기쁨의 여정 이벤트가 {}까지 진행됩니다! 경험치 획득량이 증가했습니다. 비활성화하려면 .weekendxp j off 를 입력하세요.|r'),
('mod-weekend-xp', 3, 'frFR', 'Aucune période n''est définie pour Joyeux Voyages. L''événement suit la configuration XPWeekend.IsJoyousJourneysActive (actuellement : {}).'),
('mod-weekend-xp', 4, 'frFR', 'Joyeux Voyages est programmé du {} au {} (inclus). Actuellement actif : {}.'),
('mod-weekend-xp', 5, 'frFR', 'La période de Joyeux Voyages a été supprimée. L''événement suit désormais la configuration XPWeekend.IsJoyousJourneysActive.'),
('mod-weekend-xp', 6, 'frFR', 'Date invalide ''{}''. Utilisez le format YYYY-MM-DD.'),
('mod-weekend-xp', 7, 'frFR', 'La date de fin ne doit pas être antérieure à la date de début.'),
('mod-weekend-xp', 8, 'frFR', '|cff00ccffL''événement Joyeux Voyages est actif jusqu''au {} ! Les gains d''expérience ont été augmentés. Tapez .weekendxp j off pour le désactiver.|r'),
('mod-weekend-xp', 3, 'deDE', 'Es ist kein Zeitraum für Freudige Reisen festgelegt. Das Event folgt der Einstellung XPWeekend.IsJoyousJourneysActive (aktuell: {}).'),
('mod-weekend-xp', 4, 'deDE', 'Freudige Reisen ist vom {} bis zum {} (einschließlich) geplant. Aktuell aktiv: {}.'),
('mod-weekend-xp', 5, 'deDE', 'Der Zeitraum für Freudige Reisen wurde entfernt. Das Event folgt nun wieder der Einstellung XPWeekend.IsJoyousJourneysActive.'),
('mod-weekend-xp', 6, 'deDE', 'Ungültiges Datum ''{}''. Bitte das Format YYYY-MM-DD verwenden.'),
('mod-weekend-xp', 7, 'deDE', 'Das Enddatum darf nicht vor dem Startdatum liegen.'),
('mod-weekend-xp', 8, 'deDE', '|cff00ccffDas Event Freudige Reisen ist bis zum {} aktiv! Die Erfahrungsgewinne wurden erhöht. Gib .weekendxp j off ein, um es zu deaktivieren.|r'),
('mod-weekend-xp', 3, 'zhCN', '未设置欢乐之旅日程。活动遵循 XPWeekend.IsJoyousJourneysActive 配置（当前：{}）。'),
('mod-weekend-xp', 4, 'zhCN', '欢乐之旅已安排在 {} 至 {}（含）期间。当前是否激活：{}。'),
('mod-weekend-xp', 5, 'zhCN', '欢乐之旅日程已清除。活动现在遵循 XPWeekend.IsJoyousJourneysActive 配置。'),
('mod-weekend-xp', 6, 'zhCN', '无效日期 ''{}''。请使用 YYYY-MM-DD 格式。'),
('mod-weekend-xp', 7, 'zhCN', '结束日期不能早于开始日期。'),
('mod-weekend-xp', 8, 'zhCN', '|cff00ccff欢乐之旅活动将持续到 {}！经验值获取已提升。输入 .weekendxp j off 可将其关闭。|r'),
('mod-weekend-xp', 3, 'zhTW', '未設定歡樂之旅日程。活動遵循 XPWeekend.IsJoyousJourneysActive 設定（目前：{}）。'),
('mod-weekend-xp', 4, 'zhTW', '歡樂之旅已安排在 {} 至 {}（含）期間。目前是否啟用：{}。'),
('mod-weekend-xp', 5, 'zhTW', '歡樂之旅日程已清除。活動現在遵循 XPWeekend.IsJoyousJourneysActive 設定。'),
('mod-weekend-xp', 6, 'zhTW', '無效日期 ''{}''。請使用 YYYY-MM-DD 格式。'),
('mod-weekend-xp', 7, 'zhTW', '結束日期不能早於開始日期。'),
('mod-weekend-xp', 8, 'zhTW', '|cff00ccff歡樂之旅活動將持續到 {}！經驗值獲得已提升。輸入 .weekendxp j off 可將其停用。|r'),
('mod-weekend-xp', 3, 'esES', 'No hay un periodo establecido para Viajes Alegres. El evento sigue la configuración XPWeekend.IsJoyousJourneysActive (actualmente: {}).'),
('mod-weekend-xp', 4, 'esES', 'Viajes Alegres está programado del {} al {} (inclusive). Actualmente activo: {}.'),
('mod-weekend-xp', 5, 'esES', 'Se ha eliminado el periodo de Viajes Alegres. El evento vuelve a seguir la configuración XPWeekend.IsJoyousJourneysActive.'),
('mod-weekend-xp', 6, 'esES', 'Fecha no válida ''{}''. Usa el formato YYYY-MM-DD.'),
('mod-weekend-xp', 7, 'esES', 'La fecha de fin no debe ser anterior a la fecha de inicio.'),
('mod-weekend-xp', 8, 'esES', '|cff00ccff¡El evento Viajes Alegres está activo hasta el {}! La experiencia obtenida ha aumentado. Escribe .weekendxp j off para desactivarlo.|r'),
('mod-weekend-xp', 3, 'esMX', 'No hay un periodo establecido para Viajes Alegres. El evento sigue la configuración XPWeekend.IsJoyousJourneysActive (actualmente: {}).'),
('mod-weekend-xp', 4, 'esMX', 'Viajes Alegres está programado del {} al {} (inclusive). Actualmente activo: {}.'),
('mod-weekend-xp', 5, 'esMX', 'Se eliminó el periodo de Viajes Alegres. El evento vuelve a seguir la configuración XPWeekend.IsJoyousJourneysActive.'),
('mod-weekend-xp', 6, 'esMX', 'Fecha no válida ''{}''. Usa el formato YYYY-MM-DD.'),
('mod-weekend-xp', 7, 'esMX', 'La fecha de fin no debe ser anterior a la fecha de inicio.'),
('mod-weekend-xp', 8, 'esMX', '|cff00ccff¡El evento Viajes Alegres está activo hasta el {}! La experiencia obtenida aumentó. Escribe .weekendxp j off para desactivarlo.|r'),
('mod-weekend-xp', 3, 'ruRU', 'Расписание события «Радостные путешествия» не задано. Событие следует настройке XPWeekend.IsJoyousJourneysActive (сейчас: {}).'),
('mod-weekend-xp', 4, 'ruRU', 'Событие «Радостные путешествия» запланировано с {} по {} (включительно). Сейчас активно: {}.'),
('mod-weekend-xp', 5, 'ruRU', 'Расписание события «Радостные путешествия» удалено. Событие снова следует настройке XPWeekend.IsJoyousJourneysActive.'),
('mod-weekend-xp', 6, 'ruRU', 'Неверная дата ''{}''. Используйте формат YYYY-MM-DD.'),
('mod-weekend-xp', 7, 'ruRU', 'Дата окончания не должна быть раньше даты начала.'),
('mod-weekend-xp', 8, 'ruRU', '|cff00ccffСобытие «Радостные путешествия» активно до {}! Получаемый опыт увеличен. Введите .weekendxp j off, чтобы отключить его.|r');

DELETE FROM `command` WHERE `name` IN ('weekendxp rate');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('weekendxp rate', 0, 'Syntax: weekendxp rate $value \nSet your experience rate up to the allowed value.');

DELETE FROM `command` WHERE `name` IN ('weekendxp config');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('weekendxp config', 0, 'Syntax: weekendxp config\nDisplays the current configuration for the weekendxp mod.');

DELETE FROM `command` WHERE `name` IN ('weekendxp joyousjourneys schedule');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('weekendxp joyousjourneys schedule', 3, 'Syntax: weekendxp joyousjourneys schedule [$start $end || clear]\nSchedules the Joyous Journeys event. Dates use the YYYY-MM-DD format and are inclusive; the scheduled window overrides the XPWeekend.IsJoyousJourneysActive config.\nWithout arguments the current schedule is shown, "clear" removes it.');
