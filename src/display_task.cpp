/*
 * Author: Yang
 * FastLED render engine pinned to Core 1.
 */
#include "display_task.h"

#include <cmath>
#include <cstring>

#include <Arduino.h>
#include <FastLED.h>

#include "app_config.h"
#include "app_state.h"
#include "game_logic.h"
#include "pinmap.h"
#include "timer_logic.h"

namespace {
CRGB leds[AppConfig::LedCount];
CRGB outputLeds[AppConfig::LedCount];
CRGB transitionFrom[AppConfig::LedCount];
TaskHandle_t displayTaskHandle = nullptr;
uint8_t gammaLut[256] = {};
uint32_t activeRenderKey = UINT32_MAX;
uint32_t transitionStartedMs = 0;
uint8_t currentBrightness = 1;

struct Particle {
  float x;
  float y;
  float vx;
  float vy;
  CRGB color;
};

struct ClockTime {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

constexpr size_t ParticleCount = AppConfig::FluidParticleCount;
constexpr uint8_t FluidGridW = AppConfig::MatrixWidth;
constexpr uint8_t FluidGridH = AppConfig::MatrixHeight;
constexpr size_t FluidCellCount = FluidGridW * FluidGridH;
constexpr bool FluidVisualFlipX = true;
constexpr bool FluidVisualFlipY = true;
constexpr bool UiFlipX = FluidVisualFlipX;
constexpr bool UiFlipY = FluidVisualFlipY;
constexpr bool GameFlipX = false;
constexpr bool GameFlipY = false;
constexpr bool FluidGravityFlipX = FluidVisualFlipX;
constexpr bool FluidGravityFlipY = FluidVisualFlipY;
Particle particles[ParticleCount];
float fluidU[FluidCellCount];
float fluidV[FluidCellCount];
float fluidPrevU[FluidCellCount];
float fluidPrevV[FluidCellCount];
float fluidWeight[FluidCellCount];
float fluidDensity[FluidCellCount];
float fluidPressure[FluidCellCount];
bool fluidCell[FluidCellCount];

const uint8_t font3x5[][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b010, 0b010, 0b010}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
};

const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 0x20 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 0x21 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 0x22 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 0x23 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 0x24 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 0x25 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 0x26 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 0x27 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 0x28 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 0x29 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 0x2A '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 0x2B '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 0x2C ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 0x2D '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 0x2E '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 0x2F '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0x30 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 0x31 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 0x32 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 0x33 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 0x34 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 0x35 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 0x36 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 0x37 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 0x38 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 0x39 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 0x3A ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 0x3B ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 0x3C '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 0x3D '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 0x3E '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 0x3F '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 0x40 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 0x41 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 0x42 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 0x43 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 0x44 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 0x45 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 0x46 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 0x47 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 0x48 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 0x49 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 0x4A 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 0x4B 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 0x4C 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 0x4D 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 0x4E 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 0x4F 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 0x50 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 0x51 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 0x52 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 0x53 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 0x54 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 0x55 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 0x56 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 0x57 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 0x58 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 0x59 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 0x5A 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 0x5B '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 0x5C '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 0x5D ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 0x5E '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 0x5F '_'
};

uint16_t xy(uint8_t x, uint8_t y) {
  if (x >= AppConfig::MatrixWidth || y >= AppConfig::MatrixHeight) {
    return 0;
  }
  const uint16_t columnStart = x * AppConfig::MatrixHeight;
  if (x & 0x01) {
    return columnStart + (AppConfig::MatrixHeight - 1 - y);
  }
  return columnStart + y;
}

bool mapPixelPoint(int x, int y, bool flipX, bool flipY, uint8_t &mappedX, uint8_t &mappedY) {
  if (x < 0 || y < 0 || x >= AppConfig::MatrixWidth || y >= AppConfig::MatrixHeight) {
    return false;
  }
  mappedX = static_cast<uint8_t>(flipX ? AppConfig::MatrixWidth - 1 - x : x);
  mappedY = static_cast<uint8_t>(flipY ? AppConfig::MatrixHeight - 1 - y : y);
  return true;
}

void addPixelOriented(int x, int y, const CRGB &color, bool flipX, bool flipY) {
  uint8_t mappedX = 0;
  uint8_t mappedY = 0;
  if (!mapPixelPoint(x, y, flipX, flipY, mappedX, mappedY)) {
    return;
  }
  leds[xy(mappedX, mappedY)] += color;
}

void setPixelOriented(int x, int y, const CRGB &color, bool flipX, bool flipY) {
  uint8_t mappedX = 0;
  uint8_t mappedY = 0;
  if (!mapPixelPoint(x, y, flipX, flipY, mappedX, mappedY)) {
    return;
  }
  leds[xy(mappedX, mappedY)] = color;
}

void addUiPixel(int x, int y, const CRGB &color) {
  addPixelOriented(x, y, color, UiFlipX, UiFlipY);
}

void setUiPixel(int x, int y, const CRGB &color) {
  setPixelOriented(x, y, color, UiFlipX, UiFlipY);
}

void addGamePixel(int x, int y, const CRGB &color) {
  addPixelOriented(x, y, color, GameFlipX, GameFlipY);
}

void setGamePixel(int x, int y, const CRGB &color) {
  setPixelOriented(x, y, color, GameFlipX, GameFlipY);
}

void addFluidPixel(int x, int y, const CRGB &color) {
  addPixelOriented(x, y, color, FluidVisualFlipX, FluidVisualFlipY);
}

void drawDigit(int x, int y, uint8_t digit, const CRGB &color) {
  if (digit > 9) {
    return;
  }
  for (uint8_t row = 0; row < 5; ++row) {
    for (uint8_t col = 0; col < 3; ++col) {
      if (font3x5[digit][row] & (0b100 >> col)) {
        addUiPixel(x + col, y + row, color);
      }
    }
  }
}

void drawColon(int x, int y, const CRGB &color) {
  addUiPixel(x, y + 1, color);
  addUiPixel(x, y + 3, color);
}

ClockTime getClockTime(const RenderState &state) {
  struct tm tmNow = {};
  localtime_r(&state.unixTime, &tmNow);
  return ClockTime{
      static_cast<uint8_t>(tmNow.tm_hour),
      static_cast<uint8_t>(tmNow.tm_min),
      static_cast<uint8_t>(tmNow.tm_sec),
  };
}

void drawTimeHHMM(const ClockTime &time, const CRGB &color, const CRGB &colonColor) {
  drawDigit(7, 1, time.hour / 10, color);
  drawDigit(11, 1, time.hour % 10, color);
  drawColon(15, 1, colonColor);
  drawDigit(17, 1, time.minute / 10, color);
  drawDigit(21, 1, time.minute % 10, color);
}

void drawDigitRainbow(int x, int y, uint8_t digit, uint8_t baseHue, uint8_t value) {
  if (digit > 9) {
    return;
  }
  for (uint8_t row = 0; row < 5; ++row) {
    for (uint8_t col = 0; col < 3; ++col) {
      if (font3x5[digit][row] & (0b100 >> col)) {
        addUiPixel(x + col, y + row, CHSV(baseHue + (x + col) * 7 + row * 5, 230, value));
      }
    }
  }
}

