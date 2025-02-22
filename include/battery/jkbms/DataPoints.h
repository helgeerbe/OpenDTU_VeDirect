#pragma once

#include <Arduino.h>
#include <map>
#include <frozen/map.h>
#include <frozen/string.h>
#include <DataPoints.h>

namespace Batteries::JkBms {

#define ALARM_BITS(fnc) \
    fnc(LowCapacity, (1<<0)) \
    fnc(BmsOvertemperature, (1<<1)) \
    fnc(ChargingOvervoltage, (1<<2)) \
    fnc(DischargeUndervoltage, (1<<3)) \
    fnc(BatteryOvertemperature, (1<<4)) \
    fnc(ChargingOvercurrent, (1<<5)) \
    fnc(DischargeOvercurrent, (1<<6)) \
    fnc(CellVoltageDifference, (1<<7)) \
    fnc(BatteryBoxOvertemperature, (1<<8)) \
    fnc(BatteryUndertemperature, (1<<9)) \
    fnc(CellOvervoltage, (1<<10)) \
    fnc(CellUndervoltage, (1<<11)) \
    fnc(AProtect, (1<<12)) \
    fnc(BProtect, (1<<13)) \
    fnc(Reserved1, (1<<14)) \
    fnc(Reserved2, (1<<15))

enum class AlarmBits : uint16_t {
#define ALARM_ENUM(name, value) name = value,
    ALARM_BITS(ALARM_ENUM)
#undef ALARM_ENUM
};

static const frozen::map<AlarmBits, frozen::string, 16> AlarmBitTexts = {
#define ALARM_TEXT(name, value) { AlarmBits::name, #name },
    ALARM_BITS(ALARM_TEXT)
#undef ALARM_TEXT
};

#define STATUS_BITS(fnc) \
    fnc(ChargingActive, (1<<0)) \
    fnc(DischargingActive, (1<<1)) \
    fnc(BalancingActive, (1<<2)) \
    fnc(BatteryOnline, (1<<3))

enum class StatusBits : uint16_t {
#define STATUS_ENUM(name, value) name = value,
    STATUS_BITS(STATUS_ENUM)
#undef STATUS_ENUM
};

static const frozen::map<StatusBits, frozen::string, 4> StatusBitTexts = {
#define STATUS_TEXT(name, value) { StatusBits::name, #name },
    STATUS_BITS(STATUS_TEXT)
#undef STATUS_TEXT
};

enum class DataPointLabel : uint8_t {
    CellsMilliVolt = 0x79,
    BmsTempCelsius = 0x80,
    BatteryTempOneCelsius = 0x81,
    BatteryTempTwoCelsius = 0x82,
    BatteryVoltageMilliVolt = 0x83,
    BatteryCurrentMilliAmps = 0x84,
    BatterySoCPercent = 0x85,
    BatteryTemperatureSensorAmount = 0x86,
    BatteryCycles = 0x87,
    BatteryCycleCapacity = 0x89,
    BatteryCellAmount = 0x8a,
    AlarmsBitmask = 0x8b,
    StatusBitmask = 0x8c,
    TotalOvervoltageThresholdMilliVolt = 0x8e,
    TotalUndervoltageThresholdMilliVolt = 0x8f,
    CellOvervoltageThresholdMilliVolt = 0x90,
    CellOvervoltageRecoveryMilliVolt = 0x91,
    CellOvervoltageProtectionDelaySeconds = 0x92,
    CellUndervoltageThresholdMilliVolt = 0x93,
    CellUndervoltageRecoveryMilliVolt = 0x94,
    CellUndervoltageProtectionDelaySeconds = 0x95,
    CellVoltageDiffThresholdMilliVolt = 0x96,
    DischargeOvercurrentThresholdAmperes = 0x97,
    DischargeOvercurrentDelaySeconds = 0x98,
    ChargeOvercurrentThresholdAmps = 0x99,
    ChargeOvercurrentDelaySeconds = 0x9a,
    BalanceCellVoltageThresholdMilliVolt = 0x9b,
    BalanceVoltageDiffThresholdMilliVolt = 0x9c,
    BalancingEnabled = 0x9d,
    BmsTempProtectionThresholdCelsius = 0x9e,
    BmsTempRecoveryThresholdCelsius = 0x9f,
    BatteryTempProtectionThresholdCelsius = 0xa0,
    BatteryTempRecoveryThresholdCelsius = 0xa1,
    BatteryTempDiffThresholdCelsius = 0xa2,
    ChargeHighTempThresholdCelsius = 0xa3,
    DischargeHighTempThresholdCelsius = 0xa4,
    ChargeLowTempThresholdCelsius = 0xa5,
    ChargeLowTempRecoveryCelsius = 0xa6,
    DischargeLowTempThresholdCelsius = 0xa7,
    DischargeLowTempRecoveryCelsius = 0xa8,
    CellAmountSetting = 0xa9,
    BatteryCapacitySettingAmpHours = 0xaa,
    BatteryChargeEnabled = 0xab,
    BatteryDischargeEnabled = 0xac,
    CurrentCalibrationMilliAmps = 0xad,
    BmsAddress = 0xae,
    BatteryType = 0xaf,
    SleepWaitTime = 0xb0, // what's this?
    LowCapacityAlarmThresholdPercent = 0xb1,
    ModificationPassword = 0xb2,
    DedicatedChargerSwitch = 0xb3, // what's this?
    EquipmentId = 0xb4,
    DateOfManufacturing = 0xb5,
    BmsHourMeterMinutes = 0xb6,
    BmsSoftwareVersion = 0xb7,
    CurrentCalibration = 0xb8,
    ActualBatteryCapacityAmpHours = 0xb9,
    ProductId = 0xba,
    ProtocolVersion = 0xc0
};

using tCells = tCellVoltages;

template<DataPointLabel> struct DataPointLabelTraits;

#define LABEL_TRAIT(n, t, u) template<> struct DataPointLabelTraits<DataPointLabel::n> { \
    using type = t; \
    static constexpr char const name[] = #n; \
    static constexpr char const unit[] = u; \
};

