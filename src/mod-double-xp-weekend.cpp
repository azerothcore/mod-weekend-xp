#include "Configuration/Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "DataMap.h"
#include "Log.h"
#include "LootMgr.h"
#include "QuestDef.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Timer.h"
#include "Tokenize.h"
#include "World.h"
#include "WorldState.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <limits>
#include <time.h>
#include <unordered_map>

using namespace Acore::ChatCommands;

enum WeekendXP
{
    SETTING_WEEKEND_XP_RATE = 0,
    SETTING_WEEKEND_XP_DISABLE = 1,
    SETTING_WEEKEND_XP_VERSION = 2,
    SETTING_WEEKEND_XP_JOYOUS_JOURNEYS = 3,
};

// Indices into the characters DB `worldstates` table. Must be unique across the
// core (see WorldStateDefines.h, custom entries start at 20001) and other modules.
enum WeekendXPWorldStates
{
    WS_JOYOUS_JOURNEYS_START = 97301, // unix timestamp, first second of the event
    WS_JOYOUS_JOURNEYS_END   = 97302, // unix timestamp, first second AFTER the event
};

// Reputation factions left out of the bonus by XPWeekend.ExcludeInsaneReps
enum WeekendXPInsaneFactions
{
    INSANE_FACTION_BOOTY_BAY      = 21,
    INSANE_FACTION_BLOODSAIL      = 87,
    INSANE_FACTION_STEAMWHEEDLE   = 169,
    INSANE_FACTION_RAVENHOLDT     = 349,
    INSANE_FACTION_GADGETZAN      = 369,
    INSANE_FACTION_RATCHET        = 470,
    INSANE_FACTION_EVERLOOK       = 577,
    INSANE_FACTION_SHENDRALAR     = 809,
    INSANE_FACTION_DARKMOON_FAIRE = 909,
};

// Max-level quest money noted in OnPlayerQuestComputeXP and read back in OnPlayerCompleteQuest.
// Keyed by quest id, so a quest rewarded from inside another quest's reward keeps both entries.
struct WeekendXpQuestMoney : public DataMap::Base
{
    std::unordered_map<uint32 /*questId*/, uint32 /*maxLevelMoney*/> Pending;
};

static constexpr char const* WEEKEND_XP_QUEST_MONEY_KEY = "mod-weekend-xp.QuestMoney";

class DoubleXpWeekend
{

public:
    
    DoubleXpWeekend() { }
    
    // NOTE We need to access the DoubleXpWeekend logic from other
    // places, so keep all the logic accessible via a singleton here,
    // and have the `CommandScript` and the `PlayeScript` access this
    // for the functionality they need.
    static DoubleXpWeekend* instance()
    {
        static DoubleXpWeekend instance;
        return &instance;
    }

    uint32 OnPlayerGiveXP(Player* player, uint32 originalAmount, uint8 xpSource) const
    {
        if (!_xpEnabled || !IsEventActive())
        {
            return originalAmount;
        }

        if (ConfigIsDKStartZoneRequired() && player->getClass() == CLASS_DEATH_KNIGHT && !IsDKStartZoneComplete(player))
        {
            return originalAmount;
        }

        if (ConfigQuestOnly() && xpSource != PlayerXPSource::XPSOURCE_QUEST && xpSource != PlayerXPSource::XPSOURCE_QUEST_DF)
        {
            return originalAmount;
        }

        if (player->GetLevel() >= ConfigMaxLevel())
        {
            return originalAmount;
        }

        float newAmount = (float)originalAmount * GetExperienceRate(player);
        return (uint32) newAmount;
    }

    void OnPlayerUpdateGatheringSkill(uint32 skillId, uint32& gain) const
    {
        if (!IsProfessionSkill(skillId) && skillId != SKILL_LOCKPICKING)
            return;

        if (!_professionEnabled || !IsSkillBonusActive())
            return;

        gain = std::max(gain, BoostedSkillStep(gain));
    }

    void OnPlayerUpdateCraftingSkill(SkillLineAbilityEntry const* skill, uint32& gain) const
    {
        // Item-creating class spells also raise crafting skills, so only boost professions
        if (!IsProfessionSkill(skill->SkillLine))
            return;

        if (!_professionEnabled || !IsSkillBonusActive())
            return;

        gain = std::max(gain, BoostedSkillStep(gain));
    }