void drawColonRainbow(int x, int y, uint8_t baseHue, uint8_t value) {
  addUiPixel(x, y + 1, CHSV(baseHue + x * 7 + 12, 180, value));
  addUiPixel(x, y + 3, CHSV(baseHue + x * 7 + 28, 180, value));
}

void drawTimeHHMMRainbow(const ClockTime &time, uint8_t baseHue, uint8_t value) {
  drawDigitRainbow(7, 1, time.hour / 10, baseHue, value);
  drawDigitRainbow(11, 1, time.hour % 10, baseHue, value);
  drawColonRainbow(15, 1, baseHue, value);
  drawDigitRainbow(17, 1, time.minute / 10, baseHue, value);
  drawDigitRainbow(21, 1, time.minute % 10, baseHue, value);
}

void drawSecondMarker(uint8_t second, const CRGB &color) {
  const uint8_t secX = map(second, 0, 59, 0, AppConfig::MatrixWidth - 1);
  setUiPixel(secX, AppConfig::MatrixHeight - 1, color);
}

void drawTemperatureBar(float temperatureC, const CRGB &color) {
  if (!isnan(temperatureC)) {
    const uint8_t tempHeight = constrain(lroundf(temperatureC), 0L, 40L) / 4;
    for (uint8_t y = 0; y < tempHeight && y < AppConfig::MatrixHeight; ++y) {
      addUiPixel(AppConfig::MatrixWidth - 1, AppConfig::MatrixHeight - 1 - y, color);
    }
  }
}

void renderClockClassic(const RenderState &state, const ClockTime &time) {
  fadeToBlackBy(leds, AppConfig::LedCount, 48);
  CRGB secondary = state.control.preferredColor;
  secondary.nscale8_video(176);
  CRGB secondMarker = state.control.preferredColor;
  secondMarker.nscale8_video(128);
  drawTimeHHMM(time, state.control.preferredColor, secondary);
  drawSecondMarker(time.second, secondMarker);
  drawTemperatureBar(state.environment.temperatureC, CRGB(255, 80, 32));
}

void renderClockRainbow(const RenderState &state, const ClockTime &time) {
  fadeToBlackBy(leds, AppConfig::LedCount, 64);
  const uint8_t baseHue = millis() / 40;
  drawTimeHHMMRainbow(time, baseHue, 170);
  drawSecondMarker(time.second, CHSV(baseHue + time.second * 4, 220, 140));
  drawTemperatureBar(state.environment.temperatureC, CHSV(baseHue + 48, 180, 110));
}

void renderClockBreath(const RenderState &state, const ClockTime &time) {
  fadeToBlackBy(leds, AppConfig::LedCount, 56);
  CRGB color = state.control.preferredColor;
  color.nscale8_video(beatsin8(12, 48, 210));
  drawTimeHHMM(time, color, color);
  drawSecondMarker(time.second, color);
  drawTemperatureBar(state.environment.temperatureC, CRGB(120, 36, 18));
}

void renderClockNight(const ClockTime &time) {
  fadeToBlackBy(leds, AppConfig::LedCount, 80);
  drawTimeHHMM(time, CRGB(22, 8, 2), CRGB(14, 4, 1));
  drawSecondMarker(time.second, CRGB(9, 2, 0));
}

void renderClockMinimal(const ClockTime &time) {
  fadeToBlackBy(leds, AppConfig::LedCount, 96);
  drawTimeHHMM(time, CRGB(70, 70, 70), CRGB(44, 44, 44));
  if ((time.second & 0x01) == 0) {
    setUiPixel(AppConfig::MatrixWidth - 1, AppConfig::MatrixHeight - 1, CRGB(26, 26, 26));
  }
}

void renderClock(const RenderState &state) {
  const ClockTime time = getClockTime(state);
  switch (state.control.clockTheme) {
    case ClockTheme::Rainbow:
      renderClockRainbow(state, time);
      break;
    case ClockTheme::Breath:
      renderClockBreath(state, time);
      break;
    case ClockTheme::Night:
      renderClockNight(time);
      break;
    case ClockTheme::Minimal:
      renderClockMinimal(time);
      break;
    case ClockTheme::Classic:
    default:
      renderClockClassic(state, time);
      break;
  }
}

char normalizeScrollChar(char c) {
  if (c >= 'a' && c <= 'z') {
    return static_cast<char>(c - 'a' + 'A');
  }
  if (c < 0x20 || c > 0x5F) {
    return ' ';
  }
  return c;
}

const uint8_t *getScrollGlyph(char c) {
  c = normalizeScrollChar(c);
  return font5x7[c - 0x20];
}

size_t boundedTextLength(const char *text) {
  size_t len = 0;
  while (len < AppConfig::ScrollTextMaxLen - 1 && text[len] != '\0') {
    ++len;
  }
  return len;
}

void renderScrollingText(const RenderState &state) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);

  const char *text = state.control.scrollText;
  size_t textLen = boundedTextLength(text);
  if (textLen == 0) {
    text = "PIXEL CLOCK";
    textLen = 11;
  }

  constexpr int FontW = 5;
  constexpr int FontH = 7;
  constexpr int CharSpacing = 1;
  constexpr int CharW = FontW + CharSpacing;
  const int textPixelW = static_cast<int>(textLen) * CharW;
  const int totalScrollW = textPixelW + AppConfig::MatrixWidth;
  const uint16_t speedMs = constrain(state.control.scrollSpeedMs, static_cast<uint16_t>(30), static_cast<uint16_t>(500));
  const int offset = (millis() / speedMs) % totalScrollW;

  for (int screenX = 0; screenX < AppConfig::MatrixWidth; ++screenX) {
    const int sourceX = offset + screenX - AppConfig::MatrixWidth;
    if (sourceX < 0 || sourceX >= textPixelW) {
      continue;
    }

    const int charIndex = sourceX / CharW;
    const int colInChar = sourceX % CharW;
    if (colInChar >= FontW) {
      continue;
    }

    const uint8_t *glyph = getScrollGlyph(text[charIndex]);
    const uint8_t columnBits = glyph[colInChar];
    for (int y = 0; y < FontH; ++y) {
      if ((columnBits & (1 << y)) == 0) {
        continue;
      }

      CRGB color = state.control.preferredColor;
      if (state.control.scrollRainbow) {
        color = CHSV(static_cast<uint8_t>(millis() / 24 + screenX * 7 + y * 3), 205, 255);
      }
      addUiPixel(screenX, y, color);
    }
  }
}

void drawText5x7(const char *text, int x, int y, const CRGB &color) {
  if (!text) {
    return;
  }

  int cursorX = x;
  while (*text && cursorX < static_cast<int>(AppConfig::MatrixWidth)) {
    const uint8_t *glyph = getScrollGlyph(*text++);
    for (uint8_t col = 0; col < 5; ++col) {
      const uint8_t columnBits = glyph[col];
      for (uint8_t row = 0; row < 7; ++row) {
        if (columnBits & (1 << row)) {
          addUiPixel(cursorX + col, y + row, color);
        }
      }
    }
    cursorX += 6;
  }
}

