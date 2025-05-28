#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX(void) {
    auto cfg = _bus.config();

    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 27000000;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;

    cfg.pin_sclk = 2;   // D2
    cfg.pin_mosi = 3;   // D3
    cfg.pin_miso = -1;
    cfg.pin_dc   = 7;   // D7

    _bus.config(cfg);
    _panel.setBus(&_bus);

    auto panel_cfg = _panel.config();
    panel_cfg.pin_cs   = 10;  // D10
    panel_cfg.pin_rst  = 6;   // D6
    panel_cfg.pin_busy = -1;

    panel_cfg.invert = true;
    panel_cfg.rgb_order = false;
    panel_cfg.offset_rotation = 0;
    panel_cfg.bus_shared = true;
    panel_cfg.panel_width = 240;
    panel_cfg.panel_height = 240;
    panel_cfg.memory_width = 240;
    panel_cfg.memory_height = 240;

    _panel.config(panel_cfg);
    setPanel(&_panel);
  }
};
