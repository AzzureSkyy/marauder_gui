# Marauder GUI

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
- **Random SSID loads a custom SSID list.** It reads `ssids.txt` from the SD card. The file can be
  any size. Names are sent one at a time in paced batches (see "Custom SSID list").
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

Start it from **WiFi > WiFi Spam > Random SSID**. Enter a batch size from 1 to 500. The app sends
each SSID as its own `ssid -a -n "<name>"` command, which is the form the ESP32 Marauder firmware
accepts. It pauses 20 ms between names and 100 ms after each batch. When the whole file has been
sent, the beacon attack starts with `attack -t beacon -l`.

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
- Very large lists depend on how many SSIDs the ESP32 Marauder firmware can hold in memory. A
  2,800-line list has not been tested on hardware.

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
