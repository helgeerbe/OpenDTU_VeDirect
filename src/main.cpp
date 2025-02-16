// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "Configuration.h"
#include "Datastore.h"
#include "Display_Graphic.h"
#include "I18n.h"
#include "InverterSettings.h"
#include "Led_Single.h"
#include "MessageOutput.h"
#include "SerialPortManager.h"
#include "Battery.h"
#include <gridcharger/huawei/Controller.h>
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttHandleBatteryHass.h"
#include "MqttHandleInverter.h"
#include "MqttHandleInverterTotal.h"
#include "MqttHandleHuawei.h"
#include "MqttHandlePowerLimiter.h"
#include "MqttHandlePowerLimiterHass.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "RestartHelper.h"
#include "Scheduler.h"
#include "SunPosition.h"
#include "Utils.h"
#include "WebApi.h"
#include <powermeter/Controller.h>
#include "PowerLimiter.h"
#include "defaults.h"
#include <solarcharger/Controller.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <SpiManager.h>
#include <TaskScheduler.h>
#include <esp_heap_caps.h>
#include "ShellyACPlug.h"

void setup()
{
    // Move all dynamic allocations >512byte to psram (if available)
    heap_caps_malloc_extmem_enable(512);

    // Initialize SpiManager
    SpiManagerInst.register_bus(SPI2_HOST);
#if SOC_SPI_PERIPH_NUM > 2
    SpiManagerInst.register_bus(SPI3_HOST);
#endif

    // Initialize serial output
    Serial.begin(SERIAL_BAUDRATE);
#if !ARDUINO_USB_CDC_ON_BOOT
    // Only wait for serial interface to be set up when not using CDC
    while (!Serial)
        yield();
#endif
    MessageOutput.init(scheduler);
    MessageOutput.println();
    MessageOutput.println("Starting OpenDTU");

    // Initialize file system
    MessageOutput.print("Initialize FS... ");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.print("failed... trying to format...");
        if (!LittleFS.begin(true)) {
            MessageOutput.print("success");
        } else {
            MessageOutput.print("failed");
        }
    } else {
        MessageOutput.println("done");
    }

    // Read configuration values
    Configuration.init(scheduler);
    MessageOutput.print("Reading configuration... ");
    if (!Configuration.read()) {
        if (Configuration.write()) {
            MessageOutput.print("written... ");
        } else {
            MessageOutput.print("failed... ");
        }
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        MessageOutput.print("migrated... ");
        Configuration.migrate();
    }
    if (Configuration.get().Cfg.VersionOnBattery != CONFIG_VERSION_ONBATTERY) {
        Configuration.migrateOnBattery();
        MessageOutput.print("migrated OpenDTU-OnBattery-specific config... ");
    }
    auto& config = Configuration.get();
    MessageOutput.println("done");

    // Read languate pack
    MessageOutput.print("Reading language pack... ");
    I18n.init(scheduler);
    MessageOutput.println("done");

    // Load PinMapping
    MessageOutput.print("Reading PinMapping... ");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        MessageOutput.print("found valid mapping ");
    } else {
        MessageOutput.print("using default config ");
    }
    const auto& pin = PinMapping.get();
    MessageOutput.println("done");

    SerialPortManager.init();

    // Initialize Network
    MessageOutput.print("Initialize Network... ");
    NetworkSettings.init(scheduler);
    MessageOutput.println("done");
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.print("Initialize NTP... ");
    NtpSettings.init();
    MessageOutput.println("done");

    // Initialize SunPosition
    MessageOutput.print("Initialize SunPosition... ");
    SunPosition.init(scheduler);
    MessageOutput.println("done");

    // Initialize MqTT
    MessageOutput.print("Initialize MqTT... ");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleInverter.init(scheduler);
    MqttHandleInverterTotal.init(scheduler);
    MqttHandleHass.init(scheduler);
    MqttHandleBatteryHass.init(scheduler);
    MqttHandleHuawei.init(scheduler);
    MqttHandlePowerLimiter.init(scheduler);
    MqttHandlePowerLimiterHass.init(scheduler);
    MessageOutput.println("done");

    // Initialize WebApi
    MessageOutput.print("Initialize WebApi... ");
    WebApi.init(scheduler);
    MessageOutput.println("done");

    // Initialize Display
    MessageOutput.print("Initialize Display... ");
    Display.init(
        scheduler,
        static_cast<DisplayType_t>(pin.display_type),
        pin.display_data,
        pin.display_clk,
        pin.display_cs,
        pin.display_reset);
    Display.setDiagramMode(static_cast<DiagramMode_t>(config.Display.Diagram.Mode));
    Display.setOrientation(config.Display.Rotation);
    Display.enablePowerSafe = config.Display.PowerSafe;
    Display.enableScreensaver = config.Display.ScreenSaver;
    Display.setContrast(config.Display.Contrast);
    Display.setLocale(config.Display.Locale);
    Display.setStartupDisplay();
    MessageOutput.println("done");

    // Initialize Single LEDs
    MessageOutput.print("Initialize LEDs... ");
    LedSingle.init(scheduler);
    MessageOutput.println("done");

    InverterSettings.init(scheduler);

    Datastore.init(scheduler);
    RestartHelper.init(scheduler);

    // OpenDTU-OnBattery-specific initializations go below
    SolarCharger.init(scheduler);
    PowerMeter.init(scheduler);
    PowerLimiter.init(scheduler);

    // Initialize Shelly AC-charger
    MessageOutput.println("Initialize Shelly AC charger interface... ");
    ShellyACPlug.init(scheduler);

    HuaweiCan.init(scheduler);
    Battery.init(scheduler);
}

void loop()
{
    scheduler.execute();
}
