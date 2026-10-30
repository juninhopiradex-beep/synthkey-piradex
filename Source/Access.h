// =============================================================================
//  SYNTHKEY by Piradex — acesso: login (master / beta), ativação de packs, administrador
// =============================================================================
#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <vector>
#include "Core/Licensing.h"

class AccessManager
{
public:
    static AccessManager& get();

    // ---- login -----------------------------------------------------------
    static bool inTrial()          { return sk::lic::todayDays2026() <= sk::keys::daysSince2026 (2026, 12, 31); } // aberto ate 31/12/2026
    bool isUnlocked() const        { return unlocked.load() || inTrial(); }
    sk::lic::Role role() const     { auto r = (sk::lic::Role) roleValue.load(); return (r == sk::lic::Role::None && inTrial()) ? sk::lic::Role::Beta : r; }
    juce::String roleText() const;                       // "MESTRE" / "BETA · até 01/11/2026"
    int  betaDaysLeft() const;                           // -1 se não for beta
    struct Match { int index = -1; bool expired = false; };
    static Match matchPassword (const juce::String& password);        // lento (PBKDF2): chamar fora da thread de mensagens
    enum class LoginResult { Ok, Wrong, Expired };
    LoginResult applyLogin (Match m, bool remember);
    void logout();
    void checkExpiry();                                   // chamar periodicamente (ex. 1x/min)

    // ---- packs -----------------------------------------------------------
    sk::lic::PackStatus packStatus (const std::string& packId, bool core) const;
    sk::lic::ActivateResult activate (const std::string& packId, const juce::String& keyText);
    std::vector<sk::lic::Activation> activations() const;
    bool simulateCustomer() const  { return simulate.load(); }
    void setSimulateCustomer (bool b) { simulate.store (b); }
    static constexpr bool kBetaOpensAllPacks = true;      // beta abre todos os packs até caducar

    // ---- administrador ---------------------------------------------------
    bool isAdmin() const           { return adminOk.load(); }
    void refreshAdmin();                                  // lê admin.key (lento: PBKDF2)
    std::vector<sk::lic::IssuedKey> issuedKeys() const;
    std::vector<sk::lic::IssuedKey> issue (const sk::lic::IssueRequest& r);
    juce::String issuedCsv (const std::vector<sk::lic::IssuedKey>& keys) const;
    bool isIssuedKeyActivatedHere (const sk::lic::IssuedKey& k) const;
    void removeIssued (const juce::String& key);

    // ---- utilitários -----------------------------------------------------
    int today() const;                                    // dia atual com guarda contra recuo do relógio
    static std::string secret();

private:
    AccessManager();
    void setRole (int index);
    void saveActivations();
    void loadIssued() const;
    void saveIssued() const;
    juce::String tokenFor (int index) const;
    mutable juce::CriticalSection lock;
    std::unique_ptr<juce::PropertiesFile> props;
    std::atomic<bool> unlocked { false }, adminOk { false }, simulate { false };
    std::atomic<int> roleValue { 0 };
    int currentIndex = -1;
    std::vector<sk::lic::Activation> acts;
    mutable std::vector<sk::lic::IssuedKey> issuedCache; mutable bool issuedLoaded = false; mutable uint32_t nextSerial = 1;
    mutable int lastSeenDay = 0;
};