CRGB weatherColor(int code) {
  if (code == 0) {
    return CRGB(255, 178, 30);
  }
  if ((code >= 1 && code <= 3) || code == 45 || code == 48) {
    return CRGB(130, 140, 150);
  }
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
    return CRGB(28, 110, 220);
  }
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
    return CRGB(180, 210, 255);
  }
  if (code >= 95) {
    return CRGB(190, 90, 255);
  }
  return CRGB::White;
}

void drawWeatherIcon(int code, int x0, int y0, const CRGB &color) {
  if (code == 0) {
    setUiPixel(x0 + 3, y0 + 0, color);
    setUiPixel(x0 + 1, y0 + 1, color);
    setUiPixel(x0 + 5, y0 + 1, color);
    for (int x = 2; x <= 4; ++x) {
      setUiPixel(x0 + x, y0 + 2, color);
      setUiPixel(x0 + x, y0 + 3, color);
      setUiPixel(x0 + x, y0 + 4, color);
    }
    setUiPixel(x0 + 0, y0 + 3, color);
    setUiPixel(x0 + 6, y0 + 3, color);
    setUiPixel(x0 + 1, y0 + 5, color);
    setUiPixel(x0 + 5, y0 + 5, color);
    setUiPixel(x0 + 3, y0 + 6, color);
    return;
  }

  const bool cloudy = (code >= 1 && code <= 3) || code == 45 || code == 48;
  const bool rainy = (code >= 51 && code <= 67) || (code >= 80 && code <= 82);
  const bool snowy = (code >= 71 && code <= 77) || code == 85 || code == 86;
  const bool thunder = code >= 95;

  if (cloudy || rainy) {
    CRGB cloud = cloudy ? color : CRGB(80, 90, 105);
    for (int x = 1; x <= 6; ++x) {
      setUiPixel(x0 + x, y0 + 4, cloud);
    }
    for (int x = 2; x <= 5; ++x) {
      setUiPixel(x0 + x, y0 + 3, cloud);
    }
    setUiPixel(x0 + 3, y0 + 2, cloud);
    setUiPixel(x0 + 4, y0 + 2, cloud);
  }

  if (rainy) {
    setUiPixel(x0 + 2, y0 + 6, color);
    setUiPixel(x0 + 4, y0 + 6, color);
    setUiPixel(x0 + 6, y0 + 6, color);
    setUiPixel(x0 + 3, y0 + 7, color);
    setUiPixel(x0 + 5, y0 + 7, color);
    return;
  }

  if (snowy) {
    setUiPixel(x0 + 2, y0 + 2, color);
    setUiPixel(x0 + 4, y0 + 2, color);
    setUiPixel(x0 + 3, y0 + 3, color);
    setUiPixel(x0 + 1, y0 + 4, color);
    setUiPixel(x0 + 5, y0 + 4, color);
    setUiPixel(x0 + 3, y0 + 5, color);
    setUiPixel(x0 + 2, y0 + 6, color);
    setUiPixel(x0 + 4, y0 + 6, color);
    return;
  }

  if (thunder) {
    setUiPixel(x0 + 4, y0 + 1, color);
    setUiPixel(x0 + 3, y0 + 2, color);
    setUiPixel(x0 + 3, y0 + 3, color);
    setUiPixel(x0 + 2, y0 + 4, color);
    setUiPixel(x0 + 4, y0 + 4, color);
    setUiPixel(x0 + 3, y0 + 5, color);
    setUiPixel(x0 + 3, y0 + 6, color);
    return;
  }

  drawText5x7("?", x0 + 1, y0, color);
}

void drawWeatherBar(uint8_t percent, const CRGB &color) {
  const uint8_t width = map(percent, 0, 100, 0, AppConfig::MatrixWidth);
  for (uint8_t x = 0; x < width; ++x) {
    setUiPixel(x, AppConfig::MatrixHeight - 1, color);
  }
}

void renderWeather(const RenderState &state) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);

  if (!state.control.weatherEnabled) {
    drawText5x7("OFF", 7, 0, CRGB(80, 80, 80));
    return;
  }

  const WeatherState &weather = state.weather;
  if (!weather.hasData) {
    drawText5x7(weather.failCount > 0 ? "ERR" : "WAIT", weather.failCount > 0 ? 7 : 4, 0,
                weather.failCount > 0 ? CRGB(180, 40, 40) : CRGB(70, 90, 120));
    return;
  }

  const CRGB iconColor = weatherColor(weather.weatherCode);
  char text[8] = {};

  if (state.control.weatherDisplayMode == WeatherDisplayMode::TempOnly) {
    snprintf(text, sizeof(text), "%dC", static_cast<int>(lroundf(weather.temperature)));
    drawText5x7(text, strlen(text) >= 4 ? 4 : 7, 0, state.control.preferredColor);
    drawWeatherBar(constrain(weather.cloudCover, 0, 100), CRGB(18, 36, 72));
    return;
  }

  if (state.control.weatherDisplayMode == WeatherDisplayMode::DetailCycle) {
    const uint8_t phase = (millis() / 3000) % 4;
    switch (phase) {
      case 0:
        snprintf(text, sizeof(text), "%dC", static_cast<int>(lroundf(weather.temperature)));
        break;
      case 1:
        snprintf(text, sizeof(text), "H%d", constrain(weather.relativeHumidity, 0, 99));
        break;
      case 2:
        snprintf(text, sizeof(text), "P%d", constrain(weather.todayPrecipProb, 0, 99));
        break;
      case 3:
      default:
        snprintf(text, sizeof(text), "W%d", constrain(static_cast<int>(lroundf(weather.windSpeed)), 0, 99));
        break;
    }
    drawWeatherIcon(weather.weatherCode, 0, 0, iconColor);
    drawText5x7(text, 11, 0, state.control.preferredColor);
    drawWeatherBar(constrain(phase == 2 ? weather.todayPrecipProb : weather.cloudCover, 0, 100),
                   phase == 2 ? CRGB(20, 70, 150) : CRGB(18, 36, 72));
    return;
  }

  drawWeatherIcon(weather.weatherCode, 0, 0, iconColor);
  snprintf(text, sizeof(text), "%dC", static_cast<int>(lroundf(weather.temperature)));
  drawText5x7(text, strlen(text) >= 4 ? 8 : 11, 0, state.control.preferredColor);
  drawWeatherBar(constrain(weather.cloudCover, 0, 100), CRGB(18, 36, 72));
}

void drawTimerValue(uint16_t minutes, uint8_t seconds, const CRGB &color) {
  if (minutes > 99) {
    minutes = 99;
  }
  drawDigit(7, 1, minutes / 10, color);
  drawDigit(11, 1, minutes % 10, color);
  drawColon(15, 1, color);
  drawDigit(17, 1, seconds / 10, color);
  drawDigit(21, 1, seconds % 10, color);
}

CRGB timerDisplayColor(const ControlState &control) {
  if (control.timerState == TimerRunState::Finished) {
    return ((millis() / 250) & 0x01) ? CRGB::Black : CRGB(180, 0, 0);
  }
  if (control.timerState == TimerRunState::Paused) {
    return CRGB(110, 90, 0);
  }
  return control.preferredColor;
}