    // Fishing, weapon skills and Defense have no gain hook, so the extra points are added after a
    // successful skill-up
    void OnPlayerUpdateSkill(Player* player, uint32 skillId, uint32 max, uint32 step, uint32 newValue) const
    {
        bool enabled;
        if (skillId == SKILL_FISHING)
            enabled = _professionEnabled;
        else if (IsWeaponOrDefenseSkill(skillId))
            enabled = _weaponSkillEnabled;
        else
            return;

        if (!enabled || !IsSkillBonusActive())
            return;

        uint32 const boostedStep = BoostedSkillStep(step);
        if (boostedStep <= step)
            return;

        uint32 const target = std::min<uint32>(newValue + (boostedStep - step), max);
        if (target > newValue)
            player->SetSkill(skillId, player->GetSkillStep(skillId), target, max);
    }

    void OnPlayerGiveReputation(int32 factionID, float& amount) const
    {
        // Only gains get the bonus, reputation losses stay as they are
        if (amount <= 0.0f)
            return;

        float rate = 1.0f;

        if (_reputationEnabled && IsEventActive())
            rate *= _bonusRate;

        if (IsJoyousJourneysActive() && ConfigJoyousJourneysRepRate())
            rate *= ConfigJoyousJourneysRepRate();

        if (rate == 1.0f)
            return;

        if (ExcludeInsaneReps())
        {
            switch (factionID)
            {
                case INSANE_FACTION_BOOTY_BAY:
                case INSANE_FACTION_BLOODSAIL:
                case INSANE_FACTION_STEAMWHEEDLE:
                case INSANE_FACTION_RAVENHOLDT:
                case INSANE_FACTION_GADGETZAN:
                case INSANE_FACTION_RATCHET:
                case INSANE_FACTION_EVERLOOK:
                case INSANE_FACTION_SHENDRALAR:
                case INSANE_FACTION_DARKMOON_FAIRE:
                    return;
            }
        }

        amount *= rate;
    }

    void OnPlayerBeforeLootMoney(Loot* loot) const
    {
        if (!loot->gold || !IsMoneyBonusActive())
            return;

        loot->gold = uint32(std::min<double>(double(loot->gold) * _bonusRate, std::numeric_limits<uint32>::max()));
    }

    // Player::RewardQuest calls this right before it pays the max-level money; the quest windows
    // call it too, possibly without a player. Notes that money for OnPlayerCompleteQuest.
    void OnPlayerQuestComputeXP(Player* player, Quest const* quest) const
    {
        if (!player || !quest)
            return;

        if (!IsMoneyBonusActive())
        {
            if (WeekendXpQuestMoney* data = player->CustomData.Get<WeekendXpQuestMoney>(WEEKEND_XP_QUEST_MONEY_KEY))
                data->Pending.erase(quest->GetQuestId());

            return;
        }

        // Same condition as Player::RewardQuest
        bool const moneyInsteadOfXp = player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) ||
            sScriptMgr->OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(player);

