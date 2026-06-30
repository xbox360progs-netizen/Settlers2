#include "stdafx.h"
#include "LocalizationService.h"
#include <string.h>
#include <stdio.h>

namespace UI {

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

    static const char* const g_messages[LANG_COUNT][MSG_COUNT] = {
        // ── English ────────────────────────────────────────────────────
        {
            /* MSG_NONE                */ "",
            /* MSG_GEOLOGIST_CONFIRM   */ "Geologist: A=yes  B=no",
            /* MSG_GEOLOGIST_WORKING   */ "Geologist working...",
            /* MSG_GEOLOGIST_WORKING_SEC */ "Geologist working... %d sec",
            /* MSG_GEOLOGIST_ALREADY   */ "Geologist already surveying",
            /* MSG_GEOLOGIST_CANCELLED */ "Survey cancelled",
            /* MSG_GEOLOGIST_REPORT    */ "GEOLOGIST REPORT",
            /* MSG_GEOLOGIST_COAL_FOUND */ "Coal vein found! Units: %d",
            /* MSG_GEOLOGIST_IRON_FOUND */ "Iron ore found! Units: %d",
            /* MSG_GEOLOGIST_GOLD_FOUND */ "Gold vein found! Units: %d",
            /* MSG_GEOLOGIST_STONE_FOUND */ "Stone deposit found! Units: %d",
            /* MSG_GEOLOGIST_MARBLE_FOUND */ "Marble deposit found! Units: %d",
            /* MSG_GEOLOGIST_GRANITE_FOUND */ "Granite deposit found! Units: %d",
            /* MSG_GEOLOGIST_BRONZE_FOUND */ "Bronze ore vein found! Units: %d",
            /* MSG_GEOLOGIST_UNKNOWN_FOUND */ "Minerals detected",
            /* MSG_GEOLOGIST_BARREN_FOUND */ "Barren rock - No minerals found",

            /* MSG_RESOURCE_WOOD       */ "Wood",
            /* MSG_RESOURCE_PLANKS     */ "Planks",
            /* MSG_RESOURCE_STONE      */ "Stone",
            /* MSG_RESOURCE_FISH       */ "Fish",
            /* MSG_RESOURCE_MEAT       */ "Meat",
            /* MSG_RESOURCE_BREAD      */ "Bread",
            /* MSG_RESOURCE_COAL       */ "Coal",
            /* MSG_RESOURCE_IRONORE    */ "Iron Ore",
            /* MSG_RESOURCE_GOLDORE    */ "Gold Ore",
            /* MSG_RESOURCE_IRONBAR    */ "Iron Bar",
            /* MSG_RESOURCE_GOLDBAR    */ "Gold Bar",
            /* MSG_RESOURCE_BRONZEORE  */ "Bronze Ore",
            /* MSG_RESOURCE_MARBLE     */ "Marble",
            /* MSG_RESOURCE_GRANITE    */ "Granite",
            /* MSG_RESOURCE_WHEAT      */ "Wheat",
            /* MSG_RESOURCE_FLOUR      */ "Flour",
            /* MSG_RESOURCE_WATER      */ "Water",
            /* MSG_RESOURCE_TOOLS      */ "Tools",
            /* MSG_RESOURCE_TRAP       */ "Trap",
            /* MSG_RESOURCE_FIELD      */ "Field",
            /* MSG_RESOURCE_REALWOOD   */ "Real Wood",
            /* MSG_RESOURCE_EXOTICWOOD */ "Exotic Wood",
            /* MSG_RESOURCE_TITANIUM   */ "Titanium",
            /* MSG_RESOURCE_SALPETER   */ "Salpeter",
            /* MSG_RESOURCE_BRONZEBAR  */ "Bronze Bar",

            /* MSG_CONFIRM_YES         */ "Yes",
            /* MSG_CONFIRM_NO          */ "No",
            /* MSG_BUILD_MENU          */ "Build",
            /* MSG_DELETE_FLAG         */ "Delete",

            /* MSG_BUILDING_AND_FLAG_DELETED */ "Building and flag deleted!",
            /* MSG_CANNOT_DELETE_TOWN_HALL */ "Cannot delete town hall flag!",
            /* MSG_DELETE_FLAG_PROMPT  */ "Delete building and flag? A=Yes B=No",
            /* MSG_FLAG_REMOVED        */ "Flag removed!",
            /* MSG_NO_FLAG_NEARBY      */ "No flag found nearby",
            /* MSG_BUILDING_STARTED    */ "Building construction started!",
            /* MSG_CANNOT_PLACE_FLAG_OBJECT */ "Cannot place flag on object",
            /* MSG_FLAG_WATER_ONLY     */ "Flag can only be placed on water (deep or shallow)",
            /* MSG_FLAG_PLACED         */ "Flag placed!",
            /* MSG_ROAD_BUILD_HELP     */ "ROAD: A=add tile  B=cancel",
            /* MSG_ROAD_AUTO_PATH      */ "ROAD: auto-path built!",
            /* MSG_CANNOT_BUILD_HERE   */ "Cannot build here!",
            /* MSG_CANNOT_BUILD_THROUGH_ROAD */ "Cannot build through existing road!",
            /* MSG_ROAD_BUILT          */ "Road built!",
            /* MSG_PLACEMENT_CANCELLED */ "Placement cancelled",
            /* MSG_ROAD_CANCELLED      */ "Road cancelled",
            /* MSG_CANCELLED           */ "Cancelled",
            /* MSG_LOGISTICS_DEBUG_ON  */ "LOGISTICS DEBUG ON",
            /* MSG_LOGISTICS_DEBUG_OFF */ "LOGISTICS DEBUG OFF",

            // ── Notification titles ─────────────────────────────────
            /* MSG_TITLE_CONSTRUCTION_COMPLETE */ "COMPLETE",
            /* MSG_TITLE_BUILDING              */ "BUILD",
            /* MSG_TITLE_FLAG                  */ "FLAG",
            /* MSG_TITLE_DELIVERY              */ "DELIVERY",
            /* MSG_TITLE_PRODUCTION            */ "PRODUCTION",

            // ── Notification description strings ────────────────────
            /* MSG_CONSTRUCTION_COMPLETED      */ "Construction finished!",
            /* MSG_BUILDING_PLACED_TEXT        */ "Building started",
            /* MSG_FLAG_PLACED_TEXT            */ "Flag placed",
            /* MSG_FLAG_REMOVED_TEXT           */ "Flag removed",

            // ── Notification formats (name + amount) ────────────────
            /* MSG_NAME_FORMAT                 */ "%s",
            /* MSG_DELIVERY_FORMAT             */ "%s x%d",
            /* MSG_PRODUCTION_FORMAT           */ "Produced %s x%d",

            // ── Count / dedup format ────────────────────────────────
            /* MSG_COUNT_X_FORMAT              */ "x%d",
            /* MSG_COUNT_X99_FORMAT            */ "x99+",

            // ── Building names ──────────────────────────────────────
            /* MSG_BUILDING_GENERIC            */ "Building",
            /* MSG_BUILDING_WOODCUTTER         */ "Woodcutter",
            /* MSG_BUILDING_FORESTER           */ "Forester",
            /* MSG_BUILDING_SAWMILL            */ "Sawmill",
            /* MSG_BUILDING_STONEMASON         */ "Stonemason",
            /* MSG_BUILDING_BRONZEMINE         */ "Bronze Mine",
            /* MSG_BUILDING_IRONMINE           */ "Iron Mine",
            /* MSG_BUILDING_GOLDMINE           */ "Gold Mine",
            /* MSG_BUILDING_COALMINE           */ "Coal Mine",
            /* MSG_BUILDING_IRONSMELTER        */ "Iron Smelter",
            /* MSG_BUILDING_GOLDSMELTER        */ "Gold Smelter",
            /* MSG_BUILDING_BRONZESMELTER      */ "Bronze Smelter",
            /* MSG_BUILDING_FARM               */ "Farm",
            /* MSG_BUILDING_MILL               */ "Mill",
            /* MSG_BUILDING_BAKERY             */ "Bakery",
            /* MSG_BUILDING_FISHER             */ "Fisher",
            /* MSG_BUILDING_HUNTER             */ "Hunter",
            /* MSG_BUILDING_TOOLWORKSHOP       */ "Tool Workshop",
            /* MSG_BUILDING_STOREHOUSE         */ "Storehouse",
            /* MSG_BUILDING_WELL               */ "Well",
            /* MSG_BUILDING_BARRACKS           */ "Barracks",
            /* MSG_BUILDING_HQ                 */ "Headquarters",

            // ── Menu labels ──────────────────────────────────────────
            /* MSG_MENU_NEW_GAME              */ "New Game",
            /* MSG_MENU_MAP_EDITOR            */ "Map Editor",
            /* MSG_MENU_SETTINGS              */ "Settings",
            /* MSG_MENU_EXIT                  */ "Exit",
            /* MSG_MENU_SIZE_SELECT_TITLE     */ "Select Map Size",
            /* MSG_MENU_HINT_BACK             */ "Back",
            /* MSG_MENU_HINT_SELECT           */ "Select",
            /* MSG_MENU_SET_FLAG              */ "Set Flag",
            /* MSG_MENU_DELETE_FLAG           */ "Delete Flag",
            /* MSG_MENU_BUILDINGS             */ "Buildings",
        },
        // ── Russian ────────────────────────────────────────────────────
        {
            /* MSG_NONE                */ "",
            /* MSG_GEOLOGIST_CONFIRM   */ "Геолог: A=да  B=нет",
            /* MSG_GEOLOGIST_WORKING   */ "Геолог работает...",
            /* MSG_GEOLOGIST_WORKING_SEC */ "Геолог работает... %d сек",
            /* MSG_GEOLOGIST_ALREADY   */ "Геолог уже обследует",
            /* MSG_GEOLOGIST_CANCELLED */ "Обследование отменено",
            /* MSG_GEOLOGIST_REPORT    */ "ОТЧЕТ ГЕОЛОГА",
            /* MSG_GEOLOGIST_COAL_FOUND */ "Найден уголь! Ед.: %d",
            /* MSG_GEOLOGIST_IRON_FOUND */ "Найдена железная руда! Ед.: %d",
            /* MSG_GEOLOGIST_GOLD_FOUND */ "Найдено золото! Ед.: %d",
            /* MSG_GEOLOGIST_STONE_FOUND */ "Найден камень! Ед.: %d",
            /* MSG_GEOLOGIST_MARBLE_FOUND */ "Найден мрамор! Ед.: %d",
            /* MSG_GEOLOGIST_GRANITE_FOUND */ "Найден гранит! Ед.: %d",
            /* MSG_GEOLOGIST_BRONZE_FOUND */ "Найдена бронзовая руда! Ед.: %d",
            /* MSG_GEOLOGIST_UNKNOWN_FOUND */ "Обнаружены минералы",
            /* MSG_GEOLOGIST_BARREN_FOUND */ "Пустая порода — ископаемых нет",

            /* MSG_RESOURCE_WOOD       */ "Древесина",
            /* MSG_RESOURCE_PLANKS     */ "Доски",
            /* MSG_RESOURCE_STONE      */ "Камень",
            /* MSG_RESOURCE_FISH       */ "Рыба",
            /* MSG_RESOURCE_MEAT       */ "Мясо",
            /* MSG_RESOURCE_BREAD      */ "Хлеб",
            /* MSG_RESOURCE_COAL       */ "Уголь",
            /* MSG_RESOURCE_IRONORE    */ "Железная руда",
            /* MSG_RESOURCE_GOLDORE    */ "Золотая руда",
            /* MSG_RESOURCE_IRONBAR    */ "Железные слитки",
            /* MSG_RESOURCE_GOLDBAR    */ "Золотые слитки",
            /* MSG_RESOURCE_BRONZEORE  */ "Бронзовая руда",
            /* MSG_RESOURCE_MARBLE     */ "Мрамор",
            /* MSG_RESOURCE_GRANITE    */ "Гранит",
            /* MSG_RESOURCE_WHEAT      */ "Пшеница",
            /* MSG_RESOURCE_FLOUR      */ "Мука",
            /* MSG_RESOURCE_WATER      */ "Вода",
            /* MSG_RESOURCE_TOOLS      */ "Инструменты",
            /* MSG_RESOURCE_TRAP       */ "Ловушка",
            /* MSG_RESOURCE_FIELD      */ "Поле",
            /* MSG_RESOURCE_REALWOOD   */ "Настоящая древесина",
            /* MSG_RESOURCE_EXOTICWOOD */ "Экзотическая древесина",
            /* MSG_RESOURCE_TITANIUM   */ "Титан",
            /* MSG_RESOURCE_SALPETER   */ "Селитра",
            /* MSG_RESOURCE_BRONZEBAR  */ "Бронзовые слитки",

            /* MSG_CONFIRM_YES         */ "Да",
            /* MSG_CONFIRM_NO          */ "Нет",
            /* MSG_BUILD_MENU          */ "Построить",
            /* MSG_DELETE_FLAG         */ "Удалить",

            /* MSG_BUILDING_AND_FLAG_DELETED */ "Здание и флаг удалены!",
            /* MSG_CANNOT_DELETE_TOWN_HALL */ "Нельзя удалить флаг ратуши!",
            /* MSG_DELETE_FLAG_PROMPT  */ "Удалить здание и флаг? A=Да B=Нет",
            /* MSG_FLAG_REMOVED        */ "Флаг удален!",
            /* MSG_NO_FLAG_NEARBY      */ "Флаг не найден поблизости",
            /* MSG_BUILDING_STARTED    */ "Строительство начато!",
            /* MSG_CANNOT_PLACE_FLAG_OBJECT */ "Нельзя поставить флаг на объект",
            /* MSG_FLAG_WATER_ONLY     */ "Флаг можно ставить только на глубокую или мелкую воду",
            /* MSG_FLAG_PLACED         */ "Флаг установлен!",
            /* MSG_ROAD_BUILD_HELP     */ "ДОРОГА: A=добавить B=отмена",
            /* MSG_ROAD_AUTO_PATH      */ "ДОРОГА: автопоиск проложен!",
            /* MSG_CANNOT_BUILD_HERE   */ "Здесь нельзя построить!",
            /* MSG_CANNOT_BUILD_THROUGH_ROAD */ "Нельзя проложить через существующую дорогу!",
            /* MSG_ROAD_BUILT          */ "Дорога построена!",
            /* MSG_PLACEMENT_CANCELLED */ "Размещение отменено",
            /* MSG_ROAD_CANCELLED      */ "Дорога отменена",
            /* MSG_CANCELLED           */ "Отменено",
            /* MSG_LOGISTICS_DEBUG_ON  */ "ОТЛАДКА ЛОГИСТИКИ ВКЛ",
            /* MSG_LOGISTICS_DEBUG_OFF */ "ОТЛАДКА ЛОГИСТИКИ ВЫКЛ",

            // ── Notification titles ─────────────────────────────────
            /* MSG_TITLE_CONSTRUCTION_COMPLETE */ "ЗАВЕРШЕНО",
            /* MSG_TITLE_BUILDING              */ "СТРОЙКА",
            /* MSG_TITLE_FLAG                  */ "ФЛАГ",
            /* MSG_TITLE_DELIVERY              */ "ДОСТАВКА",
            /* MSG_TITLE_PRODUCTION            */ "ПРОИЗВОДСТВО",

            // ── Notification description strings ────────────────────
            /* MSG_CONSTRUCTION_COMPLETED      */ "Строительство завершено!",
            /* MSG_BUILDING_PLACED_TEXT        */ "Строительство начато",
            /* MSG_FLAG_PLACED_TEXT            */ "Флаг установлен",
            /* MSG_FLAG_REMOVED_TEXT           */ "Флаг удален",

            // ── Notification formats (name + amount) ────────────────
            /* MSG_NAME_FORMAT                 */ "%s",
            /* MSG_DELIVERY_FORMAT             */ "%s x%d",
            /* MSG_PRODUCTION_FORMAT           */ "Произведено %s x%d",

            // ── Count / dedup format ────────────────────────────────
            /* MSG_COUNT_X_FORMAT              */ "x%d",
            /* MSG_COUNT_X99_FORMAT            */ "x99+",

            // ── Building names ──────────────────────────────────────
            /* MSG_BUILDING_GENERIC            */ "Здание",
            /* MSG_BUILDING_WOODCUTTER         */ "Дровосек",
            /* MSG_BUILDING_FORESTER           */ "Лесник",
            /* MSG_BUILDING_SAWMILL            */ "Лесопилка",
            /* MSG_BUILDING_STONEMASON         */ "Каменщик",
            /* MSG_BUILDING_BRONZEMINE         */ "Бронзовая шахта",
            /* MSG_BUILDING_IRONMINE           */ "Железная шахта",
            /* MSG_BUILDING_GOLDMINE           */ "Золотая шахта",
            /* MSG_BUILDING_COALMINE           */ "Угольная шахта",
            /* MSG_BUILDING_IRONSMELTER        */ "Плавильня железа",
            /* MSG_BUILDING_GOLDSMELTER        */ "Плавильня золота",
            /* MSG_BUILDING_BRONZESMELTER      */ "Плавильня бронзы",
            /* MSG_BUILDING_FARM               */ "Ферма",
            /* MSG_BUILDING_MILL               */ "Мельница",
            /* MSG_BUILDING_BAKERY             */ "Пекарня",
            /* MSG_BUILDING_FISHER             */ "Рыбак",
            /* MSG_BUILDING_HUNTER             */ "Охотник",
            /* MSG_BUILDING_TOOLWORKSHOP       */ "Мастерская",
            /* MSG_BUILDING_STOREHOUSE         */ "Склад",
            /* MSG_BUILDING_WELL               */ "Колодец",
            /* MSG_BUILDING_BARRACKS           */ "Казармы",
            /* MSG_BUILDING_HQ                 */ "Штаб",

            // ── Menu labels ──────────────────────────────────────────
            /* MSG_MENU_NEW_GAME              */ "Новая игра",
            /* MSG_MENU_MAP_EDITOR            */ "Редактор карт",
            /* MSG_MENU_SETTINGS              */ "Настройки",
            /* MSG_MENU_EXIT                  */ "Выход",
            /* MSG_MENU_SIZE_SELECT_TITLE     */ "Выберите размер карты",
            /* MSG_MENU_HINT_BACK             */ "Назад",
            /* MSG_MENU_HINT_SELECT           */ "Выбрать",
            /* MSG_MENU_SET_FLAG              */ "Поставить флаг",
            /* MSG_MENU_DELETE_FLAG           */ "Удалить флаг",
            /* MSG_MENU_BUILDINGS             */ "Здания",
        },
    };

