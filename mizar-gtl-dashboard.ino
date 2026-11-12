/*---------------------------------------------------------------
 * Mizar GTL camper water dashboard for the 800 x 480 RGB display.
 *--------------------------------------------------------------*/

#include <PCA9557.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <SPI.h>
#include <WiFi.h>

#define TFT_BL 2


extern "C" {
    int livelloSerbatoio1 = 75;
    int livelloSerbatoio2 = 60;
    int livelloAcqueGrigie = 35;
}

float tensioneBatteriaServizi = 12.8f;
float tensioneBatteriaMotore = 12.6f;
int caricaBatteriaServizi = 5;
int caricaBatteriaMotore = 4;
bool luciInterneAccese = true;
bool pompaAcquaAccesa = false;
bool luceEsternaAccesa = true;
/*---------------------------------------------------------------
 * RGB display hardware description
 * Bind the board's parallel signals and timing to LovyanGFX.
 *--------------------------------------------------------------*/

class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;

  /**
   * @brief Configure the RGB bus and its 800 by 480 panel.
   *
   * The data-pin order and synchronization timing must match the physical
   * panel so the display receives correctly aligned RGB565 frames.
   *
   * @param None.
   * @return A configured LGFX display object.
   * @note Called automatically while the global lcd object is constructed.
   */
  LGFX()
  {
    auto busConfig = _bus_instance.config();
    busConfig.panel = &_panel_instance;
    const int8_t dataPins[16] = {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45};
    memcpy(busConfig.pin_data, dataPins, sizeof(dataPins));
    busConfig.pin_henable = 41;
    busConfig.pin_vsync = 40;
    busConfig.pin_hsync = 39;
    busConfig.pin_pclk = 0;
    busConfig.freq_write = 24000000;
    busConfig.hsync_polarity = 0;
    busConfig.hsync_front_porch = 40;
    busConfig.hsync_pulse_width = 48;
    busConfig.hsync_back_porch = 40;
    busConfig.vsync_polarity = 0;
    busConfig.vsync_front_porch = 1;
    busConfig.vsync_pulse_width = 31;
    busConfig.vsync_back_porch = 13;
    busConfig.pclk_active_neg = 1;
    busConfig.de_idle_high = 0;
    busConfig.pclk_idle_high = 0;
    _bus_instance.config(busConfig);

    auto panelConfig = _panel_instance.config();
    panelConfig.memory_width = 800;
    panelConfig.memory_height = 480;
    panelConfig.panel_width = 800;
    panelConfig.panel_height = 480;
    _panel_instance.config(panelConfig);
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

// Owns the RGB bus, panel, and frame buffers used for drawing.
LGFX lcd;

// Logical dimensions shared by LVGL and the physical panel.
static constexpr uint32_t screenWidth = 800;
static constexpr uint32_t screenHeight = 480;

// Desired state of the output controlled by the two UI buttons.
int led;

// Retains a reference to the board's default SPI peripheral.
SPIClass& spi = SPI;

#include "touch.h"

/*---------------------------------------------------------------
 * LVGL display bridge
 * Present LVGL's completed frame through the LovyanGFX RGB bus.
 *--------------------------------------------------------------*/

/**
 * @brief Switch the RGB panel to the frame buffer rendered by LVGL.
 *
 * @param display LVGL display that requested the transfer.
 * @param area Updated area supplied by LVGL; full-frame mode is used here.
 * @param pixelMap Address of the frame buffer ready for presentation.
 * @return Nothing.
 * @note Called by LVGL whenever rendering for a frame is complete.
 */
void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixelMap)
{
  if (!lcd._bus_instance.presentFrameBuffer(pixelMap)) {
    Serial.println("LovyanGFX VSYNC frame switch timeout");
  }
  lv_display_flush_ready(display);
}

/*---------------------------------------------------------------
 * LVGL touch bridge
 * Convert the board touch driver's state into LVGL pointer samples.
 *--------------------------------------------------------------*/