        WeekendXpQuestMoney* data = player->CustomData.GetDefault<WeekendXpQuestMoney>(WEEKEND_XP_QUEST_MONEY_KEY);
        data->Pending[quest->GetQuestId()] = moneyInsteadOfXp ? quest->GetRewMoneyMaxLevel() : 0;
    }

    // Last call of Player::RewardQuest: pays the quest money bonus as a separate payment
    void OnPlayerCompleteQuest(Player* player, Quest const* quest) const
    {
        WeekendXpQuestMoney* data = player->CustomData.Get<WeekendXpQuestMoney>(WEEKEND_XP_QUEST_MONEY_KEY);
        if (!data)
            return;

        auto const itr = data->Pending.find(quest->GetQuestId());
        if (itr == data->Pending.end())
            return;

        uint32 const maxLevelMoney = itr->second;
        data->Pending.erase(itr);

        if (!IsMoneyBonusActive())
            return;

        // Quests that take money, or pay nothing, are never touched
        int64 money = int64(maxLevelMoney) + quest->GetRewOrReqMoney(player->GetLevel());
        if (money <= 0)
            return;

        // Same play time limits as Player::RewardQuest
        if (player->HasPlayerFlag(PLAYER_FLAGS_NO_PLAY_TIME))
            return;

        if (player->HasPlayerFlag(PLAYER_FLAGS_PARTIAL_PLAY_TIME))
            money /= 2;

        if (!money)
            return;

        int64 bonus = int64(double(money) * _bonusRate) - money;
        if (bonus <= 0)
            return;

        bonus = std::min<int64>(bonus, std::numeric_limits<int32>::max());

        if (player->ModifyMoney(int32(bonus)))
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD, uint32(bonus));

        // Player::RewardQuest already saved the player before this hook
        player->AdditionalSavingAddMask(ADDITIONAL_SAVING_INVENTORY_AND_GOLD);
    }

    void OnPlayerLogin(Player* player, ChatHandler* handler) const
    {
        // TODO I am assuming that this is always called when a character logs in...
        // if that is not the case, thing migh get weird... Adding some asserts or warnings would be nice
        // but I'm not sure how to handle "This shouldn't be happening but it is" kind of scenarios in acore
        MigratePlayerSettings(player, handler);

        // XPWeekend.AnnounceStartEnd replaces the XPWeekend.Announce login text
        if (!_announceStartEnd && ConfigAnnounce())
        {
            if (!_xpEnabled)
            {
                handler->PSendSysMessage("This server is running the |cff4CFF00Double XP Weekend |rmodule.");
            }
            else if (IsEventActive() && !ConfigAlwaysEnabled())
            {
                float rate = GetExperienceRate(player);
                handler->PSendSysMessage("It's the weekend! Your XP rate has been set to: {}", rate);
            }
            else if (IsEventActive() && ConfigAlwaysEnabled())
            {
                float rate = GetExperienceRate(player);
                handler->PSendSysMessage("Your XP rate has been set to: {}", rate);
            }
            else
            {
                handler->PSendSysMessage("This server is running the |cff4CFF00Double XP Weekend |rmodule.");
            }
        }

        if (_announceStartEnd && !_startMessage.empty() && IsEventActive()
            && !ConfigAlwaysEnabled())
            handler->SendSysMessage(_startMessage);
    }

    // Broadcasts XPWeekend.StartMessage / XPWeekend.EndMessage to every online
    // player when the event turns on or off, if XPWeekend.AnnounceStartEnd
    // is on. The state is tracked even while it is off.
    void OnWorldUpdate(uint32 diff)
    {
        _stateCheckTimer += diff;
        if (_stateCheckTimer < STATE_CHECK_INTERVAL)
            return;

        _stateCheckTimer = 0;

        bool const active = IsEventActive();

        // First check after startup: only remember the state, do not announce.
        if (!_wasActive)
        {
            _wasActive = active;
            return;
        }

        if (active == *_wasActive)
            return;

        _wasActive = active;

        std::string const& message = active ? _startMessage : _endMessage;
        if (_announceStartEnd && !message.empty())
            ChatHandler(nullptr).SendWorldText("{}", message);
    }

    bool HandleSetXPBonusCommand(ChatHandler* handler, float rate) const
    {
        Player* player = handler->GetPlayer();

        float maxRate = ConfigMaxAllowedRate();

        if (rate <= 0.0f || rate > maxRate)
        {
            handler->PSendModuleSysMessage("mod-weekend-xp", 1, maxRate);
            handler->SetSentErrorMessage(true);
            return true;
        }

        PlayerSettingSetRate(player, rate);
        handler->PSendModuleSysMessage("mod-weekend-xp", 0, rate);

        // TODO if the `EnablePlayerSettings` is not set, the setting wont be remembered by the
        // server after the player logs out, meaning the player needs to do this again on next login

        return true;
    }

    bool HandleGetCurrentConfigCommand(ChatHandler* handler) const
    {
        Player* player = handler->GetPlayer();

        const float actualRate = _xpEnabled ? GetExperienceRate(player) : 1.0f;
        const bool isAnnounceEnabled = ConfigAnnounce();
        const bool isAlwaysEnabled = ConfigAlwaysEnabled();
        const bool isQuestOnly = ConfigQuestOnly();
        const uint32 maxLevel = ConfigMaxLevel();
        const float xpRate = ConfigxpAmount();
        const bool isIndividulaXpEnabled = ConfigIndividualXPEnabled();
        const bool isEnabled = ConfigEnabled();
        const float maxXpRate = ConfigMaxAllowedRate();

        handler->PSendModuleSysMessage("mod-weekend-xp", 2,
            actualRate,
            isAnnounceEnabled,
            isAlwaysEnabled,
            isQuestOnly,
            maxLevel,
            xpRate,
            isIndividulaXpEnabled,
            isEnabled,
            maxXpRate,
            _xpEnabled,
            _professionEnabled,
            _weaponSkillEnabled,
            _reputationEnabled,
            _moneyEnabled
        );

        return true;
    }

    // When a window was set via `.weekendxp joyousjourneys schedule`, the
    // scheduled window decides whether the event is active. Otherwise the
    // static config flag is used.
    bool IsJoyousJourneysActive() const
    {
        if (HasJoyousJourneysSchedule())
        {
            time_t now = time(nullptr);
            return now >= JoyousJourneysStart() && now < JoyousJourneysEnd();
        }

        return _joyousJourneysActiveConfig;
    }

    bool HasJoyousJourneysSchedule() const { return JoyousJourneysEnd() != 0; }

    std::string JoyousJourneysEndDateString() const
    {
        // The stored end is the first second after the event, so the last
        // (inclusive) event day is one second earlier.
        return FormatDate(JoyousJourneysEnd() - 1);
    }

    bool HandleJoyousJourneysScheduleCommand(ChatHandler* handler, Optional<std::string> startDate, Optional<std::string> endDate) const
    {
        // PSendModuleSysMessage drops the message when there is no session, and this
        // command is available from the console, so parse and send separately.
        if (!startDate)
        {
            if (!HasJoyousJourneysSchedule())
            {
                handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 3,
                    _joyousJourneysActiveConfig));
            }
            else
            {
                handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 4,
                    FormatDate(JoyousJourneysStart()), JoyousJourneysEndDateString(), IsJoyousJourneysActive()));
            }

            return true;
        }

        if (StringEqualI(*startDate, "clear"))
        {
            sWorldState->setWorldState(WS_JOYOUS_JOURNEYS_START, 0);
            sWorldState->setWorldState(WS_JOYOUS_JOURNEYS_END, 0);
            handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 5));
            return true;
        }

        if (!endDate)
        {
            return false; // show command usage
        }

        Optional<time_t> start = ParseDate(*startDate);
        Optional<time_t> endStart = ParseDate(*endDate);

        if (!start || !endStart)
        {
            handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 6, !start ? *startDate : *endDate));
            handler->SetSentErrorMessage(true);
            return true;
        }

        if (*endStart < *start)
        {
            handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 7));
            handler->SetSentErrorMessage(true);
            return true;
        }

        sWorldState->setWorldState(WS_JOYOUS_JOURNEYS_START, uint64(*start));
        sWorldState->setWorldState(WS_JOYOUS_JOURNEYS_END, uint64(*endStart + DAY));
        handler->SendSysMessage(handler->PGetParseModuleString("mod-weekend-xp", 4,
            FormatDate(JoyousJourneysStart()), JoyousJourneysEndDateString(), IsJoyousJourneysActive()));

        return true;
    }

    float ConfigJoyousJourneysXPRate() const { return sConfigMgr->GetOption<float>("XPWeekend.JoyousJourneysXPRate", 1.0f); }
    float ConfigJoyousJourneysRepRate() const { return _joyousJourneysRepRate; }
    bool ExcludeInsaneReps() const { return _excludeInsaneReps; }
    float ConfigMaxAllowedRate() const { return sConfigMgr->GetOption<float>("XPWeekend.MaxAllowedRate", 2.0f); }

    // NOTE options read here are cached until the next config load or reload
    void LoadConfig()
    {
        _enabled = sConfigMgr->GetOption<bool>("XPWeekend.Enabled", false);
        _alwaysEnabled = sConfigMgr->GetOption<bool>("XPWeekend.AlwaysEnabled", false);
        _joyousJourneysActiveConfig = sConfigMgr->GetOption<bool>("XPWeekend.IsJoyousJourneysActive", false);
        _joyousJourneysRepRate = sConfigMgr->GetOption<float>("XPWeekend.JoyousJourneysRepRate", 1.10f);
        _excludeInsaneReps = sConfigMgr->GetOption<bool>("XPWeekend.ExcludeInsaneReps", true);

        _xpEnabled = sConfigMgr->GetOption<bool>("XPWeekend.XPEnabled", true);
        _professionEnabled = sConfigMgr->GetOption<bool>("XPWeekend.ProfessionEnabled", false);
        _weaponSkillEnabled = sConfigMgr->GetOption<bool>("XPWeekend.WeaponSkillEnabled", false);
        _reputationEnabled = sConfigMgr->GetOption<bool>("XPWeekend.ReputationEnabled", false);
        _moneyEnabled = sConfigMgr->GetOption<bool>("XPWeekend.MoneyEnabled", false);

        // XPWeekend.xpAmount is the rate of every bonus, not only XP. Between 0 and 1 it only
        // lowers XP: the other bonuses never take anything away
        _bonusRate = std::max(ConfigxpAmount(), 1.0f);

        // Builds the bonus window from XPWeekend.Days, XPWeekend.StartHour
        // and XPWeekend.EndHour.
        std::string const days =
            sConfigMgr->GetOption<std::string>("XPWeekend.Days", "5,6,0");
        int32 const startHour = LoadHourOption("XPWeekend.StartHour");
        int32 const endHour = LoadHourOption("XPWeekend.EndHour");

        _announceStartEnd =
            sConfigMgr->GetOption<bool>("XPWeekend.AnnounceStartEnd", false);
        _startMessage = sConfigMgr->GetOption<std::string>(
            "XPWeekend.StartMessage", "The Double XP Event has started!");
        _endMessage = sConfigMgr->GetOption<std::string>(
            "XPWeekend.EndMessage", "The Double XP Event has ended!");

        // EndHour is exclusive; EndHour == StartHour means a 24-hour window.
        uint32 length = (endHour - startHour + 24) % 24;
        if (!length)
            length = 24;

        _activeHours.reset();

        for (std::string_view entry : Acore::Tokenize(days, ',', false))
        {
            std::string const trimmed =
                Acore::String::Trim(std::string(entry), std::locale());
            if (trimmed.empty())
                continue;

            Optional<uint32> const day = Acore::StringTo<uint32>(trimmed);
            if (!day || *day > 6)
            {
                LOG_ERROR("module", "XPWeekend.Days: invalid entry '{}', "
                    "expected 0-6 (0 = Sunday). Entry ignored.", trimmed);
                continue;
            }

            // A window that crosses midnight on Saturday wraps to Sunday.
            for (uint32 hour = 0; hour < length; ++hour)
                _activeHours.set((*day * 24 + startHour + hour)
                    % _activeHours.size());
        }
    }

    bool IsXPBonusEnabled() const { return _xpEnabled; }

