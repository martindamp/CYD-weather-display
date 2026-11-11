// UI translations. To add a language: add it to Lang (before L_COUNT), add its
// code + native name to LANG_CODES / LANG_NAMES, and add one column to each of
// the tables below (STRINGS, DOW, WX_DESC), in the same order as Lang.
//
// Text is UTF-8 and must stay within Latin-1 (the u8g2 "_tf" fonts): no "œ",
// curly quotes or "…" (use "...").
#pragma once
#include <Arduino.h>

enum Lang : uint8_t { L_EN, L_DA, L_DE, L_FR, L_COUNT };

static const char* const LANG_CODES[L_COUNT] = {"en", "da", "de", "fr"};
static const char* const LANG_NAMES[L_COUNT] = {"English", "Dansk", "Deutsch", "Fran\xC3\xA7" "ais"};

enum StrId : uint8_t {
  S_NO_LOCATIONS, S_ADD_LOCATION_BTN, S_ALSO_BROWSER,
  S_CONNECT_WIFI, S_CANT_REACH, S_WIFI_HELP1, S_WIFI_HELP2, S_WIFI_HELP3, S_CONNECTING,
  S_OFFLINE, S_WAIT_WIFI, S_WX_FAILED, S_WX_LOADING, S_FEELS, S_WIND, S_TODAY,
  S_MENU, S_ADD_MAX, S_REMOVE, S_REMOVE_CONFIRM, S_CHECK_UPD, S_CHECKING, S_UP_TO_DATE,
  S_INSTALL_UPD, S_GITHUB_FAIL, S_LANGUAGE, S_CLOSE, S_WEATHER_BY,
  S_SEARCH_PH, S_HINT, S_SEARCHING, S_NO_MATCH, S_SEARCH_FAIL, S_FULL,
  S_KEY_SPACE, S_KEY_DEL, S_KEY_SEARCH,
  S_UPDATING, S_DONT_UNPLUG, S_RESTARTS, S_ALREADY, S_UPD_FAILED,
  S_COUNT
};

