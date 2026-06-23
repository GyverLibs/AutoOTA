#pragma once
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <flash_hal.h>
typedef BearSSL::WiFiClientSecure OtaClient;

#elif defined(ESP32)
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
typedef WiFiClientSecure OtaClient;
#endif

#if CONFIG_IDF_TARGET_ESP32
#define GOTA_PLATFORM "ESP32"
#elif CONFIG_IDF_TARGET_ESP32S2
#define GOTA_PLATFORM "ESP32-S2"
#elif CONFIG_IDF_TARGET_ESP32S3
#define GOTA_PLATFORM "ESP32-S3"
#elif CONFIG_IDF_TARGET_ESP32C3
#define GOTA_PLATFORM "ESP32-C3"
#elif CONFIG_IDF_TARGET_ESP32C6
#define GOTA_PLATFORM "ESP32-C6"
#elif CONFIG_IDF_TARGET_ESP32H2
#define GOTA_PLATFORM "ESP32-H2"
#elif defined(ESP8266)
#define GOTA_PLATFORM "ESP8266"
#else
#error "AutoOTA: platform is not supported"
#endif

#define GOTA_TIMEOUT 2500

class AutoOTA {
   public:
    AutoOTA(const char* cur_ver, const char* url, uint16_t port = 0) : _cur_ver(cur_ver) {
        if (!strncmp(url, "http", 4)) {
            _host = url;
            if (_extractPath(_host, _path, &_port)) {
                if (port) _port = port;
            }
        } else {
            _path = url;
            if (!_path.endsWith(F(".json"))) _path += F("/main/project.json");
            _host = F("raw.githubusercontent.com");
            _port = 443;
        }
    }

    // set GitHub token for private repos
    void setAuthToken(const char* token) {
        _auth_token = token ? token : "";
    }

    // override default User-Agent if needed
    void setUserAgent(const char* agent) {
        _user_agent = agent ? agent : "AutoOTA";
    }

    enum class Error : uint8_t {
        None,
        Connect,
        Timeout,
        HTTP,
        NoVersion,
        NoPlatform,
        NoPath,
        NoUpdates,
        NoFile,
        OtaStart,
        OtaEnd,
        PathError,
        NoPort,
    };

    // текущая версия
    String version() {
        return _cur_ver;
    }

    // проверить обновления. Можно передать строки для записи информации
    bool checkUpdate(String* version = nullptr, String* notes = nullptr, String* bin = nullptr) {
        if (!_port) return _err = Error::NoPort, false;
        if (_has_update) return true;

        String ver;
        if (!version) version = &ver;

        if (!_getJson(version, notes, bin)) return false;
        if (!version->length()) return _err = Error::NoVersion, false;
        if (*version == _cur_ver) return _err = Error::NoUpdates, false;

        _has_update = true;
        _err = Error::None;
        return true;
    }

    // есть обновление. Вызывать после проверки. Само сбросится в false
    bool hasUpdate() {
        return _has_update ? _has_update = false, true : false;
    }

    // обновить прошивку из loop
    void update() {
        _ota_f = true;
    }