private:

    // Bit index is `weekday * 24 + hour`, weekday in `tm_wday` numbering
    // (0 = Sunday). Filled by LoadConfig().
    std::bitset<7 * 24> _activeHours;

    // Filled by LoadConfig(). An empty message is not sent.
    bool _announceStartEnd = false;
    std::string _startMessage;
    std::string _endMessage;

    static constexpr uint32 STATE_CHECK_INTERVAL = 1000; // ms
    uint32 _stateCheckTimer = 0;
    // Event state at the last check; unset until the first check.
    Optional<bool> _wasActive;

    static int32 LoadHourOption(std::string const& name)
    {
        int32 const hour = sConfigMgr->GetOption<int32>(name, 0);
        if (hour < 0 || hour > 23)
        {
            LOG_ERROR("module", "{}: invalid value {}, expected 0-23. "
                "Using 0.", name, hour);
            return 0;
        }

        return hour;
    }

    bool _enabled = false;
    bool _alwaysEnabled = false;
    bool _joyousJourneysActiveConfig = false;
    float _joyousJourneysRepRate = 1.10f;
    bool _excludeInsaneReps = true;

    bool _xpEnabled = true;
    bool _professionEnabled = false;
    bool _weaponSkillEnabled = false;
    bool _reputationEnabled = false;
    bool _moneyEnabled = false;
    float _bonusRate = 2.0f;

    // Skill points are whole numbers, so the boosted step is rounded down; skill bonuses only ever add
    bool IsSkillBonusActive() const { return _bonusRate > 1.0f && IsEventActive(); }
    uint32 BoostedSkillStep(uint32 step) const { return uint32(double(step) * _bonusRate); }

    bool IsMoneyBonusActive() const { return _moneyEnabled && _bonusRate > 1.0f && IsEventActive(); }

    static bool IsWeaponOrDefenseSkill(uint32 skillId)
    {
        if (skillId == SKILL_DEFENSE)
            return true;

        SkillLineEntry const* entry = sSkillLineStore.LookupEntry(skillId);
        return entry && entry->categoryId == SKILL_CATEGORY_WEAPON;
    }

    // NOTE keep options together to prevent having more than 1 potential default value
    bool ConfigAlwaysEnabled() const { return _alwaysEnabled; }
    bool ConfigAnnounce() const { return sConfigMgr->GetOption<bool>("XPWeekend.Announce", false); }
    bool ConfigQuestOnly() const { return sConfigMgr->GetOption<bool>("XPWeekend.QuestOnly", false); }
    uint32 ConfigMaxLevel() const { return sConfigMgr->GetOption<uint32>("XPWeekend.MaxLevel", 80); }
    float ConfigxpAmount() const { return sConfigMgr->GetOption<float>("XPWeekend.xpAmount", 2.0f); }
    bool ConfigIndividualXPEnabled() const { return sConfigMgr->GetOption<bool>("XPWeekend.IndividualXPEnabled", false); }
    bool ConfigEnabled() const { return _enabled; }
    bool ConfigIsDKStartZoneRequired() const { return sConfigMgr->GetOption<bool>("XPWeekend.IsDKStartZoneRequired", false); }

    bool IsDKStartZoneComplete(Player* player) const
    {
        return player->IsQuestRewarded(13188) || player->IsQuestRewarded(13189);
    }

    time_t JoyousJourneysStart() const { return time_t(sWorldState->getWorldState(WS_JOYOUS_JOURNEYS_START)); }
    time_t JoyousJourneysEnd() const { return time_t(sWorldState->getWorldState(WS_JOYOUS_JOURNEYS_END)); }

    // Parses YYYY-MM-DD into the first second of that day in server local time.
    static Optional<time_t> ParseDate(std::string const& text)
    {
        unsigned int year, month, day;
        char extra;
        if (sscanf(text.c_str(), "%4u-%2u-%2u%c", &year, &month, &day, &extra) != 3)
        {
            return std::nullopt;
        }

        tm date{};
        date.tm_year = int(year) - 1900;
        date.tm_mon = int(month) - 1;
        date.tm_mday = int(day);
        date.tm_isdst = -1;

        time_t stamp = mktime(&date);

        // mktime normalizes out-of-range fields (e.g. 2026-02-30 becomes March 2nd),
        // so a changed day/month means the input was not a real calendar date.
        if (stamp == time_t(-1) || date.tm_mday != int(day) || date.tm_mon != int(month) - 1)
        {
            return std::nullopt;
        }

        return stamp;
    }

    static std::string FormatDate(time_t when)
    {
        return Acore::Time::TimeToTimestampStr(Seconds(when), "%Y-%m-%d");
    }

    void PlayerSettingSetRate(Player* player, float rate) const
    {
        // HACK PlayerSetting seems to store uint32 only, so save our `float` as if it was a `uint32`
        uint32 encodedRate;
        float* reinterpretingPointer = (float*)&encodedRate;
        *reinterpretingPointer = rate;
        player->UpdatePlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_RATE, encodedRate);
    }
    
    float PlayerSettingGetRate(Player* player) const
    {
        uint32 rateStored = player->GetPlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_RATE).value;
        // HACK PlayerSetting seems to store uint32 only, so save our `float` as if it was a `uint32`
        float rate = *(float*)&rateStored;
        return rate;
    }

    void MigratePlayerSettings(Player* player, ChatHandler* /*handler*/) const
    {
        static const uint32 VERSION = 1;

        uint32 playersCurrentVersion = player->GetPlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_VERSION).value;
        bool validMigration = playersCurrentVersion == 0 && VERSION == 1;
        if (!validMigration) 
        {
            // Currently there is only either version 0 (the default) or 1
            // This check should never fail unless a new version is introduced and the
            // migration here is not updated.
            return;
        }

        // On version 1 the only thing to migrate is the SETTING_WEEKEND_XP_RATE
        float newRate = ConfigxpAmount();
        uint32 originalRate = player->GetPlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_RATE).value;
        if (originalRate <= 0)
        {
            // player setting was never set before, just use default
        }
        else if ((float)originalRate > ConfigMaxAllowedRate())
        {
            // player setting was set but the value is not valid, just use default
        }                
        else
        {
            // player setting was set, use the same rate
            newRate = (float) originalRate;
        }

        // HACK PlayerSetting seems to store uint32 only, so save our `float` as if it was a `uint32`
        uint32 encodedRate;
        float* reinterpretingPointer = (float*)&encodedRate;
        *reinterpretingPointer = newRate;
        player->UpdatePlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_RATE, encodedRate);
        player->UpdatePlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_VERSION, VERSION);
    }
    
    // TODO why is there a `GetDisable` player setting but no way to actually modify it? Leaving as is for now...
    bool PlayerSettingGetDisable(Player* player) const
    {
        return player->GetPlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_DISABLE).value == (uint32)1;
    }

    float GetExperienceRate(Player* player) const
    {
        float rate = ConfigxpAmount();

        if (PlayerSettingGetDisable(player))
        {
            return 1.0f;
        }

        // If individualxp setting is enabled... and a rate was set, overwrite it.
        if (ConfigIndividualXPEnabled())
        {
            rate = PlayerSettingGetRate(player);
            bool storeRate = false;

            // Invalid stored rates (e.g. from the old cap) count as never set
            if (!(std::isnormal(rate) && rate > 0.0f))
            {
                rate = ConfigxpAmount();
                storeRate = true;
            }

            // If config changed, cap it to max allowed
            if (rate > ConfigMaxAllowedRate())
            {
                rate = ConfigMaxAllowedRate();
                storeRate = true;
            }

            if (storeRate)
                PlayerSettingSetRate(player, rate);
        }

        if (IsJoyousJourneysActive() && !player->GetPlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_JOYOUS_JOURNEYS).IsEnabled())
        {
            if (!player->GetMap()->IsBattlegroundOrArena())
                rate += ConfigJoyousJourneysXPRate();
        }

        // Prevent returning 0% rate.
        return rate > 0.0f ? rate : 1.0f;
    }

    bool IsEventActive() const
    {
        if (ConfigAlwaysEnabled())
        {
            return true;
        }

        if (!ConfigEnabled())
        {
            return false;
        }

        std::tm const now = Acore::Time::TimeBreakdown();
        return _activeHours[now.tm_wday * 24 + now.tm_hour];
    }
};