// Columns: English, Dansk, Deutsch, Français.  "%s" = argument (see trf()).
static const char* const STRINGS[S_COUNT][L_COUNT] = {
  /* S_NO_LOCATIONS */     {"No locations yet", "Ingen steder endnu", "Noch keine Orte", "Aucun lieu pour l'instant"},
  /* S_ADD_LOCATION_BTN */ {"+  Add location", "+  Tilf\xC3\xB8j sted", "+  Ort hinzuf\xC3\xBCgen", "+  Ajouter un lieu"},
  /* S_ALSO_BROWSER */     {"You can also add locations from a browser:", "Du kan ogs\xC3\xA5 tilf\xC3\xB8je steder fra en browser:",
                            "Orte lassen sich auch im Browser hinzuf\xC3\xBCgen:", "Ajoutez aussi des lieux depuis un navigateur :"},
  /* S_CONNECT_WIFI */     {"Connect to Wi-Fi", "Forbind til Wi-Fi", "Mit WLAN verbinden", "Connexion au Wi-Fi"},
  /* S_CANT_REACH */       {"Can't reach \"%s\" - retrying", "Kan ikke n\xC3\xA5 \"%s\" - pr\xC3\xB8ver igen",
                            "\"%s\" nicht erreichbar - Neuversuch", "\"%s\" injoignable - nouvel essai"},
  /* S_WIFI_HELP1 */       {"Plug the CYD into a computer, open", "Slut CYD'en til en computer, \xC3\xA5" "bn",
                            "Den CYD an einen Computer anschlie\xC3\x9F" "en,", "Branchez le CYD \xC3\xA0 un ordinateur, ouvrez"},
  /* S_WIFI_HELP2 */       {"the web installer in Chrome or Edge", "web-installeren i Chrome eller Edge",
                            "den Web-Installer in Chrome oder Edge", "l'installateur web dans Chrome ou Edge"},
  /* S_WIFI_HELP3 */       {"and set up Wi-Fi there:", "og ops\xC3\xA6t Wi-Fi der:", "\xC3\xB6" "ffnen und dort WLAN einrichten:",
                            "et configurez le Wi-Fi :"},
  /* S_CONNECTING */       {"Connecting to %s...", "Forbinder til %s...", "Verbinde mit %s...", "Connexion \xC3\xA0 %s..."},
  /* S_OFFLINE */          {"offline", "offline", "offline", "hors ligne"},
  /* S_WAIT_WIFI */        {"Waiting for Wi-Fi...", "Venter p\xC3\xA5 Wi-Fi...", "Warte auf WLAN...", "En attente du Wi-Fi..."},
  /* S_WX_FAILED */        {"Couldn't get weather - retrying", "Kunne ikke hente vejret - pr\xC3\xB8ver igen",
                            "Wetter nicht abrufbar - neuer Versuch", "M\xC3\xA9t\xC3\xA9o indisponible - nouvel essai"},
  /* S_WX_LOADING */       {"Loading weather...", "Henter vejret...", "Lade Wetter...", "Chargement de la m\xC3\xA9t\xC3\xA9o..."},
  /* S_FEELS */            {"Feels", "F\xC3\xB8les", "Gef\xC3\xBChlt", "Ressenti"},
  /* S_WIND */             {"Wind", "Vind", "Wind", "Vent"},
  /* S_TODAY */            {"Today", "I dag", "Heute", "Auj."},
  /* S_MENU */             {"Menu", "Menu", "Men\xC3\xBC", "Menu"},
  /* S_ADD_MAX */          {"Add location (max reached)", "Tilf\xC3\xB8j sted (maks. n\xC3\xA5" "et)",
                            "Ort hinzuf\xC3\xBCgen (Maximum erreicht)", "Ajouter un lieu (maximum atteint)"},
  /* S_REMOVE */           {"Remove %s", "Fjern %s", "%s entfernen", "Supprimer %s"},
  /* S_REMOVE_CONFIRM */   {"Tap again to remove %s", "Tryk igen for at fjerne %s", "Erneut tippen: %s entfernen",
                            "Confirmer : supprimer %s"},
  /* S_CHECK_UPD */        {"Check for updates", "S\xC3\xB8g efter opdateringer", "Nach Updates suchen", "Rechercher des mises \xC3\xA0 jour"},
  /* S_CHECKING */         {"Checking...", "S\xC3\xB8ger...", "Suche...", "Recherche..."},
  /* S_UP_TO_DATE */       {"Up to date (%s)", "Opdateret (%s)", "Aktuell (%s)", "\xC3\x80 jour (%s)"},
  /* S_INSTALL_UPD */      {"Install update %s", "Installer opdatering %s", "Update %s installieren", "Installer la mise \xC3\xA0 jour %s"},
  /* S_GITHUB_FAIL */      {"Couldn't reach GitHub - retry", "Kunne ikke n\xC3\xA5 GitHub - pr\xC3\xB8v igen",
                            "GitHub nicht erreichbar - erneut", "GitHub injoignable - r\xC3\xA9" "essayer"},
  /* S_LANGUAGE */         {"Language: %s", "Sprog: %s", "Sprache: %s", "Langue : %s"},
  /* S_CLOSE */            {"Close", "Luk", "Schlie\xC3\x9F" "en", "Fermer"},
  /* S_WEATHER_BY */       {"Weather: Open-Meteo.com", "Vejr: Open-Meteo.com", "Wetter: Open-Meteo.com", "M\xC3\xA9t\xC3\xA9o : Open-Meteo.com"},
  /* S_SEARCH_PH */        {"Search for a city or postcode", "S\xC3\xB8g efter by eller postnummer", "Stadt oder PLZ suchen",
                            "Ville ou code postal"},
  /* S_HINT */             {"Type a place name - suggestions appear here", "Skriv et stednavn - forslag vises her",
                            "Ortsnamen eingeben - Vorschl\xC3\xA4ge hier", "Tapez un lieu - suggestions ici"},
  /* S_SEARCHING */        {"Searching...", "S\xC3\xB8ger...", "Suche...", "Recherche..."},
  /* S_NO_MATCH */         {"No matches - check the spelling", "Ingen resultater - tjek stavningen",
                            "Keine Treffer - Schreibweise pr\xC3\xBC" "fen", "Aucun r\xC3\xA9sultat - v\xC3\xA9rifiez l'orthographe"},
  /* S_SEARCH_FAIL */      {"Search failed - check Wi-Fi", "S\xC3\xB8gning fejlede - tjek Wi-Fi",
                            "Suche fehlgeschlagen - WLAN pr\xC3\xBC" "fen", "\xC3\x89" "chec de la recherche - v\xC3\xA9rifiez le Wi-Fi"},
  /* S_FULL */             {"Maximum number of locations reached", "Maks. antal steder er n\xC3\xA5" "et",
                            "Maximale Anzahl Orte erreicht", "Nombre maximal de lieux atteint"},
  /* S_KEY_SPACE */        {"space", "mellemrum", "Leerzeichen", "espace"},
  /* S_KEY_DEL */          {"Del", "Slet", "Entf", "Suppr"},
  /* S_KEY_SEARCH */       {"Search", "S\xC3\xB8g", "Suchen", "Chercher"},
  /* S_UPDATING */         {"Updating firmware", "Opdaterer firmware", "Firmware-Update", "Mise \xC3\xA0 jour du firmware"},
  /* S_DONT_UNPLUG */      {"Don't unplug the display", "Tag ikke str\xC3\xB8mmen fra sk\xC3\xA6rmen", "Display nicht ausstecken",
                            "Ne d\xC3\xA9" "branchez pas l'\xC3\xA9" "cran"},
  /* S_RESTARTS */         {"It restarts by itself when done", "Den genstarter selv, n\xC3\xA5r den er f\xC3\xA6rdig",
                            "Startet danach automatisch neu", "Il red\xC3\xA9marre tout seul \xC3\xA0 la fin"},
  /* S_ALREADY */          {"Already up to date", "Allerede opdateret", "Bereits aktuell", "D\xC3\xA9j\xC3\xA0 \xC3\xA0 jour"},
  /* S_UPD_FAILED */       {"Update failed", "Opdatering fejlede", "Update fehlgeschlagen", "\xC3\x89" "chec de la mise \xC3\xA0 jour"},
};