void drawTimerProgressBar(const ControlState &control, uint32_t valueSec, const CRGB &color) {
  if (control.timerMode == TimerMode::Stopwatch || control.timerDurationSec == 0) {
    return;
  }

  const uint32_t elapsed = control.timerDurationSec > valueSec ? control.timerDurationSec - valueSec : 0;
  const uint8_t lit = map(elapsed, 0, control.timerDurationSec, 0, AppConfig::MatrixWidth);
  CRGB barColor = color;
  barColor.nscale8_video(72);
  for (uint8_t x = 0; x < AppConfig::MatrixWidth; ++x) {
    if (x < lit) {
      addUiPixel(x, AppConfig::MatrixHeight - 1, barColor);
    }
  }
}

void renderTimer(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, state.control.timerState == TimerRunState::Finished ? 96 : 72);
  const uint32_t nowMs = millis();
  const uint32_t valueSec = state.control.timerMode == TimerMode::Stopwatch
                                ? stopwatchElapsedSec(state.control, nowMs)
                                : timerRemainingSec(state.control, nowMs);
  const uint16_t minutes = valueSec / 60UL;
  const uint8_t seconds = valueSec % 60UL;
  const CRGB color = timerDisplayColor(state.control);

  if (color) {
    drawTimerValue(minutes, seconds, color);
    drawTimerProgressBar(state.control, valueSec, color);
  }

  if (state.control.timerState == TimerRunState::Idle) {
    addUiPixel(0, AppConfig::MatrixHeight - 1, CRGB(32, 32, 32));
  } else if (state.control.timerState == TimerRunState::Paused) {
    addUiPixel(0, 0, CRGB(96, 72, 0));
    addUiPixel(1, 0, CRGB(96, 72, 0));
  }
}

CRGB gameRunColor(CRGB color, GameRunState runState) {
  if (runState == GameRunState::Idle) {
    color.nscale8_video(120);
  } else if (runState == GameRunState::Paused) {
    color.nscale8_video(88);
  }
  return color;
}

void renderSnake(const GameState &game, const CRGB &accent) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);

  setGamePixel(game.food.x, game.food.y, gameRunColor(CRGB(190, 20, 18), game.runState));
  for (uint16_t i = 0; i < game.snakeLen && i < SnakeMaxLen; ++i) {
    CRGB color = accent;
    color.nscale8_video(i == 0 ? 255 : 168);
    color = gameRunColor(color, game.runState);
    setGamePixel(game.snake[i].x, game.snake[i].y, color);
  }

  if (game.runState == GameRunState::Idle) {
    CRGB idleColor = accent;
    idleColor.nscale8_video(80);
    setGamePixel(0, 0, idleColor);
  } else if (game.runState == GameRunState::Paused) {
    setGamePixel(0, 0, CRGB(160, 110, 0));
    setGamePixel(1, 0, CRGB(160, 110, 0));
  }
}

void renderGravityBall(const GameState &game, const CRGB &accent) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);
  setGamePixel(game.target.x, game.target.y, gameRunColor(CRGB(190, 24, 18), game.runState));
  setGamePixel(game.ball.x, game.ball.y, gameRunColor(accent, game.runState));

  const uint8_t scoreWidth = static_cast<uint8_t>(constrain(static_cast<int>(game.score), 0, GameBoardW));
  CRGB scoreColor = accent;
  scoreColor.nscale8_video(96);
  for (uint8_t x = 0; x < scoreWidth; ++x) {
    addGamePixel(x, GameBoardH - 1, scoreColor);
  }
}

void renderBreakout(const GameState &game, const CRGB &accent) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);

  for (uint8_t row = 0; row < BreakoutBrickRows; ++row) {
    for (uint8_t col = 0; col < BreakoutBrickCols; ++col) {
      const uint8_t brickIndex = row * BreakoutBrickCols + col;
      if ((game.breakoutBricks & (1UL << brickIndex)) == 0) {
        continue;
      }
      CRGB color = row == 0 ? CRGB(210, 42, 26) : CRGB(220, 134, 28);
      if ((col & 0x01) != 0) {
        color += row == 0 ? CRGB(0, 26, 50) : CRGB(0, 36, 20);
      }
      color = gameRunColor(color, game.runState);
      const uint8_t x0 = col * BreakoutBrickWidth;
      setGamePixel(x0, row, color);
      setGamePixel(x0 + 1, row, color);
    }
  }

  const CRGB paddle = gameRunColor(accent, game.runState);
  for (uint8_t dx = 0; dx < BreakoutPaddleWidth; ++dx) {
    setGamePixel(game.breakoutPaddleX + dx, GameBoardH - 1, paddle);
  }
  CRGB ball = accent;
  ball.nscale8_video(220);
  setGamePixel(game.ball.x, game.ball.y, gameRunColor(ball, game.runState));
}

void renderGameOver(const GameState &game) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);
  if (((millis() / 240) & 0x01) != 0) {
    return;
  }

  for (uint8_t x = 0; x < GameBoardW; ++x) {
    setGamePixel(x, 0, CRGB(180, 0, 0));
    setGamePixel(x, GameBoardH - 1, CRGB(180, 0, 0));
  }
  for (uint8_t y = 1; y < GameBoardH - 1; ++y) {
    setGamePixel(0, y, CRGB(120, 0, 0));
    setGamePixel(GameBoardW - 1, y, CRGB(120, 0, 0));
  }

  const uint8_t scoreWidth = static_cast<uint8_t>(constrain(static_cast<int>(game.score), 0, GameBoardW - 2));
  for (uint8_t x = 1; x <= scoreWidth; ++x) {
    setGamePixel(x, GameBoardH / 2, CRGB(170, 80, 0));
  }
}

void renderUnsupportedGame(const GameState &game, const CRGB &accent) {
  fill_solid(leds, AppConfig::LedCount, CRGB::Black);
  const CRGB color = gameRunColor(accent, game.runState);
  for (uint8_t x = 8; x < 24; ++x) {
    setGamePixel(x, 3, color);
    if ((x & 0x03) == 0) {
      setGamePixel(x, 4, color);
    }
  }
}

void renderGame(const RenderState &state) {
  GameState game = copyGameState();
  gameUpdate(game, state.control, state.environment);
  updateGameState(game);

  if (game.runState == GameRunState::GameOver) {
    renderGameOver(game);
    return;
  }

  switch (game.type) {
    case GameType::Breakout:
      renderBreakout(game, state.control.preferredColor);
      break;
    case GameType::GravityBall:
      renderGravityBall(game, state.control.preferredColor);
      break;
    case GameType::Snake:
      renderSnake(game, state.control.preferredColor);
      break;
    case GameType::Reaction:
    case GameType::Pong:
    default:
      renderUnsupportedGame(game, state.control.preferredColor);
      break;
  }
}

float audioDisplayGain(uint8_t sensitivity) {
  return 0.55f + sensitivity / 96.0f;
}

