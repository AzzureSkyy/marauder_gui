
![IoT Mug Logo](https://www.iotmug.com/images/thumbs/0000644_iot-logo-2-removebg-preview%20(1).png)

# Marauder GUI

Maurauder GUI working on this firmware esp32_marauder_v1_20_0_20261028_esp32c5devkitc1.bin for the custom tweaks made,using an IOTMUG 5 in 1 dev board with gps,433m,868m,NRF24,and,ESPC5.

https://www.iotmug.com/iotmug-5-in-1-module-expansion-board-compatible-with-flipper-zero-esp32-c5-24ghz5ghz-wifi-nrf24-cc1101-433mhz868mhz-gps-sd-card-support

Marauder GUI is a bilingual Flipper Zero companion application for controlling an ESP32 Marauder
board over GPIO UART. It provides structured menus for the Marauder serial CLI, so commands do not
need to be typed manually.

## Hardware and connection

- Flipper Zero
- An ESP32 board running compatible ESP32 Marauder firmware
- Flipper GPIO 13 (TX) to Marauder RX
- Flipper GPIO 14 (RX) to Marauder TX
- Common GND
- 115200 baud, 3.3 V UART logic

The application does not include or modify ESP32 Marauder firmware. Wi-Fi and Bluetooth operations
are performed by the connected ESP32 board.

## Features

- Wi-Fi scanning and AP selection
- Wi-Fi testing and beacon tools
- Wi-Fi SSID spam from a custom SSID list (see below)
- Evil Portal with a choice of built-in or custom HTML pages (see below)
- Bluetooth testing and tracker tools
- Wi-Fi and Bluetooth detectors
- Ping, ARP, and port scanning
- Device information and Marauder settings
- Raw serial terminal
- Persistent Turkish and English interface

## Revisions in this version

Compared with v1.1 in `docs/changelog.md`:

- **Removed Rick Roll** from the Wi-Fi spam types and from the Wi-Fi menu description.
- **Removed the fake AP option.** The separate "Custom SSID File" menu item is gone. The Wi-Fi spam
  menu now lists only Random SSID and Funny Names.
- **Random SSID loads a custom SSID list and rotates through it in safe-sized windows.** It reads
  `ssids.txt` from the SD card. The file can be any size - it is never loaded into memory at
  once. See "Custom SSID list" for how the rotation works.
- **Evil Portal HTML selection.** After choosing an AP, you pick the portal page: "Default
  (Tester)" (the built-in email-login page) or any `.html`/`.htm` file in the `html` folder.
  The built-in page is still the default and is unchanged.

## Custom SSID list

Create this file on the SD card:

```
apps_data/marauder_gui/ssids.txt
```

- One SSID per line.
- The last line may or may not end with a newline.
- Blank lines are skipped.
- SSIDs may contain spaces. Lines containing a double quote (") are skipped.

Start it from **WiFi > WiFi Spam > Random SSID**. Enter a window size from 1 to 500 (also capped
at 500 internally for safety, regardless of what is entered - see below).

The app then loops through the file in rounds, each round:

1. Reads the next window's worth of SSIDs (up to the window size) from wherever the file left off,
   reading in 512-byte chunks so the file's size never has to fit in memory.
2. Sends each one as its own `ssid -a -n "<name>"` command (the only syntax the ESP32 Marauder
   firmware accepts), a few lines per 100ms tick so the UI and Stop button stay responsive.
3. Sends `select -s all` to mark the newly added names, since `ssid -a` leaves them unselected and
   `attack -t beacon -l` only broadcasts selected entries.
4. Starts `attack -t beacon -l` and leaves the window broadcasting for 20 seconds.
5. Stops the attack, clears the ESP32's list (`clearlist -s`), and moves on to the next window.

When the end of the file is reached, it wraps back around to the start, so the whole list keeps
cycling for as long as the scene is open. This is the "periodically clears and rebroadcasts"
behavior from earlier builds, done safely in bounded pieces instead of one big batch.

**Safety cap:** ESP32 Marauder's firmware starts refusing new SSIDs once its free heap drops too
low, which on hardware showed up as the list silently stopping growing somewhere around ~1000-1100
entries. The app never loads more than 500 SSIDs onto the board at once, regardless of the window
size entered, to stay well clear of that limit no matter how large ssids.txt is.

Funny Names remains in the same menu and is unchanged.

## Evil Portal HTML pages

Put your own pages on the SD card:

```
apps_data/marauder_gui/html/
```

Only `.html` and `.htm` files are listed. If the folder is missing or empty, only "Default
(Tester)" is shown.

Open **WiFi > Attack > Evil Portal (Karma)**, then pick a network name from the list. The next screen
lists the HTML files and "Default (Tester)". The chosen page is served by Evil Portal. If a custom
file cannot be opened, the app falls back to the built-in page.

## Known limitations

- These changes are built and checked on the firmware side. They have not yet been tested on a
  Flipper Zero against a live ESP32 Marauder. Test with a small `ssids.txt` first.
- The 20-second rotation and 500-SSID safety cap are fixed values, not user-configurable yet.

## Installation

Copy marauder_gui.fap to apps/GPIO/ on the Flipper Zero SD card, or install the application from
the Flipper Apps Catalog when available.

To build from source, place this folder in `applications_user/marauder_gui` of the Flipper firmware
tree and run `./fbt fap_marauder_gui` (or use `ufbt` in this folder).

## Responsible use

Use wireless testing features only on systems you own or have explicit permission to test. Users
are responsible for complying with applicable laws and regulations.

## Author

mel4mi
