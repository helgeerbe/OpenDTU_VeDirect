// SPDX-License-Identifier: GPL-2.0-or-later
#include <Utils.h>
#include <powermeter/json/http/Provider.h>
#include <MessageOutput.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <base64.h>
#include <ESPmDNS.h>

namespace PowerMeters::Json::Http {

Provider::~Provider()
{
    _taskDone = false;

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = true;
    lock.unlock();

    _cv.notify_all();

    if (_taskHandle != nullptr) {
        while (!_taskDone) { delay(10); }
        _taskHandle = nullptr;
    }
}

bool Provider::init()
{
    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& valueConfig = _cfg.Values[i];

        _httpGetters[i] = nullptr;

        if (i == 0 || (_cfg.IndividualRequests && valueConfig.Enabled)) {
            _httpGetters[i] = std::make_unique<HttpGetter>(valueConfig.HttpRequest);
        }

        if (!_httpGetters[i]) { continue; }

        if (_httpGetters[i]->init()) {
            _httpGetters[i]->addHeader("Content-Type", "application/json");
            _httpGetters[i]->addHeader("Accept", "application/json");
            continue;
        }

        MessageOutput.printf("[PowerMeters::Json::Http] Initializing HTTP getter for value %d failed:\r\n", i + 1);
        MessageOutput.printf("[PowerMeters::Json::Http] %s\r\n", _httpGetters[i]->getErrorText());
        return false;
    }

    return true;
}

void Provider::loop()
{
    if (_taskHandle != nullptr) { return; }

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = false;
    lock.unlock();

    uint32_t constexpr stackSize = 3072;
    xTaskCreate(Provider::pollingLoopHelper, "PM:HTTP+JSON",
            stackSize, this, 1/*prio*/, &_taskHandle);
}

void Provider::pollingLoopHelper(void* context)
{
    auto pInstance = static_cast<Provider*>(context);
    pInstance->pollingLoop();
    pInstance->_taskDone = true;
    vTaskDelete(nullptr);
}

void Provider::pollingLoop()
{
    std::unique_lock<std::mutex> lock(_pollingMutex);

    while (!_stopPolling) {
        auto elapsedMillis = millis() - _lastPoll;
        auto intervalMillis = _cfg.PollingInterval * 1000;
        if (_lastPoll > 0 && elapsedMillis < intervalMillis) {
            auto sleepMs = intervalMillis - elapsedMillis;
            _cv.wait_for(lock, std::chrono::milliseconds(sleepMs),
                    [this] { return _stopPolling; }); // releases the mutex
            continue;
        }

        _lastPoll = millis();

        lock.unlock(); // polling can take quite some time
        auto res = poll();
        lock.lock();

        if (std::holds_alternative<String>(res)) {
            MessageOutput.printf("[PowerMeters::Json::Http] %s\r\n", std::get<String>(res).c_str());
            continue;
        }

        MessageOutput.printf("[PowerMeters::Json::Http] New total: %.2f\r\n", getPowerTotal());
    }
}

Provider::poll_result_t Provider::poll()
{
    auto dataInFlight = DataPointContainer();
    JsonDocument jsonResponse;

    auto prefixedError = [](uint8_t idx, char const* err) -> String {
        String res("Value ");
        res.reserve(strlen(err) + 16);
        return res + String(idx + 1) + ": " + err;
    };

    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& cfg = _cfg.Values[i];

        if (!cfg.Enabled) {
            continue;
        }

        auto const& upGetter = _httpGetters[i];

        if (upGetter) {
            auto res = upGetter->performGetRequest();
            if (!res) {
                return prefixedError(i, upGetter->getErrorText());
            }

            auto pStream = res.getStream();
            if (!pStream) {
                return prefixedError(i, "Programmer error: HTTP request yields no stream");
            }

            const DeserializationError error = deserializeJson(jsonResponse, *pStream);
            if (error) {
                String msg("Unable to parse server response as JSON: ");
                return prefixedError(i, String(msg + error.c_str()).c_str());
            }
        }

        auto pathResolutionResult = Utils::getJsonValueByPath<float>(jsonResponse, cfg.JsonPath);
        if (!pathResolutionResult.second.isEmpty()) {
            return prefixedError(i, pathResolutionResult.second.c_str());
        }

        // this value is supposed to be in Watts and positive if energy is consumed
        float newValue = pathResolutionResult.first;

        switch (cfg.PowerUnit) {
            case Unit_t::MilliWatts:
                newValue /= 1000;
                break;
            case Unit_t::KiloWatts:
                newValue *= 1000;
                break;
            default:
                break;
        }

        if (cfg.SignInverted) { newValue *= -1; }

        switch (i)
        {
        case 0:
            dataInFlight.add<DataPointLabel::PowerL1>(newValue);
            break;

        case 1:
            dataInFlight.add<DataPointLabel::PowerL2>(newValue);
            break;

        case 2:
            dataInFlight.add<DataPointLabel::PowerL3>(newValue);
            break;

        default:
            break;
        }
    }

    _dataCurrent.updateFrom(dataInFlight);
    return dataInFlight;
}

bool Provider::isDataValid() const
{
    uint32_t age = millis() - getLastUpdate();
    return getLastUpdate() > 0 && (age < (3 * _cfg.PollingInterval * 1000));
}

} // namespace PowerMeters::Json::Http
