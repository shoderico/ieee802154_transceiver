#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "ieee802154_transceiver.h"

#define TAG "IEEE802154_SNIFFER"

#define RX_CHANNEL 11



// Function to format a byte buffer as a hexadecimal string in one line
int hex_dump_in_oneline(char *line, size_t line_size, const uint8_t *buffer, size_t len)
{
    size_t line_pos = 0; // Current position in the output line

    // Check if the output buffer size is valid
    if (line_size == 0) {
        return 0; // Return 0 if no space is available
    }

    // Iterate through each byte in the input buffer
    for (size_t i = 0; i < len; i++) {
        // Append the byte as a two-digit hexadecimal value
        int written = snprintf(line + line_pos, line_size - line_pos, "%02x", buffer[i]);
        if (written < 0 || (size_t)written >= line_size - line_pos) {
            ESP_LOGE(TAG, "Buffer overflow at byte %zu", i);
            break; // Stop if there's not enough space
        }
        line_pos += written;

        // Add a double space every 8 bytes, except for the last byte
        if ((i + 1) % 8 == 0 && i != len - 1) {
            written = snprintf(line + line_pos, line_size - line_pos, "  ");
            if (written < 0 || (size_t)written >= line_size - line_pos) {
                ESP_LOGE(TAG, "Buffer overflow at delimiter %zu", i);
                break; // Stop if there's not enough space
            }
            line_pos += written;
        } else if (i != len - 1) {
            // Add a single space between bytes, except for the last byte
            written = snprintf(line + line_pos, line_size - line_pos, " ");
            if (written < 0 || (size_t)written >= line_size - line_pos) {
                ESP_LOGE(TAG, "Buffer overflow at space %zu", i);
                break; // Stop if there's not enough space
            }
            line_pos += written;
        }
    }

    // Ensure the output string is null-terminated
    if (line_pos < line_size) {
        line[line_pos] = '\0';
    } else {
        line[line_size - 1] = '\0'; // Truncate if buffer is full
    }

    return line_pos; // Return the number of characters written
}



// Callback for received IEEE 802.15.4 frames.
void esp_ieee802154_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info)
{
    ieee802154_transceiver_handle_receive_done(frame, frame_info);
}

// Callback for received frames.
static void rx_callback(ieee802154_frame_t *frame, esp_ieee802154_frame_info_t *frame_info, void *user_data)
{
    char buff[512] = {0};
    int pos = 0;
    pos += sprintf(&buff[pos], "frameType: %s", ieee802154_frame_type_to_str(frame->fcf.frameType));
    pos += sprintf(&buff[pos], ", seqNum: %02x", frame->sequenceNumber);
    pos += sprintf(&buff[pos], ", dstPanId: %04x", frame->destPanId);

    pos += sprintf(&buff[pos], ", dstAddr: ");
    pos += hex_dump_in_oneline(&buff[pos], sizeof(buff) - pos, frame->destAddress, frame->destAddrLen);

    pos += sprintf(&buff[pos], ", srcAddr: ");
    pos += hex_dump_in_oneline(&buff[pos], sizeof(buff) - pos, frame->srcAddress, frame->srcAddrLen);

    pos += sprintf(&buff[pos], ", payload: ");
    pos += hex_dump_in_oneline(&buff[pos], sizeof(buff) - pos, frame->payload, frame->payloadLen);

    ESP_LOGI(TAG, "%s", buff);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS: %d", ret);
            return;
        }
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize NVS: %d", ret);
            return;
        }
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %d", ret);
        return;
    }

    // Set receive callback
    ret = ieee802154_transceiver_set_rx_callback(rx_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set receive callback: %d", ret);
        return;
    }

    // Initialize transceiver with channel
    ret = ieee802154_transceiver_init(RX_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize transceiver: %d", ret);
        return;
    }
}