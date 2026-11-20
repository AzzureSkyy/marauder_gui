#include "../marauder_gui_app_i.h"
#include <stdio.h>
#include <string.h>
#include <storage/storage.h>

enum {
    WifiSpamEventStop,
};

/* Pause between individual "ssid -a -n" commands so the ESP32 UART buffer isn't overrun */
#define WIFI_SPAM_SSID_GAP_MS 20

/* "attack -t beacon -r/funny" - fake-AP beacon spam types that need no target
   selection (unlike "attack -t beacon -a", which spams using the currently selected APs, or
   "-l", which needs an SSID list loaded first via the "ssid" command - both skipped for now).
   See ATTACK_TYPE_BEACON/_RR/_FUNNY handling in the ESP32 Marauder source's CommandLine.cpp.
   Exposed via accessor functions so marauder_gui_scene_wifi_spam_menu.c can list them without
   duplicating this table. */
static const struct {
    const char* label_tr;
    const char* label_en;
    const char* command;
    const char* description_tr;
    const char* description_en;
} marauder_wifi_spam_types[] = {
    {"Rastgele SSID",
     "Random SSID",
     "attack -t beacon -r",
     "ssids.txt dosyasindan SSID'leri batch halinde yayinlar.",
     "Broadcasts SSIDs from ssids.txt in batches."},
    {"Komik Isimler",
     "Funny Names",
     "attack -t funny",
     "Onceden tanimli esprili SSID isimleriyle sahte agler yayinlar.",
     "Broadcasts fake networks with pre-defined joke SSID names."},
};
#define MARAUDER_WIFI_SPAM_TYPE_COUNT \
    (sizeof(marauder_wifi_spam_types) / sizeof(marauder_wifi_spam_types[0]))

size_t marauder_gui_wifi_spam_type_count(void) {
    return MARAUDER_WIFI_SPAM_TYPE_COUNT;
}

const char* marauder_gui_wifi_spam_type_label(size_t index) {
    return (index < MARAUDER_WIFI_SPAM_TYPE_COUNT) ? marauder_wifi_spam_types[index].label_tr : "?";
}

const char* marauder_gui_wifi_spam_type_label_en(size_t index) {
    return (index < MARAUDER_WIFI_SPAM_TYPE_COUNT) ? marauder_wifi_spam_types[index].label_en : "?";
}

const char* marauder_gui_wifi_spam_type_command(size_t index) {
    return (index < MARAUDER_WIFI_SPAM_TYPE_COUNT) ? marauder_wifi_spam_types[index].command :
                                                       "attack -t beacon -r";
}

const char* marauder_gui_wifi_spam_type_description(size_t index) {
    return (index < MARAUDER_WIFI_SPAM_TYPE_COUNT) ?
               marauder_wifi_spam_types[index].description_tr :
               "?";
}

const char* marauder_gui_wifi_spam_type_description_en(size_t index) {
    return (index < MARAUDER_WIFI_SPAM_TYPE_COUNT) ?
               marauder_wifi_spam_types[index].description_en :
               "?";
}

static void marauder_gui_scene_wifi_spam_button_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    MarauderGuiApp* app = context;
    if(type == InputTypeShort && result == GuiButtonTypeCenter) {
        view_dispatcher_send_custom_event(app->view_dispatcher, WifiSpamEventStop);
    }
}

static void marauder_gui_scene_wifi_spam_redraw(MarauderGuiApp* app) {
    widget_reset(app->widget);
    widget_add_string_element(app->widget, 64, 2, AlignCenter, AlignTop, FontPrimary, "WiFi Spam");
    widget_add_string_element(
        app->widget, 64, 14, AlignCenter, AlignTop, FontSecondary, app->wifi_spam_status_label);
    widget_add_text_box_element(
        app->widget, 0, 26, 128, 22, AlignCenter, AlignTop, app->last_uart_line, true);
    widget_add_button_element(
        app->widget,
        GuiButtonTypeCenter,
        marauder_gui_text(app, "Durdur", "Stop"),
        marauder_gui_scene_wifi_spam_button_callback,
        app);
}

static void marauder_gui_scene_wifi_spam_uart_line(MarauderGuiApp* app, const char* line) {
    strncpy(app->last_uart_line, line, MARAUDER_LINE_MAX - 1);
    app->last_uart_line[MARAUDER_LINE_MAX - 1] = '\0';
    marauder_gui_scene_wifi_spam_redraw(app);
}

/* ESP32 Marauder's "ssid -a" accepts one name per command ("-n <name>"). Quotes keep names
   containing spaces together through the firmware's argument tokenizer. */
static void marauder_gui_scene_wifi_spam_send_ssid(MarauderGuiApp* app, const char* ssid) {
    static char cmd[256 + 16];
    snprintf(cmd, sizeof(cmd), "ssid -a -n \"%s\"", ssid);
    marauder_uart_send_line(app->uart, cmd);
}

static void marauder_gui_scene_wifi_spam_show_progress(MarauderGuiApp* app, int total_count) {
    snprintf(
        app->wifi_spam_status_label,
        sizeof(app->wifi_spam_status_label),
        app->language == MarauderLanguageTurkish ? "Yayinlaniyor: %d" : "Broadcasting: %d",
        total_count);
    marauder_gui_scene_wifi_spam_redraw(app);
}