// Short weekday names, Sunday first.
static const char* const DOW[L_COUNT][7] = {
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"S\xC3\xB8n", "Man", "Tir", "Ons", "Tor", "Fre", "L\xC3\xB8r"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
  {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"},
};

// ---- weather descriptions (WMO codes) ----
enum WxDesc : uint8_t {
  D_CLEAR, D_MAINLY_CLEAR, D_PARTLY, D_OVERCAST, D_FOG, D_FREEZING_FOG,
  D_LIGHT_DRIZZLE, D_DRIZZLE, D_HEAVY_DRIZZLE, D_FREEZING_DRIZZLE,
  D_LIGHT_RAIN, D_RAIN, D_HEAVY_RAIN, D_FREEZING_RAIN,
  D_LIGHT_SNOW, D_SNOW, D_HEAVY_SNOW, D_SNOW_GRAINS,
  D_LIGHT_SHOWERS, D_SHOWERS, D_HEAVY_SHOWERS, D_SNOW_SHOWERS,
  D_THUNDER, D_THUNDER_HAIL, D_UNKNOWN, D_COUNT
};

static const char* const WX_DESC[D_COUNT][L_COUNT] = {
  /* D_CLEAR */            {"Clear sky", "Klar himmel", "Klarer Himmel", "Ciel d\xC3\xA9gag\xC3\xA9"},
  /* D_MAINLY_CLEAR */     {"Mainly clear", "Overvejende klart", "\xC3\x9C" "berwiegend klar", "Plut\xC3\xB4t d\xC3\xA9gag\xC3\xA9"},
  /* D_PARTLY */           {"Partly cloudy", "Delvis skyet", "Teilweise bew\xC3\xB6lkt", "Partiellement nuageux"},
  /* D_OVERCAST */         {"Overcast", "Overskyet", "Bedeckt", "Couvert"},
  /* D_FOG */              {"Fog", "T\xC3\xA5ge", "Nebel", "Brouillard"},
  /* D_FREEZING_FOG */     {"Freezing fog", "Rimt\xC3\xA5ge", "Gefrierender Nebel", "Brouillard givrant"},
  /* D_LIGHT_DRIZZLE */    {"Light drizzle", "Let st\xC3\xB8vregn", "Leichter Nieselregen", "Bruine l\xC3\xA9g\xC3\xA8re"},
  /* D_DRIZZLE */          {"Drizzle", "St\xC3\xB8vregn", "Nieselregen", "Bruine"},
  /* D_HEAVY_DRIZZLE */    {"Heavy drizzle", "Kraftig st\xC3\xB8vregn", "Starker Nieselregen", "Forte bruine"},
  /* D_FREEZING_DRIZZLE */ {"Freezing drizzle", "Underafk\xC3\xB8let st\xC3\xB8vregn", "Gefrierender Nieselregen", "Bruine vergla\xC3\xA7" "ante"},
  /* D_LIGHT_RAIN */       {"Light rain", "Let regn", "Leichter Regen", "Pluie l\xC3\xA9g\xC3\xA8re"},
  /* D_RAIN */             {"Rain", "Regn", "Regen", "Pluie"},
  /* D_HEAVY_RAIN */       {"Heavy rain", "Kraftig regn", "Starker Regen", "Forte pluie"},
  /* D_FREEZING_RAIN */    {"Freezing rain", "Isslag", "Gefrierender Regen", "Pluie vergla\xC3\xA7" "ante"},
  /* D_LIGHT_SNOW */       {"Light snow", "Let sne", "Leichter Schneefall", "Neige l\xC3\xA9g\xC3\xA8re"},
  /* D_SNOW */             {"Snow", "Sne", "Schneefall", "Neige"},
  /* D_HEAVY_SNOW */       {"Heavy snow", "Kraftig sne", "Starker Schneefall", "Forte neige"},
  /* D_SNOW_GRAINS */      {"Snow grains", "Kornsne", "Schneegriesel", "Neige en grains"},
  /* D_LIGHT_SHOWERS */    {"Light showers", "Lette byger", "Leichte Schauer", "Averses l\xC3\xA9g\xC3\xA8res"},
  /* D_SHOWERS */          {"Rain showers", "Regnbyger", "Regenschauer", "Averses"},
  /* D_HEAVY_SHOWERS */    {"Heavy showers", "Kraftige byger", "Starke Schauer", "Fortes averses"},
  /* D_SNOW_SHOWERS */     {"Snow showers", "Snebyger", "Schneeschauer", "Averses de neige"},
  /* D_THUNDER */          {"Thunderstorm", "Torden", "Gewitter", "Orage"},
  /* D_THUNDER_HAIL */     {"Thunder & hail", "Torden og hagl", "Gewitter mit Hagel", "Orage avec gr\xC3\xAAle"},
  /* D_UNKNOWN */          {"Unknown", "Ukendt", "Unbekannt", "Inconnu"},
};

static inline WxDesc wxDescFor(int code) {
  switch (code) {
    case 0:  return D_CLEAR;
    case 1:  return D_MAINLY_CLEAR;
    case 2:  return D_PARTLY;
    case 3:  return D_OVERCAST;
    case 45: return D_FOG;
    case 48: return D_FREEZING_FOG;
    case 51: return D_LIGHT_DRIZZLE;
    case 53: return D_DRIZZLE;
    case 55: return D_HEAVY_DRIZZLE;
    case 56: case 57: return D_FREEZING_DRIZZLE;
    case 61: return D_LIGHT_RAIN;
    case 63: return D_RAIN;
    case 65: return D_HEAVY_RAIN;
    case 66: case 67: return D_FREEZING_RAIN;
    case 71: return D_LIGHT_SNOW;
    case 73: return D_SNOW;
    case 75: return D_HEAVY_SNOW;
    case 77: return D_SNOW_GRAINS;
    case 80: return D_LIGHT_SHOWERS;
    case 81: return D_SHOWERS;
    case 82: return D_HEAVY_SHOWERS;
    case 85: case 86: return D_SNOW_SHOWERS;
    case 95: return D_THUNDER;
    case 96: case 99: return D_THUNDER_HAIL;
    default: return D_UNKNOWN;
  }
}

// ---- current language ----
static Lang g_lang = L_EN;

static inline const char* tr(StrId id) { return STRINGS[id][g_lang]; }
static inline const char* trDow(int d) { return DOW[g_lang][d % 7]; }
static inline const char* trWx(int code) { return WX_DESC[wxDescFor(code)][g_lang]; }
static inline const char* langCode() { return LANG_CODES[g_lang]; }

// tr() with one "%s" argument.
static inline String trf(StrId id, const String& arg) {
  String s = tr(id);
  int p = s.indexOf("%s");
  if (p < 0) return s;
  return s.substring(0, p) + arg + s.substring(p + 2);
}

static inline Lang langFromCode(const String& c) {
  for (int i = 0; i < L_COUNT; i++) if (c == LANG_CODES[i]) return (Lang)i;
  return L_EN;
}