class weekendxp_commandscript : public CommandScript
{
public:
    weekendxp_commandscript() : CommandScript("weekendxp_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "weekendxp rate", HandleSetXPBonusCommand, SEC_PLAYER, Console::No },
            { "weekendxp config", HandleGetCurrentConfigCommand, SEC_PLAYER, Console::No },
            { "weekendxp joyousjourneys", HandleJoyousJourneysCommand, SEC_PLAYER, Console::No },
            { "weekendxp joyousjourneys schedule", HandleJoyousJourneysScheduleCommand, SEC_ADMINISTRATOR, Console::Yes }
        };

        return commandTable;
    }

    static bool HandleSetXPBonusCommand(ChatHandler* handler, float rate)
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        return mod->HandleSetXPBonusCommand(handler, rate);
    }

    static bool HandleGetCurrentConfigCommand(ChatHandler* handler)
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        return mod->HandleGetCurrentConfigCommand(handler);
    }

    static bool HandleJoyousJourneysCommand(ChatHandler* handler, bool enable)
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        if (!mod->IsJoyousJourneysActive())
        {
            handler->PSendSysMessage("The Joyous Journeys event is not enabled on this server.");
            handler->SetSentErrorMessage(true);
            return true;
        }

        handler->GetPlayer()->UpdatePlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_JOYOUS_JOURNEYS, !enable);
        handler->PSendSysMessage("Joyous Journeys experience boost {}.", !enable ? "disabled" : "enabled");
        return true;
    }

    static bool HandleJoyousJourneysScheduleCommand(ChatHandler* handler, Optional<std::string> startDate, Optional<std::string> endDate)
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        return mod->HandleJoyousJourneysScheduleCommand(handler, startDate, endDate);
    }
};