uint8_t mapAudioToHeight(float level, uint8_t sensitivity, uint8_t minWhenActive = 0) {
  const float scaled = constrain(level * audioDisplayGain(sensitivity), 0.0f, 1.2f);
  int height = lroundf(scaled * AppConfig::MatrixHeight);
  if (level > 0.006f && height < minWhenActive) {
    height = minWhenActive;
  }
  return static_cast<uint8_t>(constrain(height, 0, static_cast<int>(AppConfig::MatrixHeight)));
}

uint8_t mapAudioToWidth(float level, uint8_t sensitivity) {
  const float scaled = constrain(level * audioDisplayGain(sensitivity), 0.0f, 1.2f);
  return static_cast<uint8_t>(constrain(lroundf(scaled * AppConfig::MatrixWidth), 0, static_cast<int>(AppConfig::MatrixWidth)));
}

CRGB audioVisualColor(const RenderState &state, uint8_t band, uint8_t y, uint8_t value) {
  if (state.control.audioRainbow) {
    return CHSV(static_cast<uint8_t>(millis() / 32 + band * 7 + y * 5), 220, value);
  }

  CRGB color = state.control.preferredColor;
  color.nscale8_video(value);
  return color;
}

void renderSpectrumNormal(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 86);
  for (uint8_t band = 0; band < AppConfig::SpectrumBins && band < AppConfig::MatrixWidth; ++band) {
    const float floorPulse = constrain(state.audio.rms * 0.14f, 0.0f, 0.12f);
    const float level = constrain(state.audio.smoothSpectrum[band] + floorPulse, 0.0f, 1.0f);
    const uint8_t height = mapAudioToHeight(level, state.control.audioSensitivity, 1);

    for (uint8_t y = 0; y < height; ++y) {
      const uint8_t displayY = AppConfig::MatrixHeight - 1 - y;
      const uint8_t value = static_cast<uint8_t>(constrain(120 + y * 18, 120, 220));
      addUiPixel(band, displayY, audioVisualColor(state, band, y, value));
    }
  }
}

void renderSpectrumMirror(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 78);
  constexpr uint8_t HalfWidth = AppConfig::MatrixWidth / 2;

  for (uint8_t i = 0; i < HalfWidth; ++i) {
    const uint8_t band = constrain(i * 2, 0, static_cast<int>(AppConfig::SpectrumBins - 1));
    const float level = constrain(state.audio.smoothSpectrum[band] + state.audio.rms * 0.08f, 0.0f, 1.0f);
    const uint8_t height = mapAudioToHeight(level, state.control.audioSensitivity, 1);
    const int leftX = HalfWidth - 1 - i;
    const int rightX = HalfWidth + i;

    for (uint8_t y = 0; y < height; ++y) {
      const uint8_t displayY = AppConfig::MatrixHeight - 1 - y;
      const uint8_t value = static_cast<uint8_t>(constrain(130 + y * 16, 130, 225));
      const CRGB color = audioVisualColor(state, band, y, value);
      addUiPixel(leftX, displayY, color);
      addUiPixel(rightX, displayY, color);
    }
  }
}

void renderVuMeter(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 64);
  const float level = constrain(max(state.audio.rms * 1.8f, state.audio.energy * 2.2f), 0.0f, 1.0f);
  const uint8_t lit = mapAudioToWidth(level, state.control.audioSensitivity);

  for (uint8_t x = 0; x < lit; ++x) {
    CRGB color = state.control.preferredColor;
    color.nscale8_video(static_cast<uint8_t>(constrain(118 + x * 4, 118, 235)));

    for (uint8_t y = AppConfig::MatrixHeight - 3; y < AppConfig::MatrixHeight; ++y) {
      addUiPixel(x, y, color);
    }
  }

  const uint8_t peakX = constrain(lroundf(state.audio.peak * audioDisplayGain(state.control.audioSensitivity) * AppConfig::MatrixWidth), 0L, static_cast<long>(AppConfig::MatrixWidth - 1));
  addUiPixel(peakX, AppConfig::MatrixHeight - 4, CRGB(190, 190, 190));
}

void renderBassPulse(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 44);

  if (state.audio.beat && state.control.audioBeatFlash) {
    CRGB flash = state.control.audioRainbow ? CRGB(CHSV(millis() / 5, 220, 190)) : state.control.preferredColor;
    if (!state.control.audioRainbow) {
      flash.nscale8_video(176);
    }
    for (uint16_t i = 0; i < AppConfig::LedCount; ++i) {
      leds[i] += flash;
    }
  }

  const uint8_t height = mapAudioToHeight(state.audio.lowEnergy + state.audio.rms * 0.12f, state.control.audioSensitivity, 1);
  for (uint8_t x = 11; x <= 20; ++x) {
    for (uint8_t y = 0; y < height; ++y) {
      addUiPixel(x, AppConfig::MatrixHeight - 1 - y, audioVisualColor(state, x, y, 205));
    }
  }

  const uint8_t midWidth = mapAudioToWidth(state.audio.midEnergy * 1.4f, state.control.audioSensitivity) / 2;
  for (uint8_t dx = 0; dx < midWidth && dx < 12; ++dx) {
    const CRGB color = audioVisualColor(state, dx, 0, 150);
    addUiPixel(15 - dx, AppConfig::MatrixHeight - 1, color);
    addUiPixel(16 + dx, AppConfig::MatrixHeight - 1, color);
  }
}

void renderFireSpectrum(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 62);
  for (uint8_t x = 0; x < AppConfig::MatrixWidth; ++x) {
    const uint8_t band = x < AppConfig::SpectrumBins ? x : AppConfig::SpectrumBins - 1;
    const uint8_t height = mapAudioToHeight(state.audio.smoothSpectrum[band] + state.audio.lowEnergy * 0.22f,
                                            state.control.audioSensitivity,
                                            1);

    for (uint8_t y = 0; y < height; ++y) {
      CRGB color;
      if (!state.control.audioRainbow) {
        color = audioVisualColor(state, x, y, static_cast<uint8_t>(constrain(130 + y * 15, 130, 220)));
      } else if (y < 2) {
        color = CRGB(150, 8, 0);
      } else if (y < 5) {
        color = CRGB(180, 80, 0);
      } else {
        color = CRGB(170, 155, 42);
      }
      addUiPixel(x, AppConfig::MatrixHeight - 1 - y, color);
    }
  }
}

void renderCenterBurst(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 52);
  const uint8_t radius = constrain(mapAudioToWidth(state.audio.energy * 2.0f + state.audio.rms * 0.6f,
                                                   state.control.audioSensitivity) /
                                       2,
                                   0,
                                   static_cast<int>(AppConfig::MatrixWidth / 2));
  const uint8_t baseHue = millis() / 20;

  for (uint8_t dx = 0; dx < radius; ++dx) {
    const int leftX = AppConfig::MatrixWidth / 2 - 1 - dx;
    const int rightX = AppConfig::MatrixWidth / 2 + dx;
    const uint8_t value = static_cast<uint8_t>(constrain(220 - dx * 10, 60, 220));
    CRGB color = state.control.audioRainbow ? CHSV(baseHue + dx * 8, 220, value) : state.control.preferredColor;
    if (!state.control.audioRainbow) {
      color.nscale8_video(value);
    }

    for (uint8_t y = 2; y <= 5; ++y) {
      addUiPixel(leftX, y, color);
      addUiPixel(rightX, y, color);
    }
  }

  if (state.audio.beat && state.control.audioBeatFlash) {
    addUiPixel(15, 3, CRGB::White);
    addUiPixel(16, 3, CRGB::White);
    addUiPixel(15, 4, CRGB::White);
    addUiPixel(16, 4, CRGB::White);
  }
}

