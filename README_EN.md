This is an automatic translation and may be incorrect in some places. See the source README and examples for authoritative information.

[![latest](https://img.shields.io/github/v/release/GyverLibs/AutoOTA.svg?color=brightgreen)](https://github.com/GyverLibs/AutoOTA/releases/latest/download/AutoOTA.zip)
[![PIO](https://badges.registry.platformio.org/packages/gyverlibs/library/AutoOTA.svg)](https://registry.platformio.org/libraries/gyverlibs/AutoOTA)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD%24%E2%82%AC%20%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D1%82%D1%8C-%D0%B0%D0%B2%D1%82%D0%BE%D1%80%D0%B0-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/AutoOTA?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# AutoOTA
A library to automatically check OTA for project updates from GitHub and other sources

### Compatibility
ESP8266/ESP32

## Contents
- [Use of use](#usage)
- [Versions](#versions)
- [Installation](#install)
- [Bugs and feedback](#feedback)

<a id="usage"></a>

## Use of use
- Indicate in the firmware the current version and the path to the file with information`project.json`
- When updating download binaries and increase the version in the information file
- The program will check the version and offer to update.

Library accepts three paths to file`project.json`:
- `https://...json`Fully your path to the file, the port will be determined automatically 80 or 443, unless manually specified in the constructor.
- `user/repo`- file on GitHub at the root of the branch repository`main`
- `user/repo/main/folder/file.json`- file on GitHub on the specified path in the specified branch

```cpp
AutoOTA(const char* cur_ver, const char* url, uint16_t port = 0);

// current
const char* version();

// Check the updates. You can send lines to record information.
bool checkUpdate(String* version = nullptr, String* notes = nullptr, String* bin = nullptr);

// There's an update. Call after the check. He'll throw himself into falsehood.
bool hasUpdate();

// loop-up
void update();

// Update the firmware now and restart the chip
bool updateNow();

// ticker, call the loop. Return True When Trying to Upgrade
bool tick();

// bug
bool hasError();

// mistake
Error getError();
```

### Examples
Local object, update immediately
```cpp
// AutoOTA ota("1.0", "GyverLibs/GyverHub-example") // If the file is at the root of the main branch repository

AutoOTA ota("1.0", "GyverLibs/GyverHub-example/main/project.json");
if (ota.checkUpdate()) {
    ota.updateNow();
}
```

Update from loop
```cpp
// AutoOTA ota("1.0", "GyverLibs/GyverHub-example") // If the file is at the root of the main branch repository

AutoOTA ota("1.0", "GyverLibs/GyverHub-example/main/project.json");

void setup() {
    if (ota.checkUpdate()) {
        ota.update();
    }
}

void loop() {
    ota.tick();
}
```

Receipt of update information
```cpp
String ver, notes;
if (ota.checkUpdate(&ver, &notes)) {
    Serial.println(ver);
    Serial.println(notes);
}
```

## Project.json file
Instructions for the design of the repository and manifesto are available[here](https://github.com/AlexGyver/ota-projects).

<a id="versions"></a>

## Versions
- v1.0
- v1.2.0

<a id="install"></a>
## Installation
- The library can be found under the name **AutoOTA** and installed through the library manager in:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Download the library](https://github.com/GyverLibs/AutoOTA/archive/refs/heads/main.zip).zip archive for manual installation:
    - Unpack and put in *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Unpack and put in *C:\Program Files\Arduino\libraries* (Windows x32)
    - Unpack and put in *Documents/Arduino/libraries/ *
    - (Arduino IDE) Automatic installation from .zip: *Sketch/Connect library/Add .ZIP library...* and specify downloaded archive
- Read more detailed instructions for installing libraries[here](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: new versions fix errors and bugs, as well as optimize and add new features.
- Through the library manager IDE: find the library as when installing and click "Update"
- Manually: **Delete the folder with the old version** and then put the new one in its place. “Replacement” can not be done: sometimes new versions delete files that will remain when replaced and can lead to errors!

<a id="feedback"></a>

## Bugs and feedback
If you find bugs, create **Issue**, or better write to the mail immediately.[alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
The library is open for revision and your **Pull Requests*!

When reporting bugs or incorrect work of the library, it is necessary to specify:
- Library version
- What is used by the IC
- SDK version (for ESP)
- Arduino IDE version
- Are embedded examples that use features and designs that cause bugs in your code working correctly?
- What code was downloaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed. Not a canvas of a thousand lines, but a minimum code.
