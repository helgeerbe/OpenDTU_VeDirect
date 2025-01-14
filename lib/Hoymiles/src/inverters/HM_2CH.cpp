
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "HM_2CH.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 6, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 14, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    { TYPE_DC, CH1, FLD_UDC, UNIT_V, 8, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A, 10, 2, 100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W, 12, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_YD, UNIT_WH, 24, 2, 1, false, 0 },
    { TYPE_DC, CH1, FLD_YT, UNIT_KWH, 18, 4, 1000, false, 3 },
    { TYPE_DC, CH1, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH1, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 26, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 34, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 30, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 32, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 28, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 36, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 38, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 40, 2, 1, false, 0 },

    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

static const channelMetaData_t channelMetaData[] = {
    { CH0, MPPT_A },
    { CH1, MPPT_B }
};

HM_2CH::HM_2CH(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
}

bool HM_2CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x114100000000 && serial <= 0x1141ffffffff

    uint8_t preId[2];
    preId[0] = static_cast<uint8_t>(serial >> 40);
    preId[1] = static_cast<uint8_t>(serial >> 32);

    if (static_cast<uint8_t>((((static_cast<uint16_t>(preId[0]) << 8) | preId[1]) >> 4) & 0xff) == 0x14) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x30) || ((preId[1] & 0xf0) == 0x40))
        && (((preId[0] == 0x10) && (preId[1] == 0x42)) || ((preId[0] == 0x11) && (preId[1] == 0x41)))) {
        return true;
    }

    return false;
}

String HM_2CH::typeName() const
{
    return "HM-600/700/800-2T";
}

const byteAssign_t* HM_2CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t HM_2CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

const channelMetaData_t* HM_2CH::getChannelMetaData() const
{
    return channelMetaData;
}

uint8_t HM_2CH::getChannelMetaDataSize() const
{
    return sizeof(channelMetaData) / sizeof(channelMetaData[0]);
}