void marauder_gui_scene_wifi_spam_on_enter(void* context) {
    MarauderGuiApp* app = context;

    app->last_uart_line[0] = '\0';
    app->uart_line_handler = marauder_gui_scene_wifi_spam_uart_line;

    view_dispatcher_switch_to_view(app->view_dispatcher, MarauderGuiViewWidget);

    if(app->wifi_spam_use_count) {
        /* Load custom SSIDs from file and broadcast in intelligent batches */
        Storage* storage = furi_record_open(RECORD_STORAGE);
        File* file = storage_file_alloc(storage);

        const char* ssid_file_path = EXT_PATH("apps_data/marauder_gui/ssids.txt");

        snprintf(app->wifi_spam_status_label, sizeof(app->wifi_spam_status_label),
                 app->language == MarauderLanguageTurkish ? "Dosya okunuyor..." : "Reading file...");
        marauder_gui_scene_wifi_spam_redraw(app);

        if(!storage_file_open(file, ssid_file_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
            snprintf(app->wifi_spam_status_label, sizeof(app->wifi_spam_status_label),
                     app->language == MarauderLanguageTurkish ? "Dosya bulunamadi!" : "File not found!");
            marauder_gui_scene_wifi_spam_redraw(app);
            storage_file_free(file);
            furi_record_close(RECORD_STORAGE);
            return;
        }

        /* Stream SSIDs from file one line at a time, pausing after each batch */
        static char line_buffer[256];
        int batch_count = 0;
        int total_count = 0;
        int batch_size = app->wifi_spam_count;  /* Use the input as batch size */

        marauder_uart_send_line(app->uart, "clearlist -s");

        /* Read the file in 512-byte chunks, so only one chunk and one line are in RAM at a time */
        static uint8_t chunk[512];
        size_t line_pos = 0;
        while(true) {
            size_t chunk_len = storage_file_read(file, chunk, sizeof(chunk));
            bool eof = chunk_len == 0;
            if(eof) {
                /* End of file acts as a line break so a final line without '\n' is still sent */
                chunk[0] = '\n';
                chunk_len = 1;
            }
            for(size_t i = 0; i < chunk_len; i++) {
                uint8_t byte = chunk[i];
                if(byte == '\n' || byte == '\r') {
                    if(line_pos > 0) {
                        line_buffer[line_pos] = '\0';

                        /* Skip empty lines and names with '"', which can't be quoted in the command */
                        if(strlen(line_buffer) > 0 && strchr(line_buffer, '"') == NULL) {
                            marauder_gui_scene_wifi_spam_send_ssid(app, line_buffer);
                            total_count++;
                            batch_count++;
                            furi_delay_ms(WIFI_SPAM_SSID_GAP_MS);

                            /* After each full batch, update progress and pause briefly */
                            if(batch_count >= batch_size) {
                                marauder_gui_scene_wifi_spam_show_progress(app, total_count);
                                batch_count = 0;
                                furi_delay_ms(100);
                            }
                        }
                        line_pos = 0;
                    }
                } else if(byte >= 32 && byte < 127) {
                    /* Printable ASCII character */
                    if(line_pos < sizeof(line_buffer) - 1) {
                        line_buffer[line_pos++] = (char)byte;
                    }
                }
            }
            if(eof) break;
        }

        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);

        /* Start broadcasting the loaded list */
        snprintf(app->wifi_spam_status_label, sizeof(app->wifi_spam_status_label),
                 app->language == MarauderLanguageTurkish ? "%d SSID Yayinlaniyor" : "Broadcasting %d SSIDs",
                 total_count);
        marauder_gui_scene_wifi_spam_redraw(app);
        marauder_uart_send_line(app->uart, "attack -t beacon -l");

    } else {
        strncpy(
            app->wifi_spam_status_label,
            marauder_gui_text(
                app,
                marauder_gui_wifi_spam_type_label(app->wifi_spam_type_index),
                marauder_gui_wifi_spam_type_label_en(app->wifi_spam_type_index)),
            sizeof(app->wifi_spam_status_label) - 1);
        app->wifi_spam_status_label[sizeof(app->wifi_spam_status_label) - 1] = '\0';

        marauder_gui_scene_wifi_spam_redraw(app);
        marauder_uart_send_line(app->uart, marauder_gui_wifi_spam_type_command(app->wifi_spam_type_index));
    }
}

bool marauder_gui_scene_wifi_spam_on_event(void* context, SceneManagerEvent event) {
    MarauderGuiApp* app = context;
    bool consumed = false;

    if((event.type == SceneManagerEventTypeCustom && event.event == WifiSpamEventStop) ||
       event.type == SceneManagerEventTypeBack) {
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void marauder_gui_scene_wifi_spam_on_exit(void* context) {
    MarauderGuiApp* app = context;

    marauder_uart_send_line(app->uart, "stopscan");

    app->uart_line_handler = NULL;
    widget_reset(app->widget);
}