void renderAudioVisual(const RenderState &state) {
  switch (state.control.audioVisualMode) {
    case AudioVisualMode::MirrorSpectrum:
      renderSpectrumMirror(state);
      break;
    case AudioVisualMode::VuMeter:
      renderVuMeter(state);
      break;
    case AudioVisualMode::BassPulse:
      renderBassPulse(state);
      break;
    case AudioVisualMode::FireSpectrum:
      renderFireSpectrum(state);
      break;
    case AudioVisualMode::CenterBurst:
      renderCenterBurst(state);
      break;
    case AudioVisualMode::Spectrum:
    default:
      renderSpectrumNormal(state);
      break;
  }
}

void initParticles() {
  for (size_t i = 0; i < ParticleCount; ++i) {
    const float clusterX = AppConfig::MatrixWidth * 0.5f + (random(900) - 450) / 100.0f;
    const float clusterY = AppConfig::MatrixHeight * 0.5f + (random(420) - 210) / 100.0f;
    particles[i].x = constrain(clusterX, 0.0f, static_cast<float>(AppConfig::MatrixWidth - 1));
    particles[i].y = constrain(clusterY, 0.0f, static_cast<float>(AppConfig::MatrixHeight - 1));
    particles[i].vx = 0.0f;
    particles[i].vy = 0.0f;
    particles[i].color = CHSV(static_cast<uint8_t>(132 + i * 24 / ParticleCount), 210, 190);
  }
}

uint16_t fluidIndex(int x, int y) {
  x = constrain(x, 0, static_cast<int>(FluidGridW - 1));
  y = constrain(y, 0, static_cast<int>(FluidGridH - 1));
  return static_cast<uint16_t>(x * FluidGridH + y);
}

void splatFluid(float x, float y, const CRGB &color, uint8_t coreBrightness, uint8_t edgeBrightness) {
  const int cx = lroundf(x);
  const int cy = lroundf(y);

  CRGB core = color;
  core.nscale8_video(coreBrightness);
  addFluidPixel(cx, cy, core);

  CRGB edge = color;
  edge.nscale8_video(edgeBrightness);
  addFluidPixel(cx - 1, cy, edge);
  addFluidPixel(cx + 1, cy, edge);
  addFluidPixel(cx, cy - 1, edge);
  addFluidPixel(cx, cy + 1, edge);

  CRGB corner = color;
  corner.nscale8_video(edgeBrightness / 2);
  addFluidPixel(cx - 1, cy - 1, corner);
  addFluidPixel(cx + 1, cy - 1, corner);
  addFluidPixel(cx - 1, cy + 1, corner);
  addFluidPixel(cx + 1, cy + 1, corner);
}

void clearFluidGrid() {
  for (size_t i = 0; i < FluidCellCount; ++i) {
    fluidPrevU[i] = fluidU[i];
    fluidPrevV[i] = fluidV[i];
    fluidU[i] = 0.0f;
    fluidV[i] = 0.0f;
    fluidWeight[i] = 0.0f;
    fluidDensity[i] = 0.0f;
    fluidPressure[i] = 0.0f;
    fluidCell[i] = false;
  }
}

void addWeightedGridValue(float *field, float x, float y, float value) {
  const int x0 = floorf(x);
  const int y0 = floorf(y);
  const float tx = x - x0;
  const float ty = y - y0;
  const float sx = 1.0f - tx;
  const float sy = 1.0f - ty;

  field[fluidIndex(x0, y0)] += value * sx * sy;
  field[fluidIndex(x0 + 1, y0)] += value * tx * sy;
  field[fluidIndex(x0 + 1, y0 + 1)] += value * tx * ty;
  field[fluidIndex(x0, y0 + 1)] += value * sx * ty;
}

float sampleGridValue(const float *field, float x, float y) {
  const int x0 = floorf(x);
  const int y0 = floorf(y);
  const float tx = x - x0;
  const float ty = y - y0;
  const float sx = 1.0f - tx;
  const float sy = 1.0f - ty;

  return field[fluidIndex(x0, y0)] * sx * sy +
         field[fluidIndex(x0 + 1, y0)] * tx * sy +
         field[fluidIndex(x0 + 1, y0 + 1)] * tx * ty +
         field[fluidIndex(x0, y0 + 1)] * sx * ty;
}

size_t activeParticleCount(const ControlState &control) {
  return constrain(static_cast<size_t>(control.fluidParticles), static_cast<size_t>(8), ParticleCount);
}

void particlesToGrid(size_t activeCount) {
  clearFluidGrid();
  for (size_t i = 0; i < activeCount; ++i) {
    const auto &p = particles[i];
    const float x = constrain(p.x, 0.0f, static_cast<float>(FluidGridW - 1.001f));
    const float y = constrain(p.y, 0.0f, static_cast<float>(FluidGridH - 1.001f));
    addWeightedGridValue(fluidU, x, y, p.vx);
    addWeightedGridValue(fluidV, x, y, p.vy);
    addWeightedGridValue(fluidWeight, x, y, 1.0f);
    addWeightedGridValue(fluidDensity, x, y, 1.0f);
    fluidCell[fluidIndex(lroundf(x), lroundf(y))] = true;
  }

  for (size_t i = 0; i < FluidCellCount; ++i) {
    if (fluidWeight[i] > 0.001f) {
      fluidU[i] /= fluidWeight[i];
      fluidV[i] /= fluidWeight[i];
    }
  }
}

void solveFluidPressure(size_t activeCount, bool compensateDrift) {
  constexpr uint8_t PressureIters = 8;
  constexpr float OverRelaxation = 1.65f;
  const float restDensity = static_cast<float>(activeCount) / 64.0f;

  for (uint8_t iter = 0; iter < PressureIters; ++iter) {
    for (uint8_t x = 1; x < FluidGridW - 1; ++x) {
      for (uint8_t y = 1; y < FluidGridH - 1; ++y) {
        const uint16_t c = fluidIndex(x, y);
        if (!fluidCell[c] && fluidDensity[c] < 0.15f) {
          continue;
        }

        const uint16_t l = fluidIndex(x - 1, y);
        const uint16_t r = fluidIndex(x + 1, y);
        const uint16_t b = fluidIndex(x, y - 1);
        const uint16_t t = fluidIndex(x, y + 1);
        float div = (fluidU[r] - fluidU[l] + fluidV[t] - fluidV[b]) * 0.5f;
        const float compression = fluidDensity[c] - restDensity;
        if (compensateDrift && compression > 0.0f) {
          div -= compression * 0.025f;
        }

        const float p = -div * 0.25f * OverRelaxation;
        fluidPressure[c] += p;
        fluidU[l] -= p;
        fluidU[r] += p;
        fluidV[b] -= p;
        fluidV[t] += p;
      }
    }
  }
}

