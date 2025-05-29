#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"

#include "esp_log.h"
#include "esp_ieee802154.h"

#include "ieee802154_transceiver.h"

#define TAG "IEEE802154_TRANSCEIVER"
#define MAX_FRAME_LEN 128

// Structure to hold frame data and frame info
typedef struct {
    uint8_t frame[MAX_FRAME_LEN]; // Raw frame data
    esp_ieee802154_frame_info_t frame_info; // Frame info (RSSI, LQI, etc.)
} frame_data_t;

// Global state
static StreamBufferHandle_t message_buffer = NULL;
static ieee802154_transceiver_rx_callback_t rx_callback = NULL;
static void *rx_callback_user_data = NULL;
static TaskHandle_t rx_task_handle = NULL;
static bool radio_enabled = false;

// Transmit buffer
static uint8_t transmit_buffer[ MAX_FRAME_LEN ] = {0};

// Forward declarations
static void receive_packet_task(void *pvParameters);

/**
 * @brief Initialize the IEEE 802.15.4 radio in promiscuous mode with a specified channel.
 *
 * @param channel Channel number (11-26) to set for the transceiver.
 * @note NVS must be initialized by the user before calling this function.
 */
esp_err_t ieee802154_transceiver_init(uint8_t channel) {
    esp_err_t ret;

    // Validate channel
    if (channel < 11 || channel > 26) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }

    // Create message buffer
    message_buffer = xMessageBufferCreate(sizeof(frame_data_t) + 4);
    if (!message_buffer) {
        ESP_LOGE(TAG, "Failed to create message buffer");
        ieee802154_transceiver_deinit();
        return ESP_ERR_NO_MEM;
    }

    // Initialize IEEE 802.15.4 radio
    ret = esp_ieee802154_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable IEEE 802.15.4 radio: %d", ret);
        ieee802154_transceiver_deinit();
        return ret;
    }
    radio_enabled = true;

    ret = esp_ieee802154_set_coordinator(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set coordinator mode to false: %d", ret);
        ieee802154_transceiver_deinit();
        return ret;
    }

    ret = esp_ieee802154_set_promiscuous(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable promiscuous mode: %d", ret);
        ieee802154_transceiver_deinit();
        return ret;
    }

    ret = esp_ieee802154_set_rx_when_idle(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set rx when idle: %d", ret);
        ieee802154_transceiver_deinit();
        return ret;
    }

    ret = esp_ieee802154_set_channel(channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set channel %d: %d", channel, ret);
        ieee802154_transceiver_deinit();
        return ret;
    }

    ret = esp_ieee802154_receive();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start receiving: %d", ret);
        ieee802154_transceiver_deinit();
        return ret;
    }

    // Start receive task
    if (xTaskCreate(receive_packet_task, "RX", 1024 * 5, NULL, 5, &rx_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create receive task");
        ieee802154_transceiver_deinit();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "IEEE 802.15.4 transceiver initialized on channel %d", channel);
    return ESP_OK;
}

/**
 * @brief Deinitialize the transceiver and free resources.
 */
esp_err_t ieee802154_transceiver_deinit(void) {
    esp_err_t ret;

    // Stop receive task
    if (rx_task_handle) {
        vTaskDelete(rx_task_handle);
        rx_task_handle = NULL;
    }

    // Free message buffer
    if (message_buffer) {
        vMessageBufferDelete(message_buffer);
        message_buffer = NULL;
    }

    // Disable radio
    if (radio_enabled) {
        ret = esp_ieee802154_disable();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to disable IEEE 802.15.4 radio: %d", ret);
            return ret;
        }
        radio_enabled = false;
    }

    ESP_LOGI(TAG, "IEEE 802.15.4 transceiver deinitialized");
    return ESP_OK;
}

/**
 * @brief Set the receive callback function.
 */
esp_err_t ieee802154_transceiver_set_rx_callback(ieee802154_transceiver_rx_callback_t callback, void *user_data) {
    rx_callback = callback;
    rx_callback_user_data = user_data;
    ESP_LOGI(TAG, "Receive callback set");
    return ESP_OK;
}