    // обновить прошивку сейчас и перезагрузить чип
    bool updateNow() {
        _has_update = false;

        uint16_t port = 0;
        String bin, path;
        if (!checkUpdate(nullptr, nullptr, &bin)) return false;
        if (!_extractPath(bin, path, &port)) return _err = Error::PathError, false;

        OtaClient client;
        client.setInsecure();
        if (!_request(client, bin, path, port)) return false;
        if (!_waitClient(client)) return _err = Error::NoFile, false;

#ifdef ESP8266
        size_t ota_size = (size_t)((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#else
        size_t ota_size = UPDATE_SIZE_UNKNOWN;
#endif

        if (!Update.begin(ota_size, U_FLASH)) {
            return _err = Error::OtaStart, false;
        }

        while (_waitClient(client)) Update.write(client);

        if (!Update.end(true)) {
            return _err = Error::OtaEnd, false;
        }

        ESP.restart();
        _err = Error::None;
        return true;
    }

    // тикер, вызывать в loop. Вернёт true при попытке обновления
    bool tick() {
        if (_ota_f) {
            _ota_f = false;
            updateNow();
            return true;
        }
        return false;
    }

    // есть ошибка
    bool hasError() {
        return _err != Error::None;
    }

    // прочитать ошибку
    Error getError() {
        return _err;
    }

   private:
    const char* _cur_ver;
    String _path;
    String _host;
    uint16_t _port = 0;
    Error _err = Error::None;
    bool _has_update = false;
    bool _ota_f = false;
    String _auth_token;
    String _user_agent = "AutoOTA";

    bool _extractPath(String& url, String& path, uint16_t* port) {
        if (!url.startsWith("http")) return false;

        int stt = url.indexOf("//");
        if (stt < 0) return false;

        int end = url.indexOf("/", stt + 2);
        if (end < 0) return false;

        *port = url.startsWith("https") ? 443 : 80;
        path = url.substring(end + 1);      // path
        url = url.substring(stt + 2, end);  // host
        return true;
    }

    bool _getJson(String* version = nullptr, String* notes = nullptr, String* bin = nullptr) {
        OtaClient client;
#ifdef ESP8266
        client.setBufferSizes(1024, 512);
#endif
        client.setInsecure();
        if (!_request(client, _host, _path, _port)) return false;

        if (version) *version = _readKey(client, "version");
        if (notes) *notes = _readKey(client, "notes");
        if (bin) {
            String chip;

            while (true) {
                delay(0);
                chip = _readKey(client, "chipFamily");
                if (!chip.length()) return _err = Error::NoPlatform, false;

                if (chip != GOTA_PLATFORM) continue;

                *bin = _readKey(client, "path");
                if (!bin->length()) return _err = Error::NoPath, false;
                break;
            }
        }
        return true;
    }

    String _readKey(Stream& stream, const String& key) {
        while (stream.available()) {
            stream.readStringUntil('"');
            String thisKey = stream.readStringUntil('"');
            if (thisKey == key) {
                stream.readStringUntil('"');
                return stream.readStringUntil('"');
            }
        }
        return String();
    }

    bool _waitClient(OtaClient& client) {
        if (!client.connected()) return false;
        if (!client.available()) {
            uint32_t ms = millis();
            while (!client.available()) {
                delay(0);
                if (millis() - ms >= GOTA_TIMEOUT) {
                    _err = Error::Timeout;
                    return false;
                }
                if (!client.connected()) return false;
            }
        }
        return true;
    }

    bool _request(OtaClient& client, const String& host, const String& path, uint16_t port) {
        client.connect(host.c_str(), port);

        if (client.connected()) {
            {
                String req(F("GET /"));
                req += path;
                req += F(" HTTP/1.1\r\nHost: ");
                req += host;
                req += F("\r\nUser-Agent: ");
                req += _user_agent;
                if (_auth_token.length()) {
                    req += F("\r\nAuthorization: token ");
                    req += _auth_token;
                }
                req += F("\r\n\r\n");
                client.print(req);
            }

            if (!_waitClient(client)) return _err = Error::HTTP, false;

            client.readStringUntil(' ');  // HTTP/1.1 CODE

            uint16_t code = client.parseInt();

            if (code == 200 || code == 302) {
                while (client.available() && client.connected()) {
                    delay(0);
                    String read = client.readStringUntil('\n');

                    if (code == 302 && read.startsWith("Location: ")) {
                        read = read.substring(strlen("Location: "), read.length() - 1);  // redirect
                        if (!_skipClient(client)) break;
                        client.stop();
                        String path;
                        if (!_extractPath(read, path, &port)) break;
                        return _request(client, read, path, port);
                    }

                    if (read.length() == 1) return true;
                    if (!read.length()) break;
                }
            }

            _err = Error::HTTP;
        } else {
            _err = Error::Connect;
        }
        return false;
    }

    bool _skipClient(OtaClient& client) {
        while (client.available() && client.connected()) {
            delay(0);
            uint16_t len = client.readStringUntil('\n').length();
            if (len == 1) return true;
            if (!len) return false;
        }
        return false;
    }
};