/**
 * @brief Supply the current touch state and position to LVGL.
 *
 * A press includes calibrated coordinates. Paths without an active touch
 * explicitly report release so LVGL cannot retain a stale pressed state.
 *
 * @param indev LVGL input device requesting a sample.
 * @param data Destination for the pointer state and coordinates.
 * @return Nothing.
 * @note Called periodically by LVGL after registration in setup().
 */
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      Serial.print("Data x ");
      Serial.println(data->point.x);
      Serial.print("Data y ");
      Serial.println(data->point.y);
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
  delay(15);
}

// Dashboard widgets and palette. Only this UI layer changes; the board setup
// and the display/touch pipeline below remain the Elecrow implementation.
static const lv_color_t COLOR_BG = lv_color_hex(0x061629);
static const lv_color_t COLOR_HEADER = lv_color_hex(0x07182B);
static const lv_color_t COLOR_WHITE = lv_color_hex(0xF5F8FC);
static const lv_color_t COLOR_MUTED = lv_color_hex(0xAFC1D4);
static const lv_color_t COLOR_BLUE = lv_color_hex(0x55D4FF);
static const lv_color_t COLOR_BLUE_DARK = lv_color_hex(0x163F61);
static const lv_color_t COLOR_GREY = lv_color_hex(0x7C878F);
static const lv_color_t COLOR_GOLD = lv_color_hex(0xFFC94A);
static const lv_color_t COLOR_GOLD_DARK = lv_color_hex(0x6D4C2C);
static const lv_color_t COLOR_GREEN = lv_color_hex(0x35E879);
static const lv_color_t COLOR_BATTERY_GREEN = lv_color_hex(0x45F0A3);
static const lv_color_t COLOR_BATTERY_GREEN_END = lv_color_hex(0x13A96D);
static const lv_color_t COLOR_BATTERY_GREEN_BG = lv_color_hex(0x082B25);
static const lv_color_t COLOR_BATTERY_GREEN_BORDER = lv_color_hex(0x238C72);
static const lv_color_t COLOR_BATTERY_BLUE = lv_color_hex(0x65D7FF);
static const lv_color_t COLOR_BATTERY_BLUE_END = lv_color_hex(0x238AE0);
static const lv_color_t COLOR_BATTERY_BLUE_BG = lv_color_hex(0x092541);
static const lv_color_t COLOR_BATTERY_BLUE_BORDER = lv_color_hex(0x277AA8);
static const lv_color_t COLOR_SEGMENT_OFF = lv_color_hex(0x26343C);

static lv_obj_t *tankValues[3];
static lv_obj_t *tankBars[3];
static lv_obj_t *tankWaters[3];
static lv_obj_t *tankBodies[3];
static lv_obj_t *batteryValues[2];
static lv_obj_t *batterySegments[2][5];
static lv_obj_t *commandSwitchTracks[3];
static lv_obj_t *commandSwitchLabels[3];
static lv_obj_t *commandSwitchKnobs[3];
static float lastBatteryVoltages[2] = {-1.0f, -1.0f};
static int lastBatteryCharges[2] = {-1, -1};
static lv_obj_t *wifiIcon;
static lv_obj_t *wifiStateLabel;
static bool lastWifiConnected = false;
static uint32_t lastWifiCheck = 0;
static int lastTankValues[3] = {-1, -1, -1};

static lv_obj_t *makeText(lv_obj_t *parent, const char *text,
                          int x, int y, int w, int h,
                          const lv_font_t *font, lv_color_t color)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_size(label, w, h);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, color, 0);
  return label;
}