class DoubleXpWeekendPlayerScript : public PlayerScript
{
public:
    DoubleXpWeekendPlayerScript() : PlayerScript("DoubleXpWeekend", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_GIVE_EXP,
        PLAYERHOOK_ON_GIVE_REPUTATION,
        PLAYERHOOK_ON_UPDATE_GATHERING_SKILL,
        PLAYERHOOK_ON_UPDATE_CRAFTING_SKILL,
        PLAYERHOOK_ON_UPDATE_SKILL,
        PLAYERHOOK_ON_BEFORE_LOOT_MONEY,
        PLAYERHOOK_ON_QUEST_COMPUTE_EXP,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        ChatHandler handler = ChatHandler(player->GetSession());
        mod->OnPlayerLogin(player, &handler);

        if (mod->IsXPBonusEnabled() && mod->IsJoyousJourneysActive() && mod->ConfigJoyousJourneysXPRate())
        {
            if (mod->HasJoyousJourneysSchedule())
                handler.PSendModuleSysMessage("mod-weekend-xp", 8, mod->JoyousJourneysEndDateString());
            else
                handler.PSendSysMessage("|cff00ccffThe Joyous Journeys event is active! Experience gains have been increased. Type .weekendxp j off to disable it.|r");
        }
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 xpSource) override
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        amount = mod->OnPlayerGiveXP(player, amount, xpSource);
    }

    void OnPlayerGiveReputation(Player* /*player*/, int32 factionID, float& amount, ReputationSource /*repSource*/) override
    {
        DoubleXpWeekend::instance()->OnPlayerGiveReputation(factionID, amount);
    }

    void OnPlayerUpdateGatheringSkill(Player* /*player*/, uint32 skill_id, uint32 /*current*/, uint32 /*gray*/,
        uint32 /*green*/, uint32 /*yellow*/, uint32& gain) override
    {
        DoubleXpWeekend::instance()->OnPlayerUpdateGatheringSkill(skill_id, gain);
    }

    void OnPlayerUpdateCraftingSkill(Player* /*player*/, SkillLineAbilityEntry const* skill, uint32 /*current_level*/,
        uint32& gain) override
    {
        DoubleXpWeekend::instance()->OnPlayerUpdateCraftingSkill(skill, gain);
    }

    void OnPlayerUpdateSkill(Player* player, uint32 skillId, uint32 /*value*/, uint32 max, uint32 step,
        uint32 newValue) override
    {
        DoubleXpWeekend::instance()->OnPlayerUpdateSkill(player, skillId, max, step, newValue);
    }

    void OnPlayerBeforeLootMoney(Player* /*player*/, Loot* loot) override
    {
        DoubleXpWeekend::instance()->OnPlayerBeforeLootMoney(loot);
    }

    void OnPlayerQuestComputeXP(Player* player, Quest const* quest, uint32& /*xpValue*/) override
    {
        DoubleXpWeekend::instance()->OnPlayerQuestComputeXP(player, quest);
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        DoubleXpWeekend::instance()->OnPlayerCompleteQuest(player, quest);
    }
};

class DoubleXpWeekendWorldScript : public WorldScript
{
public:
    DoubleXpWeekendWorldScript() : WorldScript("DoubleXpWeekendWorldScript", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD,
        WORLDHOOK_ON_UPDATE
    }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        DoubleXpWeekend::instance()->LoadConfig();
    }

    void OnUpdate(uint32 diff) override
    {
        DoubleXpWeekend::instance()->OnWorldUpdate(diff);
    }
};

void AdddoublexpScripts()
{
    new DoubleXpWeekendPlayerScript();
    new DoubleXpWeekendWorldScript();
    new weekendxp_commandscript();
}