// Internal: Transmit an IEEE 802.15.4 frame
esp_err_t transmit_channel(const ieee802154_frame_t *frame, uint8_t channel, bool change_channel)
{
    bool verbose = false;
    esp_err_t ret;

    if (!frame) {
        ESP_LOGE(TAG, "Invalid frame pointer");
        return ESP_ERR_INVALID_ARG;
    }

    if (change_channel) {
        if (channel < 11 || channel > 26) {
            ESP_LOGE(TAG, "Invalid channel: %d", channel);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Prepare buffer
    memset(transmit_buffer, 0, MAX_FRAME_LEN); // Clear

    // Build frame into a byte array
    size_t len = ieee802154_frame_build(frame, transmit_buffer, false);
    if (len == 0) {
        ESP_LOGE(TAG, "Failed to build frame");
        return ESP_FAIL;
    }
    // ESP_LOGI(TAG, "len: %d, buffer[0]: %d", len, buffer[0]);
    // ESP_LOG_BUFFER_HEX(TAG, buffer, buffer[0]);

    if (change_channel) {
        // Set channel
        ret = esp_ieee802154_set_channel(channel);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set channel %d: %d", channel, ret);
            return ret;
        }
    }

    // Transmit frame
    ret = esp_ieee802154_transmit( transmit_buffer, false );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit frame: %d", ret);
        return ret;
    }

    if (verbose) {
        if (change_channel)
            ESP_LOGI(TAG, "Transmitted frame of %zu bytes on channel %d", len, channel);
        else   
            ESP_LOGI(TAG, "Transmitted frame of %zu bytes", len);
    }
    return ESP_OK;

}


/**
 * @brief Transmit an IEEE 802.15.4 frame on the current channel.
 */
esp_err_t ieee802154_transceiver_transmit(const ieee802154_frame_t *frame) {
    return transmit_channel(frame, -1, false);
}

/**
 * @brief Transmit an IEEE 802.15.4 frame on a specified channel.
 */
esp_err_t ieee802154_transceiver_transmit_channel(const ieee802154_frame_t *frame, uint8_t channel) {
    return transmit_channel(frame, channel, true);
}



/**
 * @brief Set the IEEE 802.15.4 channel.
 */
esp_err_t ieee802154_transceiver_set_channel(uint8_t channel) {
    if (channel < 11 || channel > 26) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    // Set channel
    ret = esp_ieee802154_set_channel(channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set channel %d: %d", channel, ret);
        return ret;
    }

    // Start receiving
    ret = esp_ieee802154_receive();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start receiving: %d", ret);
        return ret;
    }

    // ESP_LOGI(TAG, "Channel set to %d", channel);
    return ESP_OK;
}

/**
 * @brief Handle the callback for received IEEE 802.15.4 frames.
 */
void ieee802154_transceiver_handle_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info) {
    if (!message_buffer) {
        esp_ieee802154_receive_handle_done(frame);
        return;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;

    // Prepare packet
    frame_data_t packet;
    memcpy(packet.frame, frame, frame[0]);
    packet.frame_info = *frame_info;

    // Send to message buffer
    size_t bytes_sent = xMessageBufferSendFromISR(message_buffer, &packet, sizeof(frame_data_t), &higher_priority_task_woken);
    if (bytes_sent == 0) {
        ESP_EARLY_LOGW(TAG, "Message buffer full, packet discarded");
    }

    esp_ieee802154_receive_handle_done(frame);

    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/**
 * @brief Task to process received packets and invoke callback.
 */
static void receive_packet_task(void *pvParameters) {
    frame_data_t packet;
    ieee802154_frame_t frame = {0};

    ESP_LOGI(TAG, "Receive packet task started");

    while (1) {
        // Receive packet from message buffer
        size_t read_bytes = xMessageBufferReceive(message_buffer, &packet, sizeof(frame_data_t), pdMS_TO_TICKS(10));
        if (read_bytes != sizeof(frame_data_t)) {
            if (read_bytes != 0) {
                ESP_LOGE(TAG, "Invalid packet size received: %d", read_bytes);
            }
            continue;
        }

        // Parse frame
        if (!ieee802154_frame_parse(packet.frame, &frame, false)) {
            ESP_LOGE(TAG, "Failed to parse frame");
            continue;
        }

        // Invoke callback if set
        if (rx_callback) {
            rx_callback(&frame, &packet.frame_info, rx_callback_user_data);
        }

        // Short delay to yield CPU
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