void gridToParticles(size_t activeCount, float flipRatio) {
  flipRatio = constrain(flipRatio, 0.0f, 1.0f);
  for (size_t i = 0; i < activeCount; ++i) {
    auto &p = particles[i];
    const float x = constrain(p.x, 0.0f, static_cast<float>(FluidGridW - 1.001f));
    const float y = constrain(p.y, 0.0f, static_cast<float>(FluidGridH - 1.001f));

    const float picU = sampleGridValue(fluidU, x, y);
    const float picV = sampleGridValue(fluidV, x, y);
    const float corrU = sampleGridValue(fluidU, x, y) - sampleGridValue(fluidPrevU, x, y);
    const float corrV = sampleGridValue(fluidV, x, y) - sampleGridValue(fluidPrevV, x, y);

    p.vx = (1.0f - flipRatio) * picU + flipRatio * (p.vx + corrU);
    p.vy = (1.0f - flipRatio) * picV + flipRatio * (p.vy + corrV);
    p.vx = constrain(p.vx, -0.75f, 0.75f);
    p.vy = constrain(p.vy, -0.75f, 0.75f);
  }
}

void pushParticlesApart(size_t activeCount, uint8_t iterations) {
  constexpr float MinDist = 0.86f;
  constexpr float MinDist2 = MinDist * MinDist;
  for (uint8_t iter = 0; iter < iterations; ++iter) {
    for (size_t i = 0; i < activeCount; ++i) {
      for (size_t j = i + 1; j < activeCount; ++j) {
        float dx = particles[j].x - particles[i].x;
        float dy = particles[j].y - particles[i].y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= 0.0001f || d2 >= MinDist2) {
          continue;
        }
        const float d = sqrtf(d2);
        const float s = 0.5f * (MinDist - d) / d;
        dx *= s;
        dy *= s;
        particles[i].x -= dx;
        particles[i].y -= dy;
        particles[j].x += dx;
        particles[j].y += dy;
      }
    }
  }
}

void applyFluidBounds(size_t activeCount) {
  for (size_t i = 0; i < activeCount; ++i) {
    auto &p = particles[i];
    if (p.x < 0.35f) {
      p.x = 0.35f;
      p.vx = fabsf(p.vx) * 0.16f;
    } else if (p.x > FluidGridW - 1.35f) {
      p.x = FluidGridW - 1.35f;
      p.vx = -fabsf(p.vx) * 0.16f;
    }
    if (p.y < 0.35f) {
      p.y = 0.35f;
      p.vy = fabsf(p.vy) * 0.16f;
    } else if (p.y > FluidGridH - 1.35f) {
      p.y = FluidGridH - 1.35f;
      p.vy = -fabsf(p.vy) * 0.16f;
    }
  }
}

void renderFluid(const RenderState &state) {
  fadeToBlackBy(leds, AppConfig::LedCount, 24);
  const size_t activeCount = activeParticleCount(state.control);
  const float drift = millis() * 0.0012f;
  float gravityX = constrain(state.environment.accelX, -1.5f, 1.5f) * 0.038f + sinf(drift) * 0.004f;
  float gravityY = -constrain(state.environment.accelY, -1.5f, 1.5f) * 0.038f - cosf(drift * 0.77f) * 0.004f;
  if (FluidGravityFlipX) {
    gravityX = -gravityX;
  }
  if (FluidGravityFlipY) {
    gravityY = -gravityY;
  }
  const float beat = constrain(state.audio.rms * 3.0f + state.audio.peak * 0.7f, 0.0f, 0.55f);

  for (size_t i = 0; i < activeCount; ++i) {
    auto &p = particles[i];
    const float swirl = sinf(drift + p.x * 0.31f + p.y * 0.17f) * beat * 0.018f;
    p.vx = (p.vx + gravityX + swirl) * 0.992f;
    p.vy = (p.vy + gravityY + beat * 0.018f - swirl) * 0.992f;
    p.x += p.vx;
    p.y += p.vy;
  }

  if (state.control.fluidSeparateParticles) {
    pushParticlesApart(activeCount, 2);
  }
  applyFluidBounds(activeCount);
  particlesToGrid(activeCount);
  solveFluidPressure(activeCount, state.control.fluidCompensateDrift);
  gridToParticles(activeCount, state.control.fluidFlipRatio);

  for (size_t i = 0; i < activeCount; ++i) {
    auto &p = particles[i];
    CRGB color = state.control.preferredColor;
    const uint8_t shimmer = static_cast<uint8_t>(176 + ((i * 17 + millis() / 32) % 48));
    color.nscale8_video(static_cast<uint8_t>(constrain(shimmer + beat * 64, 176, 255)));
    splatFluid(p.x, p.y, color, AppConfig::FluidCoreBrightness, AppConfig::FluidEdgeBrightness);
  }
}

void applyBrightnessAndPower(const RenderState &state) {
  uint8_t targetBrightness = state.control.autoBrightness
                                 ? state.environment.adaptiveBrightness
                                 : state.control.manualBrightness;
  targetBrightness = min(targetBrightness, state.power.brightnessCap);
  if (state.control.smartScenes &&
      (state.context.quietHours || state.context.darkEnvironment)) {
    targetBrightness = min(targetBrightness, state.control.nightBrightnessCap);
  }

  if (currentBrightness < targetBrightness) {
    const uint8_t riseStep = state.control.autoBrightness ? 2 : 8;
    currentBrightness = static_cast<uint8_t>(
        min(static_cast<int>(targetBrightness), static_cast<int>(currentBrightness) + riseStep));
  } else if (currentBrightness > targetBrightness) {
    const uint8_t fallStep = state.control.autoBrightness ? 3 : 10;
    currentBrightness = static_cast<uint8_t>(
        max(static_cast<int>(targetBrightness), static_cast<int>(currentBrightness) - fallStep));
  }
  FastLED.setBrightness(currentBrightness);

  static uint16_t lastLimit = 0;
  if (state.power.maxMilliamps != lastLimit) {
    FastLED.setMaxPowerInVoltsAndMilliamps(5, state.power.maxMilliamps);
    lastLimit = state.power.maxMilliamps;
  }
}

uint32_t renderVariantKey(const RenderState &state, DisplayMode mode) {
  uint32_t key = static_cast<uint8_t>(mode);
  switch (mode) {
    case DisplayMode::Clock:
      key |= static_cast<uint32_t>(state.control.clockTheme) << 8;
      break;
    case DisplayMode::Spectrum:
      key |= static_cast<uint32_t>(state.control.audioVisualMode) << 8;
      break;
    case DisplayMode::Weather:
      key |= static_cast<uint32_t>(state.control.weatherDisplayMode) << 8;
      break;
    case DisplayMode::Game:
      key |= static_cast<uint32_t>(state.control.gameType) << 8;
      break;
    case DisplayMode::Timer:
      key |= static_cast<uint32_t>(state.control.timerMode) << 8;
      break;
    default:
      break;
  }
  return key;
}