static lv_obj_t *makeShape(lv_obj_t *parent, int x, int y, int w, int h,
                           lv_color_t fill, lv_color_t border,
                           int borderWidth, int radius)
{
  lv_obj_t *shape = lv_obj_create(parent);
  lv_obj_set_pos(shape, x, y);
  lv_obj_set_size(shape, w, h);
  lv_obj_set_style_bg_color(shape, fill, 0);
  lv_obj_set_style_bg_opa(shape, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(shape, border, 0);
  lv_obj_set_style_border_width(shape, borderWidth, 0);
  lv_obj_set_style_radius(shape, radius, 0);
  lv_obj_set_style_pad_all(shape, 0, 0);
  lv_obj_set_scrollbar_mode(shape, LV_SCROLLBAR_MODE_OFF);
  return shape;
}

static int quantizeLevel(int value)
{
  value = constrain(value, 0, 100);
  return ((value + 12) / 25) * 25;
}

static void createCamperIcon(lv_obj_t *parent)
{
  // Simple vector camper silhouette built from native LVGL shapes.
  lv_obj_t *body = makeShape(parent, 29, 22, 70, 42, COLOR_HEADER, COLOR_WHITE, 3, 8);
  (void)body;
  makeShape(parent, 91, 33, 27, 31, COLOR_HEADER, COLOR_WHITE, 3, 8);
  makeShape(parent, 97, 38, 14, 12, COLOR_HEADER, COLOR_WHITE, 2, 3);
  makeShape(parent, 40, 29, 29, 11, COLOR_HEADER, COLOR_WHITE, 2, 2);
  makeShape(parent, 47, 17, 34, 7, COLOR_HEADER, COLOR_WHITE, 3, 4);
  makeShape(parent, 39, 57, 18, 18, COLOR_BG, COLOR_WHITE, 3, 9);
  makeShape(parent, 91, 57, 18, 18, COLOR_BG, COLOR_WHITE, 3, 9);
}

static void createTankIcon(lv_obj_t *parent, int x, int y, lv_color_t accent,
                           lv_obj_t **tankBody, lv_obj_t **tankWater)
{
  // Neck and outline of a small tank. The water fill is resized on update.
  makeShape(parent, x + 20, y, 24, 10, COLOR_BG, COLOR_WHITE, 2, 3);
  *tankBody = makeShape(parent, x, y + 8, 64, 67,
                        COLOR_BG, COLOR_WHITE, 3, 12);
  lv_obj_set_style_pad_all(*tankBody, 5, 0);
  *tankWater = lv_obj_create(*tankBody);
  lv_obj_set_style_bg_color(*tankWater, accent, 0);
  lv_obj_set_style_bg_opa(*tankWater, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(*tankWater, 0, 0);
  lv_obj_set_style_radius(*tankWater, 7, 0);
  lv_obj_set_scrollbar_mode(*tankWater, LV_SCROLLBAR_MODE_OFF);
}

static void updateTankCard(int index, int rawValue, lv_color_t accent)
{
  int value = constrain(rawValue, 0, 100);
  int stepValue = quantizeLevel(value);
  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d%%", value);
  lv_label_set_text(tankValues[index], valueText);
  lv_bar_set_value(tankBars[index], stepValue, LV_ANIM_OFF);

  // Four discrete quarters (0, 25, 50, 75, 100) in the tank icon.
  const int insideHeight = 51;
  int fillHeight = insideHeight * stepValue / 100;
  lv_obj_set_size(tankWaters[index], 48, fillHeight);
  lv_obj_set_pos(tankWaters[index], 0, insideHeight - fillHeight);
  lv_obj_set_style_bg_color(tankWaters[index], accent, 0);
}

static void createTankCard(lv_obj_t *screen, int index, int x,
                           const char *title, const char *bottomLabel,
                           const char *iconSymbol,
                           lv_color_t accent, lv_color_t accentEnd,
                           lv_color_t cardFill, lv_color_t cardBorder,
                           int initialValue)
{
  const int cardWidth = 250;
  const int cardY = 90;
  const int cardHeight = 165;
  lv_obj_t *card = makeShape(screen, x, cardY, cardWidth, cardHeight,
                             cardFill, cardBorder, 2, 18);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_set_style_shadow_color(card, cardBorder, 0);
  lv_obj_set_style_shadow_width(card, 10, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);

  lv_obj_t *titleLabel = makeText(card, title, 10, 4, cardWidth - 20, 27,
                                  &lv_font_montserrat_20, COLOR_WHITE);
  lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *icon = makeText(card, iconSymbol, 18, 36, 48, 48,
                            &lv_font_montserrat_40, accent);
  lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, 0);

  tankValues[index] = makeText(card, "0%", 72, 38, 110, 52,
                               &lv_font_montserrat_40, accent);
  createTankIcon(card, 174, 34, accent,
                 &tankBodies[index], &tankWaters[index]);

  tankBars[index] = lv_bar_create(card);
  lv_obj_set_pos(tankBars[index], 18, 112);
  lv_obj_set_size(tankBars[index], 214, 16);
  lv_bar_set_range(tankBars[index], 0, 100);
  lv_bar_set_value(tankBars[index], 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(tankBars[index], cardBorder, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(tankBars[index], LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(tankBars[index], cardBorder, LV_PART_MAIN);
  lv_obj_set_style_border_width(tankBars[index], 1, LV_PART_MAIN);
  lv_obj_set_style_radius(tankBars[index], 9, LV_PART_MAIN);
  lv_obj_set_style_bg_color(tankBars[index], accent, LV_PART_INDICATOR);
  lv_obj_set_style_bg_grad_color(tankBars[index], accentEnd, LV_PART_INDICATOR);
  lv_obj_set_style_bg_grad_dir(tankBars[index], LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(tankBars[index], LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(tankBars[index], 8, LV_PART_INDICATOR);

  makeText(card, bottomLabel, 18, 133, 214, 22,
           &lv_font_montserrat_14, COLOR_MUTED);
  updateTankCard(index, initialValue, accent);
}

static void updateBatteryCard(int index, float voltage, int charge,
                              lv_color_t accent, lv_color_t accentEnd)
{
  if (charge < 0) charge = 0;
  if (charge > 5) charge = 5;
  char voltageText[16];
  snprintf(voltageText, sizeof(voltageText), "%.1f V", voltage);
  lv_label_set_text(batteryValues[index], voltageText);
  for (int i = 0; i < 5; ++i) {
    lv_obj_set_style_bg_color(batterySegments[index][i],
                              i < charge ? accent : COLOR_SEGMENT_OFF, 0);
    lv_obj_set_style_bg_grad_color(batterySegments[index][i], accentEnd, 0);
    lv_obj_set_style_bg_grad_dir(batterySegments[index][i],
                                 LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(batterySegments[index][i],
                            i < charge ? LV_OPA_COVER : LV_OPA_50, 0);
  }
}

static void createBatteryIcon(lv_obj_t *card, int x, lv_color_t accent,
                              lv_color_t cardFill, bool motorBattery)
{
  lv_obj_t *battery = makeText(card, LV_SYMBOL_BATTERY_FULL,
                               x, 34, 60, 35,
                               &lv_font_montserrat_28, accent);
  lv_obj_set_style_text_align(battery, LV_TEXT_ALIGN_CENTER, 0);

  if (motorBattery) {
    // Small car silhouette over the lower edge of the battery symbol.
    makeShape(card, x + 11, 59, 46, 9, accent, accent, 1, 4);
    makeShape(card, x + 20, 52, 27, 10, accent, accent, 1, 4);
    makeShape(card, x + 15, 64, 10, 10, cardFill, accent, 2, 5);
    makeShape(card, x + 43, 64, 10, 10, cardFill, accent, 2, 5);
  } else {
    lv_obj_t *bolt = makeText(card, LV_SYMBOL_CHARGE,
                              x + 23, 39, 24, 26,
                              &lv_font_montserrat_20, accent);
    lv_obj_set_style_text_align(bolt, LV_TEXT_ALIGN_CENTER, 0);
  }
}

static void createBatteryCard(lv_obj_t *screen, int index, int x,
                              const char *title, float voltage, int charge,
                              lv_color_t accent, lv_color_t accentEnd,
                              lv_color_t cardFill, lv_color_t cardBorder,
                              bool motorBattery)
{
  lv_obj_t *card = makeShape(screen, x, 264, 388, 104,
                             cardFill, cardBorder, 2, 18);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_set_style_shadow_color(card, cardBorder, 0);
  lv_obj_set_style_shadow_width(card, 10, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);

  makeText(card, title, 18, 5, 350, 25,
           &lv_font_montserrat_20, COLOR_WHITE);
  createBatteryIcon(card, 18, accent, cardFill, motorBattery);
  batteryValues[index] = makeText(card, "0.0 V", 82, 43, 190, 42,
                                  &lv_font_montserrat_28, accent);

  // Independent five-segment charge indicator at the right of each card.
  makeShape(card, 286, 36, 76, 42, cardFill, accent, 2, 7);
  makeShape(card, 362, 48, 8, 18, accent, accent, 0, 3);
  for (int i = 0; i < 5; ++i) {
    batterySegments[index][i] = makeShape(card, 292 + i * 13, 42, 10, 28,
                                          COLOR_SEGMENT_OFF, accent, 0, 3);
  }

  updateBatteryCard(index, voltage, charge, accent, accentEnd);
}

static void updateCommandSwitch(int index, bool isOn)
{
  lv_obj_t *track = commandSwitchTracks[index];
  lv_obj_set_style_bg_color(track, isOn ? COLOR_BATTERY_GREEN : COLOR_GREY, 0);
  lv_obj_set_style_border_color(track, isOn ? COLOR_BATTERY_GREEN : COLOR_GREY, 0);
  lv_label_set_text(commandSwitchLabels[index], isOn ? "ON" : "OFF");
  lv_obj_set_pos(commandSwitchLabels[index], isOn ? 7 : 43, 11);
  lv_obj_set_pos(commandSwitchKnobs[index], isOn ? 42 : 4, 4);
}

static void interiorLightsEvent(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  luciInterneAccese = !luciInterneAccese;
  updateCommandSwitch(0, luciInterneAccese);
}

static void waterPumpEvent(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  pompaAcquaAccesa = !pompaAcquaAccesa;
  updateCommandSwitch(1, pompaAcquaAccesa);
}

static void exteriorLightEvent(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  luceEsternaAccesa = !luceEsternaAccesa;
  updateCommandSwitch(2, luceEsternaAccesa);
}

static void createBulbIcon(lv_obj_t *card, int x, lv_color_t cardFill)
{
  const lv_color_t bulbColor = COLOR_GOLD;
  makeShape(card, x + 13, 14, 32, 32, cardFill, bulbColor, 3, 16);
  makeShape(card, x + 21, 45, 16, 5, bulbColor, bulbColor, 0, 2);
  makeShape(card, x + 23, 52, 12, 3, bulbColor, bulbColor, 0, 1);
  makeShape(card, x + 27, 3, 4, 8, bulbColor, bulbColor, 0, 2);
  makeShape(card, x + 2, 27, 8, 4, bulbColor, bulbColor, 0, 2);
  makeShape(card, x + 48, 27, 8, 4, bulbColor, bulbColor, 0, 2);
  makeShape(card, x + 8, 10, 8, 4, bulbColor, bulbColor, 0, 2);
  makeShape(card, x + 42, 10, 8, 4, bulbColor, bulbColor, 0, 2);
}

static void createPumpIcon(lv_obj_t *card, int x, lv_color_t cardFill)
{
  const lv_color_t tapColor = COLOR_BATTERY_BLUE;
  makeShape(card, x + 8, 19, 4, 26, tapColor, tapColor, 0, 2);
  makeShape(card, x + 8, 17, 33, 4, tapColor, tapColor, 0, 2);
  makeShape(card, x + 18, 10, 20, 4, tapColor, tapColor, 0, 2);
  makeShape(card, x + 26, 12, 4, 8, tapColor, tapColor, 0, 2);
  makeShape(card, x + 37, 18, 4, 16, tapColor, tapColor, 0, 2);
  makeShape(card, x + 6, 43, 22, 4, tapColor, tapColor, 0, 2);
  lv_obj_t *drop = makeText(card, LV_SYMBOL_TINT, x + 29, 35, 22, 27,
                            &lv_font_montserrat_20, tapColor);
  lv_obj_set_style_text_align(drop, LV_TEXT_ALIGN_CENTER, 0);
  (void)cardFill;
}

static void createSunIcon(lv_obj_t *card, int x, lv_color_t cardFill)
{
  const lv_color_t sunColor = COLOR_GOLD;
  makeShape(card, x + 17, 19, 28, 28, cardFill, sunColor, 3, 14);
  makeShape(card, x + 29, 4, 4, 9, sunColor, sunColor, 0, 2);
  makeShape(card, x + 29, 53, 4, 9, sunColor, sunColor, 0, 2);
  makeShape(card, x + 2, 31, 9, 4, sunColor, sunColor, 0, 2);
  makeShape(card, x + 51, 31, 9, 4, sunColor, sunColor, 0, 2);
  makeShape(card, x + 8, 10, 8, 4, sunColor, sunColor, 0, 2);
  makeShape(card, x + 46, 10, 8, 4, sunColor, sunColor, 0, 2);
  makeShape(card, x + 8, 50, 8, 4, sunColor, sunColor, 0, 2);
  makeShape(card, x + 46, 50, 8, 4, sunColor, sunColor, 0, 2);
}

static void createCommandCard(lv_obj_t *screen, int index, int x,
                              const char *title, bool initialState)
{
  lv_obj_t *card = makeShape(screen, x, 377, 252, 96,
                             lv_color_hex(0x0D2742), lv_color_hex(0x286F9B), 2, 16);
  lv_obj_set_style_pad_all(card, 0, 0);

  if (index == 0) createBulbIcon(card, 12, lv_color_hex(0x0D2742));
  else if (index == 1) createPumpIcon(card, 12, lv_color_hex(0x0D2742));
  else createSunIcon(card, 12, lv_color_hex(0x0D2742));

  lv_obj_t *titleLabel = makeText(card, title, 72, 14, 87, 62,
                                  &lv_font_montserrat_20, COLOR_WHITE);
  lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_LEFT, 0);

  commandSwitchTracks[index] = makeShape(card, 164, 28, 78, 40,
                                         COLOR_GREY, COLOR_GREY, 2, 20);
  lv_obj_add_flag(commandSwitchTracks[index], LV_OBJ_FLAG_CLICKABLE);
  commandSwitchLabels[index] = makeText(commandSwitchTracks[index],
                                        initialState ? "ON" : "OFF",
                                        initialState ? 7 : 43, 11, 31, 18,
                                        &lv_font_montserrat_14, COLOR_WHITE);
  lv_obj_add_flag(commandSwitchLabels[index], LV_OBJ_FLAG_EVENT_BUBBLE);
  commandSwitchKnobs[index] = makeShape(commandSwitchTracks[index],
                                        initialState ? 42 : 4, 4, 32, 32,
                                        COLOR_WHITE, COLOR_WHITE, 0, 16);
  lv_obj_add_flag(commandSwitchKnobs[index], LV_OBJ_FLAG_EVENT_BUBBLE);
  updateCommandSwitch(index, initialState);

  if (index == 0)
    lv_obj_add_event_cb(commandSwitchTracks[index], interiorLightsEvent,
                        LV_EVENT_CLICKED, NULL);
  else if (index == 1)
    lv_obj_add_event_cb(commandSwitchTracks[index], waterPumpEvent,
                        LV_EVENT_CLICKED, NULL);
  else
    lv_obj_add_event_cb(commandSwitchTracks[index], exteriorLightEvent,
                        LV_EVENT_CLICKED, NULL);
}

static void updateWifiIndicator(bool connected)
{
  lastWifiConnected = connected;
  lv_obj_set_style_text_color(wifiIcon, connected ? COLOR_GREEN : COLOR_GREY, 0);
  lv_label_set_text(wifiStateLabel, connected ? "Connesso" : "Disconnesso");
  lv_obj_set_style_text_color(wifiStateLabel, connected ? COLOR_GREEN : COLOR_GREY, 0);
}

static void createDashboard()
{
  lv_obj_t *screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(screen, 0, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

  // Deep-blue header, matching the reference while keeping all type crisp.
  lv_obj_t *header = makeShape(screen, 0, 0, 800, 83, COLOR_HEADER, COLOR_HEADER, 0, 0);
  createCamperIcon(header);
  makeShape(header, 132, 8, 2, 72, COLOR_BLUE, COLOR_BLUE, 0, 0);
  makeText(header, "MIZAR GTL", 154, 0, 318, 48,
           &lv_font_montserrat_40, COLOR_WHITE);
  makeText(header, "CAMPER CONTROL CENTER", 157, 48, 320, 26,
           &lv_font_montserrat_20, COLOR_MUTED);

  wifiIcon = makeText(header, LV_SYMBOL_WIFI, 481, 18, 51, 53,
                      &lv_font_montserrat_40, COLOR_GREY);
  makeText(header, "Wi-Fi", 535, 10, 101, 32,
           &lv_font_montserrat_24, COLOR_WHITE);
  wifiStateLabel = makeText(header, "Disconnesso", 535, 43, 105, 26,
                            &lv_font_montserrat_14, COLOR_GREY);
  makeShape(header, 644, 8, 2, 72, COLOR_BLUE, COLOR_BLUE, 0, 0);
  makeText(header, "10:24", 655, 5, 92, 38,
           &lv_font_montserrat_32, COLOR_WHITE);
  makeText(header, "Sab 25 Gen", 654, 47, 94, 25,
           &lv_font_montserrat_14, COLOR_MUTED);
  lv_obj_t *settings = lv_button_create(header);
  lv_obj_set_pos(settings, 754, 8);
  lv_obj_set_size(settings, 42, 72);
  lv_obj_set_style_bg_color(settings, lv_color_hex(0x173A5C), 0);
  lv_obj_set_style_bg_opa(settings, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(settings, 0, 0);
  lv_obj_set_style_radius(settings, 12, 0);
  lv_obj_t *gear = lv_label_create(settings);
  lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_font(gear, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(gear, COLOR_WHITE, 0);
  lv_obj_center(gear);

  lv_obj_t *headerRule = makeShape(screen, 0, 81, 800, 2,
                                   COLOR_BLUE, COLOR_BLUE, 0, 0);
  lv_obj_set_style_bg_grad_color(headerRule, lv_color_hex(0x1679C5), 0);
  lv_obj_set_style_bg_grad_dir(headerRule, LV_GRAD_DIR_HOR, 0);

  createTankCard(screen, 0, 8, "Acqua potabile 1", "Serbatoio 1",
                 LV_SYMBOL_TINT, COLOR_BLUE, lv_color_hex(0x168CFF),
                 lv_color_hex(0x0D2742), lv_color_hex(0x286F9B), livelloSerbatoio1);
  createTankCard(screen, 1, 272, "Acqua potabile 2", "Serbatoio 2",
                 LV_SYMBOL_TINT, COLOR_BLUE, lv_color_hex(0x168CFF),
                 lv_color_hex(0x0D2742), lv_color_hex(0x286F9B), livelloSerbatoio2);
  createTankCard(screen, 2, 536, "Acque grigie", "Serbatoio recupero",
                 LV_SYMBOL_WIFI, COLOR_GOLD, lv_color_hex(0xE9851C),
                 lv_color_hex(0x302317), COLOR_GOLD_DARK, livelloAcqueGrigie);

  createBatteryCard(screen, 0, 8, "Batteria servizi",
                    tensioneBatteriaServizi, caricaBatteriaServizi,
                    COLOR_BATTERY_GREEN, COLOR_BATTERY_GREEN_END,
                    COLOR_BATTERY_GREEN_BG, COLOR_BATTERY_GREEN_BORDER, false);
  createBatteryCard(screen, 1, 404, "Batteria motore",
                    tensioneBatteriaMotore, caricaBatteriaMotore,
                    COLOR_BATTERY_BLUE, COLOR_BATTERY_BLUE_END,
                    COLOR_BATTERY_BLUE_BG, COLOR_BATTERY_BLUE_BORDER, true);

  createCommandCard(screen, 0, 8, "Luci\ninterne", luciInterneAccese);
  createCommandCard(screen, 1, 274, "Pompa\nacqua", pompaAcquaAccesa);
  createCommandCard(screen, 2, 540, "Luce\nesterna", luceEsternaAccesa);

  updateWifiIndicator(WiFi.status() == WL_CONNECTED);
  lv_screen_load(screen);
}

static void updateDashboard()
{
  const int values[3] = {
    constrain(livelloSerbatoio1, 0, 100),
    constrain(livelloSerbatoio2, 0, 100),
    constrain(livelloAcqueGrigie, 0, 100)
  };
  const lv_color_t accents[3] = {COLOR_BLUE, COLOR_BLUE, COLOR_GOLD};
  for (int i = 0; i < 3; ++i) {
    if (values[i] != lastTankValues[i]) {
      updateTankCard(i, values[i], accents[i]);
      lastTankValues[i] = values[i];
    }
  }

  const float voltages[2] = {
    tensioneBatteriaServizi,
    tensioneBatteriaMotore
  };
  const int charges[2] = {
    caricaBatteriaServizi,
    caricaBatteriaMotore
  };
  const lv_color_t batteryAccents[2] = {
    COLOR_BATTERY_GREEN, COLOR_BATTERY_BLUE
  };
  const lv_color_t batteryAccentEnds[2] = {
    COLOR_BATTERY_GREEN_END, COLOR_BATTERY_BLUE_END
  };
  for (int i = 0; i < 2; ++i) {
    if (voltages[i] != lastBatteryVoltages[i] || charges[i] != lastBatteryCharges[i]) {
      updateBatteryCard(i, voltages[i], charges[i],
                        batteryAccents[i], batteryAccentEnds[i]);
      lastBatteryVoltages[i] = voltages[i];
      lastBatteryCharges[i] = charges[i];
    }
  }

  uint32_t now = millis();
  if (now - lastWifiCheck >= 1000) {
    lastWifiCheck = now;
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected != lastWifiConnected) updateWifiIndicator(connected);
  }
}

// Controls board-level reset and support signals through the I2C expander.
PCA9557 Out;

/*---------------------------------------------------------------
 * Initialize the dashboard
 * Bring up hardware in dependency order before loading the generated UI.
 *--------------------------------------------------------------*/

/**
 * @brief Initialize the display, touch input, LVGL, and dashboard.
 *
 * The RGB frame buffers owned by LovyanGFX are registered directly with
 * LVGL in full-render mode, avoiding an additional full-screen copy.
 *
 * @param None.
 * @return Nothing.
 * @note Called once by the Arduino framework after reset.
 */
void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("LVGL_Arduino7.0 starting...");
  Serial.printf("PSRAM: %s, size: %u, free: %u\n",
                psramFound() ? "OK" : "NOT FOUND",
                ESP.getPsramSize(), ESP.getFreePsram());

  Wire.begin(19, 20);

  // The panel controller must leave reset before the RGB bus starts.
  Out.reset();
  Out.setMode(IO_OUTPUT);
  Out.setState(IO0, IO_LOW);
  Out.setState(IO1, IO_LOW);
  delay(20);
  Out.setState(IO0, IO_HIGH);
  delay(100);
  Out.setMode(IO1, IO_INPUT);
  Serial.println("PCA9557 init done");

  if (!lcd.begin()) {
    Serial.println("lcd.begin() failed!");
    Serial.println("Check Arduino Tools > PSRAM is set to OPI PSRAM.");
    return;
  } else {
    Serial.println("LovyanGFX lcd.begin() OK");
  }
  delay(200);

  lv_init();
  lv_tick_set_cb(millis);
  delay(100);

  touch_init();
  Serial.println("touch init done");

  Serial.printf("screen: %u x %u\n", screenWidth, screenHeight);

  // LovyanGFX owns two full-screen RGB buffers after lcd.begin() succeeds.
  lv_color_t* frameBuffer0 = (lv_color_t*)lcd._bus_instance.getFrameBuffer(0);
  lv_color_t* frameBuffer1 = (lv_color_t*)lcd._bus_instance.getFrameBuffer(1);
  Serial.printf("LovyanGFX RGB buffers: %p, %p, free PSRAM: %u\n",
                frameBuffer0, frameBuffer1, ESP.getFreePsram());

  // LVGL renders directly into the LovyanGFX buffers and asks the flush
  // callback to present the completed frame at the next VSYNC boundary.
  lv_display_t *display = lv_display_create(screenWidth, screenHeight);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, my_disp_flush);
  lv_display_set_buffers(display, frameBuffer0, frameBuffer1,
                         screenWidth * screenHeight * sizeof(lv_color_t),
                         LV_DISPLAY_RENDER_MODE_FULL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  createDashboard();
  lv_timer_handler();

  Serial.println("Setup done");
}

/*---------------------------------------------------------------
 * Run the dashboard
 * Sample slowly changing sensor data while servicing LVGL continuously.
 *--------------------------------------------------------------*/

/**
 * @brief Refresh tank levels and process LVGL input and drawing tasks.
 *
 * @param None.
 * @return Nothing.
 * @note Called repeatedly by the Arduino framework after setup().
 */
void loop()
{
  updateDashboard();
  lv_timer_handler();
  delay(10);
}
