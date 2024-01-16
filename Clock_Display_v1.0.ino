#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Font_Data.h"
#include <DS3231.h>
#include <Wire.h>

DS3231 Clock;

bool Century = false;
bool h24 = true; //24-godzinny format czasu
bool PM;
byte dd, mm, yyy;
uint16_t h, m, s;

#define MAX_DEVICES 4
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // Rodzaj modułu
// Pinout dla modułu
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define SPEED_TIME 55
#define PAUSE_TIME 0

#define MAX_MESG  20

char szTime[9];
char szMesg[MAX_MESG + 1] = "";

// Tablice znaków do wyświetlania stopni C
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 };

// Funkcja zamieniająca numer miesiąca na jego skrót
char *mon2str(uint8_t mon, char *psz, uint8_t len)
{
  static const __FlashStringHelper *str[] =
  {
    F("Sty"), F("Luty"), F("Mar"), F("Kwi"),
    F("Maj"), F("Cze"), F("Lip"), F("Sie"),
    F("Wrz"), F("Paz"), F("Lis"), F("Gru")
  };

  strncpy_P(psz, (const char PROGMEM *)str[mon - 1], len);
  psz[len] = '\0';

  return psz;
}

// Funkcja zamieniająca numer dnia tygodnia na jego nazwę
char *dow2str(uint8_t code, char *psz, uint8_t len)
{
  static const __FlashStringHelper *str[] =
  {
    F("Niedziela"), F("Poniedzialek"), F("Wtorek"),
    F("Sroda"), F("Czwartek"), F("Piatek"),
    F("Sobota")
  };

  strncpy_P(psz, (const char PROGMEM *)str[code], len);
  psz[len] = '\0';

  return psz;
}

// Funkcja pobierająca aktualny czas i formatująca go
void getTime(char *psz, bool f = true)
{
  s = Clock.getSecond();
  m = Clock.getMinute();
  h = Clock.getHour(h24, PM);

  sprintf(psz, "%02d%c%02d", h, (f ? ':' : ' '), m);

  if (h >= 13 || h == 0)
  {
    h = h - 12;
  }
}

// Funkcja pobierająca aktualną datę i formatująca ją
void getDate(char *psz)
{
  char  szBuf[10];

  dd = Clock.getDate();
  mm = Clock.getMonth(Century);
  yyy = Clock.getYear();
  sprintf(psz, "%d %s %04d", dd, mon2str(mm, szBuf, sizeof(szBuf) - 1), (yyy + 2000));
}

// Inicjalizacja ustawień
void setup(void)
{
  P.begin(2);
  P.setInvert(false);
  Wire.begin();

  P.setZone(0, MAX_DEVICES - 4, MAX_DEVICES - 1);

  P.setZone(1, MAX_DEVICES - 4, MAX_DEVICES - 1);
  P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, 0, PA_PRINT, PA_NO_EFFECT);

  P.addChar('$', degC); // Usunięto dodanie znaku stopnia F
}

// Główna pętla programu
void loop(void)
{
  static uint32_t lastTime = 0;
  static uint8_t  display = 0;
  static bool flasher = false;

  P.displayAnimate();

  if (P.getZoneStatus(0))
  {
    switch (display)
    {
      case 0:    // Temperatura deg C
        P.setPause(0, 1000);
        P.setTextEffect(0, PA_SCROLL_UP, PA_SCROLL_LEFT);
        display++;
        dtostrf(Clock.getTemperature(), 3, 1, szMesg);
        strcat(szMesg, "$");
        break;

      case 1: // Czas
        P.setFont(0, numeric7Seg);
        P.setTextEffect(0, PA_PRINT, PA_NO_EFFECT);
        P.setPause(0, 0);

        if (millis() - lastTime >= 1000)
        {
          lastTime = millis();
          getTime(szMesg, flasher);
          flasher = !flasher;
        }

        if (s == 0 && s <= 30)
        {
          display++;
          P.setTextEffect(0, PA_CLOSING, PA_GROW_DOWN);
        }
        break;

      case 2: // Dzień tygodnia
        P.setFont(0, nullptr);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        dow2str(Clock.getDoW(), szMesg, MAX_MESG);
        break;

      case 3: // Data
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        getDate(szMesg);
        break;

      case 4: // Wiadomość
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        strcpy(szMesg, "Design LAB");
        display = 0;
        break;

      default:
        display = 0;
        break;
    }

    P.displayReset(0); // Zresetuj strefę zero
  }
}