/**
 * the types associated with the labels are the types for the respective data
 * points in the JkBms::DataPoint class. they are *not* always equal to the
 * type used in the serial message.
 *
 * it is unfortunate that we have to repeat all enum values here to define the
 * traits. code generation could help here (labels are defined in a single
 * source of truth and this code is generated -- no typing errors, etc.).
 * however, the compiler will complain if an enum is misspelled or traits are
 * defined for a removed enum, so we will notice. it will also complain when a
 * trait is missing and if a data point for a label without traits is added to
 * the DataPointContainer class, because the traits must be available then.
 * even though this is tedious to maintain, human errors will be caught.
 */
LABEL_TRAIT(CellsMilliVolt,                         tCells,      "mV");
LABEL_TRAIT(BmsTempCelsius,                         int16_t,     "°C");
LABEL_TRAIT(BatteryTempOneCelsius,                  int16_t,     "°C");
LABEL_TRAIT(BatteryTempTwoCelsius,                  int16_t,     "°C");
LABEL_TRAIT(BatteryVoltageMilliVolt,                uint32_t,    "mV");
LABEL_TRAIT(BatteryCurrentMilliAmps,                int32_t,     "mA");
LABEL_TRAIT(BatterySoCPercent,                      uint8_t,     "%");
LABEL_TRAIT(BatteryTemperatureSensorAmount,         uint8_t,     "");
LABEL_TRAIT(BatteryCycles,                          uint16_t,    "");
LABEL_TRAIT(BatteryCycleCapacity,                   uint32_t,    "Ah");
LABEL_TRAIT(BatteryCellAmount,                      uint16_t,    "");
LABEL_TRAIT(AlarmsBitmask,                          uint16_t,    "");
LABEL_TRAIT(StatusBitmask,                          uint16_t,    "");
LABEL_TRAIT(TotalOvervoltageThresholdMilliVolt,     uint32_t,    "mV");
LABEL_TRAIT(TotalUndervoltageThresholdMilliVolt,    uint32_t,    "mV");
LABEL_TRAIT(CellOvervoltageThresholdMilliVolt,      uint16_t,    "mV");
LABEL_TRAIT(CellOvervoltageRecoveryMilliVolt,       uint16_t,    "mV");
LABEL_TRAIT(CellOvervoltageProtectionDelaySeconds,  uint16_t,    "s");
LABEL_TRAIT(CellUndervoltageThresholdMilliVolt,     uint16_t,    "mV");
LABEL_TRAIT(CellUndervoltageRecoveryMilliVolt,      uint16_t,    "mV");
LABEL_TRAIT(CellUndervoltageProtectionDelaySeconds, uint16_t,    "s");
LABEL_TRAIT(CellVoltageDiffThresholdMilliVolt,      uint16_t,    "mV");
LABEL_TRAIT(DischargeOvercurrentThresholdAmperes,   uint16_t,    "A");
LABEL_TRAIT(DischargeOvercurrentDelaySeconds,       uint16_t,    "s");
LABEL_TRAIT(ChargeOvercurrentThresholdAmps,         uint16_t,    "A");
LABEL_TRAIT(ChargeOvercurrentDelaySeconds,          uint16_t,    "s");
LABEL_TRAIT(BalanceCellVoltageThresholdMilliVolt,   uint16_t,    "mV");
LABEL_TRAIT(BalanceVoltageDiffThresholdMilliVolt,   uint16_t,    "mV");
LABEL_TRAIT(BalancingEnabled,                       bool,        "");
LABEL_TRAIT(BmsTempProtectionThresholdCelsius,      uint16_t,    "°C");
LABEL_TRAIT(BmsTempRecoveryThresholdCelsius,        uint16_t,    "°C");
LABEL_TRAIT(BatteryTempProtectionThresholdCelsius,  uint16_t,    "°C");
LABEL_TRAIT(BatteryTempRecoveryThresholdCelsius,    uint16_t,    "°C");
LABEL_TRAIT(BatteryTempDiffThresholdCelsius,        uint16_t,    "°C");
LABEL_TRAIT(ChargeHighTempThresholdCelsius,         uint16_t,    "°C");
LABEL_TRAIT(DischargeHighTempThresholdCelsius,      uint16_t,    "°C");
LABEL_TRAIT(ChargeLowTempThresholdCelsius,          int16_t,     "°C");
LABEL_TRAIT(ChargeLowTempRecoveryCelsius,           int16_t,     "°C");
LABEL_TRAIT(DischargeLowTempThresholdCelsius,       int16_t,     "°C");
LABEL_TRAIT(DischargeLowTempRecoveryCelsius,        int16_t,     "°C");
LABEL_TRAIT(CellAmountSetting,                      uint8_t,     "");
LABEL_TRAIT(BatteryCapacitySettingAmpHours,         uint32_t,    "Ah");
LABEL_TRAIT(BatteryChargeEnabled,                   bool,        "");
LABEL_TRAIT(BatteryDischargeEnabled,                bool,        "");
LABEL_TRAIT(CurrentCalibrationMilliAmps,            uint16_t,    "mA");
LABEL_TRAIT(BmsAddress,                             uint8_t,     "");
LABEL_TRAIT(BatteryType,                            uint8_t,     "");
LABEL_TRAIT(SleepWaitTime,                          uint16_t,    "s");
LABEL_TRAIT(LowCapacityAlarmThresholdPercent,       uint8_t,     "%");
LABEL_TRAIT(ModificationPassword,                   std::string, "");
LABEL_TRAIT(DedicatedChargerSwitch,                 bool,        "");
LABEL_TRAIT(EquipmentId,                            std::string, "");
LABEL_TRAIT(DateOfManufacturing,                    std::string, "");
LABEL_TRAIT(BmsHourMeterMinutes,                    uint32_t,    "min");
LABEL_TRAIT(BmsSoftwareVersion,                     std::string, "");
LABEL_TRAIT(CurrentCalibration,                     bool,        "");
LABEL_TRAIT(ActualBatteryCapacityAmpHours,          uint32_t,    "Ah");
LABEL_TRAIT(ProductId,                              std::string, "");
LABEL_TRAIT(ProtocolVersion,                        uint8_t,     "");
#undef LABEL_TRAIT

} // namespace Batteries::JkBms

using JkBmsDataPoint = DataPoint<bool, uint8_t, uint16_t, uint32_t,
              int16_t, int32_t, std::string, Batteries::JkBms::tCells>;

template class DataPointContainer<JkBmsDataPoint, Batteries::JkBms::DataPointLabel, Batteries::JkBms::DataPointLabelTraits>;

namespace Batteries::JkBms {
    using DataPointContainer = DataPointContainer<JkBmsDataPoint, DataPointLabel, DataPointLabelTraits>;
} // namespace Batteries::JkBms
