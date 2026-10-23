// =============================================================================
//  SYNTHKEY by Piradex — pastas do utilizador e do sistema
// =============================================================================
#pragma once
#include <juce_core/juce_core.h>

namespace skpaths
{
// pasta de dados do utilizador:  macOS ~/Library/Application Support/Piradex/SYNTHKEY
//                                Windows %APPDATA%\Piradex\SYNTHKEY      Linux ~/.config/Piradex/SYNTHKEY
inline juce::File userDir()
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");
   #endif
    auto d = base.getChildFile ("Piradex").getChildFile ("SYNTHKEY");
    d.createDirectory();
    return d;
}
inline juce::File userPacks()    { auto d = userDir().getChildFile ("Packs");   d.createDirectory(); return d; }
inline juce::File userPresets()  { auto d = userDir().getChildFile ("Presets"); d.createDirectory(); return d; }

// instalação para todos os utilizadores:  macOS /Library/Application Support/...   Windows C:\ProgramData\...
inline juce::File sharedPacks()
{
   #if JUCE_MAC
    return juce::File ("/Library/Application Support/Piradex/SYNTHKEY/Packs");
   #elif JUCE_WINDOWS
    return juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory).getChildFile ("Piradex").getChildFile ("SYNTHKEY").getChildFile ("Packs");
   #else
    return juce::File ("/usr/share/piradex/synthkey/packs");
   #endif
}
inline juce::File adminKeyFile()  { return userDir().getChildFile ("admin.key"); }
inline juce::File issuedFile()    { return userDir().getChildFile ("issued.json"); }
inline juce::File settingsFile()  { return userDir().getChildFile ("settings.xml"); }
}