    // Compile-time checks: table dimensions must match enum sizes.
    typedef char G_MESSAGES_MUST_MATCH_MSG_COUNT
        [(ARRAY_COUNT(g_messages[LANG_EN]) == MSG_COUNT) ? 1 : -1];
    typedef char G_MESSAGES_MUST_MATCH_LANG_COUNT
        [(ARRAY_COUNT(g_messages) == LANG_COUNT) ? 1 : -1];

    // ─── LocalizationService ────────────────────────────────────────────

    LocalizationService::LocalizationService()
        : m_lang(LANG_EN)
    {
    }

    LocalizationService::~LocalizationService()
    {
    }

    void LocalizationService::SetLanguage(Language lang)
    {
        if (lang >= LANG_EN && lang < LANG_COUNT)
            m_lang = lang;
    }

    const char* LocalizationService::Get(UiMessageId id) const
    {
        if ((unsigned)id >= (unsigned)MSG_COUNT)
            return "";
        return g_messages[m_lang][id];
    }

    int LocalizationService::Format(UiMessageId id, const UiFormatArgs& args, char* out, int capacity) const
    {
        if (!out || capacity <= 0) return 0;
        const char* fmt = Get(id);
        if (!fmt || fmt[0] == '\0') {
            out[0] = '\0';
            return 0;
        }
        return _snprintf(out, capacity, fmt,
            args.values[0], args.values[1], args.values[2], args.values[3]);
    }

} // namespace UI