void renderSelectedMode(const RenderState &state, DisplayMode mode) {
  switch (mode) {
    case DisplayMode::Spectrum:
      renderAudioVisual(state);
      break;
    case DisplayMode::Fluid:
      renderFluid(state);
      break;
    case DisplayMode::Text:
      renderScrollingText(state);
      break;
    case DisplayMode::Timer:
      renderTimer(state);
      break;
    case DisplayMode::Weather:
      renderWeather(state);
      break;
    case DisplayMode::Game:
      renderGame(state);
      break;
    case DisplayMode::Clock:
    default:
      renderClock(state);
      break;
  }
}

void renderNotificationOverlay(const NotificationState &notifications) {
  if (!notifications.activeVisible || notifications.active.text[0] == '\0') {
    return;
  }

  const NotificationItem &item = notifications.active;
  const uint32_t elapsed = millis() - notifications.activeStartedMs;
  uint8_t overlayAlpha = 255;
  if (elapsed < 180) {
    overlayAlpha = static_cast<uint8_t>(elapsed * 255UL / 180UL);
  } else if (item.durationMs > 260 && elapsed + 260 > item.durationMs) {
    const uint32_t remaining = item.durationMs > elapsed ? item.durationMs - elapsed : 0;
    overlayAlpha = static_cast<uint8_t>(remaining * 255UL / 260UL);
  }

  const uint8_t backgroundScale = static_cast<uint8_t>(
      255 - scale8(overlayAlpha, 205));
  for (size_t i = 0; i < AppConfig::LedCount; ++i) {
    leds[i].nscale8(backgroundScale);
  }

  constexpr int CharW = 6;
  constexpr int FontW = 5;
  constexpr int FontH = 7;
  const size_t textLen = boundedTextLength(item.text);
  const int textPixelW = static_cast<int>(textLen) * CharW;
  const int totalScrollW = textPixelW + AppConfig::MatrixWidth;
  const uint16_t speedMs = constrain(
      item.speedMs, static_cast<uint16_t>(30), static_cast<uint16_t>(240));
  const int offset = totalScrollW > 0
                         ? static_cast<int>((elapsed / speedMs) % totalScrollW)
                         : 0;

  for (int screenX = 0; screenX < AppConfig::MatrixWidth; ++screenX) {
    const int sourceX = offset + screenX - AppConfig::MatrixWidth;
    if (sourceX < 0 || sourceX >= textPixelW) {
      continue;
    }
    const int charIndex = sourceX / CharW;
    const int colInChar = sourceX % CharW;
    if (colInChar >= FontW) {
      continue;
    }

    const uint8_t bits = getScrollGlyph(item.text[charIndex])[colInChar];
    for (int y = 0; y < FontH; ++y) {
      if ((bits & (1 << y)) == 0) {
        continue;
      }
      CRGB color = item.color;
      color.nscale8_video(overlayAlpha);
      addUiPixel(screenX, y, color);
    }
  }
}

void composeTransition(const ControlState &control, uint32_t now) {
  const uint16_t duration = constrain(
      control.transitionDurationMs,
      static_cast<uint16_t>(120),
      static_cast<uint16_t>(1200));
  const uint32_t elapsed = now - transitionStartedMs;
  if (!control.smoothTransitions || elapsed >= duration) {
    memcpy(outputLeds, leds, sizeof(outputLeds));
    return;
  }

  const uint8_t progress = static_cast<uint8_t>(elapsed * 255UL / duration);
  for (size_t i = 0; i < AppConfig::LedCount; ++i) {
    switch (control.transitionStyle) {
      case TransitionStyle::Wipe: {
        const uint8_t x = static_cast<uint8_t>(i / AppConfig::MatrixHeight);
        const uint8_t edge = static_cast<uint8_t>(
            progress * AppConfig::MatrixWidth / 255UL);
        outputLeds[i] = x <= edge ? leds[i] : transitionFrom[i];
        break;
      }
      case TransitionStyle::PixelDissolve: {
        const uint8_t threshold = static_cast<uint8_t>(
            i * 73U + (i >> 1U) * 29U + (i % 7U) * 41U);
        outputLeds[i] = threshold <= progress ? leds[i] : transitionFrom[i];
        break;
      }
      case TransitionStyle::CrossFade:
      default:
        outputLeds[i] = blend(transitionFrom[i], leds[i], progress);
        break;
    }
  }
}

void applyColorPipeline(const RenderState &state) {
  const bool warmNight = state.control.smartScenes &&
                         (state.context.quietHours || state.context.darkEnvironment);
  for (size_t i = 0; i < AppConfig::LedCount; ++i) {
    CRGB color = outputLeds[i];
    if (warmNight) {
      color.g = scale8_video(color.g, 220);
      color.b = scale8_video(color.b, 150);
    }
    if (state.control.gammaCorrection) {
      color.r = gammaLut[color.r];
      color.g = gammaLut[color.g];
      color.b = gammaLut[color.b];
    }
    outputLeds[i] = color;
  }
}

void renderFrame(const RenderState &state) {
  applyBrightnessAndPower(state);

  const DisplayMode effectiveMode = state.control.smartScenes
                                        ? state.context.effectiveMode
                                        : state.control.mode;
  const uint32_t key = renderVariantKey(state, effectiveMode);
  if (key != activeRenderKey) {
    memcpy(transitionFrom, leds, sizeof(transitionFrom));
    fill_solid(leds, AppConfig::LedCount, CRGB::Black);
    activeRenderKey = key;
    transitionStartedMs = millis();
  }

  renderSelectedMode(state, effectiveMode);
  renderNotificationOverlay(state.notifications);
  composeTransition(state.control, millis());
  applyColorPipeline(state);

  updateScreenSnapshot(outputLeds, AppConfig::LedCount);
  FastLED.show();
}

void displayTask(void *) {
  FastLED.addLeds<WS2812B, static_cast<uint8_t>(Pinmap::WS2812_DATA), GRB>(
      outputLeds, AppConfig::LedCount);
  FastLED.clear(true);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(true);
  FastLED.setMaxRefreshRate(120);
  for (uint16_t i = 0; i < 256; ++i) {
    const float normalized = static_cast<float>(i) / 255.0f;
    gammaLut[i] = static_cast<uint8_t>(
        constrain(lroundf(powf(normalized, 1.55f) * 255.0f), 0L, 255L));
  }
  initParticles();

  RenderState state = copySharedState();
  const TickType_t frameDelay = pdMS_TO_TICKS(1000 / AppConfig::RenderFps);
  TickType_t lastWake = xTaskGetTickCount();

  while (true) {
    RenderState queued;
    while (xQueueReceive(gState.renderQueue, &queued, 0) == pdTRUE) {
      state = queued;
    }
    state.unixTime = time(nullptr);
    renderFrame(state);
    vTaskDelayUntil(&lastWake, frameDelay);
  }
}
} // namespace

void startDisplayTask() {
  xTaskCreatePinnedToCore(displayTask, "fastled_render_core1", 12288, nullptr, 4, &displayTaskHandle, 1);
}
