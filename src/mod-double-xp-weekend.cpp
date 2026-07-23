#include "Configuration/Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "Timer.h"
#include "WorldState.h"
#include <cstdio>
#include <time.h>

using namespace Acore::ChatCommands;

enum WeekendXP
{
    SETTING_WEEKEND_XP_RATE = 0,
    SETTING_WEEKEND_XP_DISABLE = 1,
    SETTING_WEEKEND_XP_VERSION = 2,
    SETTING_WEEKEND_XP_JOYOUS_JOURNEYS = 3,

    WD_FRIDAY   = 5,
    WD_SATURDAY = 6,
    WD_SUNDAY   = 0,
};

// Indices into the characters DB `worldstates` table. Must be unique across the
// core (see WorldStateDefines.h, custom entries start at 20001) and other modules.
enum WeekendXPWorldStates
{
    WS_JOYOUS_JOURNEYS_START = 97301, // unix timestamp, first second of the event
    WS_JOYOUS_JOURNEYS_END   = 97302, // unix timestamp, first second AFTER the event
};

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
        if (!IsEventActive())
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

    void OnPlayerLogin(Player* player, ChatHandler* handler) const
    {
        // TODO I am assuming that this is always called when a character logs in...
        // if that is not the case, thing migh get weird... Adding some asserts or warnings would be nice
        // but I'm not sure how to handle "This shouldn't be happening but it is" kind of scenarios in acore
        MigratePlayerSettings(player, handler);

        if (ConfigAnnounce())
        {
            if (IsEventActive() && !ConfigAlwaysEnabled())
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

        const float actualRate = GetExperienceRate(player);
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
            maxXpRate
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

        return sConfigMgr->GetOption<bool>("XPWeekend.IsJoyousJourneysActive", false);
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
                    sConfigMgr->GetOption<bool>("XPWeekend.IsJoyousJourneysActive", false)));
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
    float ConfigJoyousJourneysRepRate() const { return sConfigMgr->GetOption<float>("XPWeekend.JoyousJourneysRepRate", 1.10f); }
    bool ExcludeInsaneReps() const { return sConfigMgr->GetOption<bool>("XPWeekend.ExcludeInsaneReps", true); }
    float ConfigMaxAllowedRate() const { return sConfigMgr->GetOption<float>("XPWeekend.MaxAllowedRate", 2.0f); }

private:

    // NOTE keep options together to prevent having more than 1 potential default value
    bool ConfigAlwaysEnabled() const { return sConfigMgr->GetOption<bool>("XPWeekend.AlwaysEnabled", false); }
    bool ConfigAnnounce() const { return sConfigMgr->GetOption<bool>("XPWeekend.Announce", false); }
    bool ConfigQuestOnly() const { return sConfigMgr->GetOption<bool>("XPWeekend.QuestOnly", false); }
    uint32 ConfigMaxLevel() const { return sConfigMgr->GetOption<uint32>("XPWeekend.MaxLevel", 80); }
    float ConfigxpAmount() const { return sConfigMgr->GetOption<float>("XPWeekend.xpAmount", 2.0f); }
    bool ConfigIndividualXPEnabled() const { return sConfigMgr->GetOption<bool>("XPWeekend.IndividualXPEnabled", false); }
    bool ConfigEnabled() const { return sConfigMgr->GetOption<bool>("XPWeekend.Enabled", false); }
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

            // If config changed, cap it to max allowed
            if (rate > ConfigMaxAllowedRate())
            {
                rate = ConfigMaxAllowedRate();
                player->UpdatePlayerSetting("mod-double-xp-weekend", SETTING_WEEKEND_XP_RATE, ConfigMaxAllowedRate());
            }
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

        time_t t = time(nullptr);
        tm* now = localtime(&t);

        return now->tm_wday == WD_FRIDAY || now->tm_wday == WD_SATURDAY || now->tm_wday == WD_SUNDAY;
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
        PLAYERHOOK_ON_GIVE_REPUTATION
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        ChatHandler handler = ChatHandler(player->GetSession());
        mod->OnPlayerLogin(player, &handler);

        if (mod->IsJoyousJourneysActive() && mod->ConfigJoyousJourneysXPRate())
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
        DoubleXpWeekend* mod = DoubleXpWeekend::instance();
        if (!mod->IsJoyousJourneysActive() || !mod->ConfigJoyousJourneysRepRate())
            return;

        if (mod->ExcludeInsaneReps())
        {
            switch (factionID)
            {
                case 349: // Ravenholdt
                case 87:  // bloodsail bucaneers
                case 21:  // Booty Bay
                case 169: // Steemwhedle Cartel
                case 577: // Everlook
                case 369: // Gadgetzan
                case 470: // Ratchet
                case 909: // Darkmoon Faire
                case 809: // Shen'dralar
                    return;
            }
        }

        amount *= mod->ConfigJoyousJourneysRepRate();
    }

};

void AdddoublexpScripts()
{
    new DoubleXpWeekendPlayerScript();
    new weekendxp_commandscript();
}
