#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "ieee802154_frame.h"
#include "ieee802154_transceiver.h"


#define TAG "SIMPLE_TRANSCEIVER"
#define CHANNEL 11


// Callback for received IEEE 802.15.4 frames.
void esp_ieee802154_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info)
{
    ieee802154_transceiver_handle_receive_done(frame, frame_info);
}

// Callback function for received frames
static void rx_callback(ieee802154_frame_t *frame, esp_ieee802154_frame_info_t *frame_info, void *user_data)
{
    // Validate pointers
    if (!frame || !frame_info) {
        ESP_LOGE(TAG, "Invalid frame or frame_info pointer");
        return;
    }

    // Frame Info (frame_info)
    ESP_LOGI(TAG, "Receiver Frame Info:");
    ESP_LOGI(TAG, "  Pending: %d", frame_info->pending);
    ESP_LOGI(TAG, "  Process: %d", frame_info->process);
    ESP_LOGI(TAG, "  Channel: %d", frame_info->channel);
    ESP_LOGI(TAG, "  RSSI: %d dBm", frame_info->rssi);
    ESP_LOGI(TAG, "  LQI: %d", frame_info->lqi);
    ESP_LOGI(TAG, "  Timestamp: %llu us", frame_info->timestamp);

    // Frame Info (frame)
    ESP_LOGI(TAG, "Frame Info:");
    ESP_LOGI(TAG, "  Payload Length: %zu bytes", frame->payloadLen);
    ESP_LOGI(TAG, "  RSSI_LQI: 0x%02x", frame->rssi_lqi);

    // Frame Control Field (FCF)
    ESP_LOGI(TAG, "Frame Control Field:");
    ESP_LOGI(TAG, "  Frame Type: %d (%s)", frame->fcf.frameType, ieee802154_frame_type_to_str(frame->fcf.frameType));
    ESP_LOGI(TAG, "  Security Enabled: %d", frame->fcf.securityEnabled);
    ESP_LOGI(TAG, "  Frame Pending: %d", frame->fcf.framePending);
    ESP_LOGI(TAG, "  ACK Request: %d", frame->fcf.ackRequest);
    ESP_LOGI(TAG, "  PAN ID Compression: %d", frame->fcf.panIdCompression);
    ESP_LOGI(TAG, "  Sequence Number Suppression: %d", frame->fcf.sequenceNumberSuppression);
    ESP_LOGI(TAG, "  Information Elements Present: %d", frame->fcf.informationElementsPresent);
    ESP_LOGI(TAG, "  Destination Address Mode: %d", frame->fcf.destAddrMode);
    ESP_LOGI(TAG, "  Frame Version: %d", frame->fcf.frameVersion);
    ESP_LOGI(TAG, "  Source Address Mode: %d", frame->fcf.srcAddrMode);

    // Sequence Number
    ESP_LOGI(TAG, "Sequence Number:");
    ESP_LOGI(TAG, "  Sequence Number: %d", frame->sequenceNumber);

    // Address Information
    ESP_LOGI(TAG, "Address Information:");
    ESP_LOGI(TAG, "  Destination PAN ID: 0x%04x", frame->destPanId);
    ESP_LOGI(TAG, "  Destination Address (len=%d):", frame->destAddrLen);
    ESP_LOG_BUFFER_HEX(TAG, frame->destAddress, frame->destAddrLen);
    ESP_LOGI(TAG, "  Source PAN ID: 0x%04x", frame->srcPanId);
    ESP_LOGI(TAG, "  Source Address (len=%d):", frame->srcAddrLen);
    ESP_LOG_BUFFER_HEX(TAG, frame->srcAddress, frame->srcAddrLen);

    // Payload
    ESP_LOGI(TAG, "Payload:");
    ESP_LOG_BUFFER_HEX(TAG, frame->payload, frame->payloadLen);
}

// Task to periodically transmit a test frame
static void transmit_task(void *pvParameters)
{
    uint8_t payloadData[] = "Hello, IEEE 802.15.4!";

    ieee802154_frame_t frame = {
        .fcf = {
            .frameType = IEEE802154_FRAME_TYPE_DATA,
            .destAddrMode = IEEE802154_ADDR_MODE_SHORT,
            .srcAddrMode = IEEE802154_ADDR_MODE_SHORT,
            .frameVersion = IEEE802154_VERSION_2006,
        },
        .sequenceNumber = 0x01,
        .destPanId = 0x1234,
        .destAddress = {0xFF, 0xFF}, // Broadcast
        .destAddrLen = 2,
        .srcPanId = 0x1234,
        .srcAddress = {0xAB, 0xCD},
        .srcAddrLen = 2,
    };
    frame.payloadLen = sizeof(payloadData),
    frame.payload = malloc(sizeof(payloadData));
    memcpy( frame.payload, payloadData, sizeof(payloadData) );

    while (1) {
        esp_err_t ret = ieee802154_transceiver_transmit(&frame);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Transmitted frame");
        } else {
            ESP_LOGE(TAG, "Transmit failed: %d", ret);
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Transmit every 5 seconds
    }

    free(frame.payload);
}

void app_main(void)
{
    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Set the receive callback
    ret = ieee802154_transceiver_set_rx_callback(rx_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set RX callback: %d", ret);
        return;
    }

    // Initialize the IEEE 802.15.4 transceiver
    ret = ieee802154_transceiver_init(CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize transceiver: %d", ret);
        return;
    }

    // Start the transmit task
    xTaskCreate(transmit_task, "transmit_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Simple transceiver started on channel %d", CHANNEL);
}