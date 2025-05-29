#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include <esp_ieee802154.h>
#include <esp_log.h>
//#include <esp_mac.h>
#include <esp_timer.h>

#include "ieee802154_transceiver.h"


#define TAG "IEEE802154_BRIDGE"

#define RX_CHANNEL 11
#define TX_CHANNEL 13

static bool transmitting = false;



//=========================================================================================
// Log performance

#define NUM_SAMPLES 100
static int64_t total_time_us = 0;
static uint32_t sample_count = 0;
static int64_t start_time = 0;
static QueueHandle_t log_queue = NULL;

void capture_start()
{
    start_time = esp_timer_get_time();
}

void capture_done()
{
    int64_t end_time = esp_timer_get_time();
    int64_t elapsed_time = end_time - start_time;

    total_time_us += elapsed_time;
    sample_count++;

    if (sample_count >= NUM_SAMPLES) {
        float average_time_us = (float)total_time_us / NUM_SAMPLES;
        if (log_queue != NULL) {
            xQueueSendFromISR(log_queue, &average_time_us, NULL);
        }
        total_time_us = 0;
        sample_count = 0;
    }
}

void log_task(void *pvParameters)
{
    float average_time_us;
    while (1) {
        if (xQueueReceive(log_queue, &average_time_us, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Average execution time over %d samples: %.2f us", NUM_SAMPLES, average_time_us);
        }
    }
}



//=========================================================================================
// esp_ieee802154 interrupt callbacks

// The SFD field of the frame was received.
void esp_ieee802154_receive_sfd_done(void)
{
	// ESP_EARLY_LOGI(TAG, "rx sfd done");
}

// Callback for received IEEE 802.15.4 frames.
void esp_ieee802154_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info)
{
    capture_start();
    ieee802154_transceiver_handle_receive_done(frame, frame_info);
}

// The Frame Transmission succeeded.
void esp_ieee802154_transmit_done(const uint8_t *frame, const uint8_t *ack, esp_ieee802154_frame_info_t *ack_frame_info)
{
    transmitting = false;
    capture_done();
	// ESP_EARLY_LOGI(TAG, "tx OK, sent %d bytes, ack %d", frame[0], ack != NULL);
}

// The Frame Transmission failed.
void esp_ieee802154_transmit_failed(const uint8_t *frame, esp_ieee802154_tx_error_t error)
{
    transmitting = false;
    capture_done();
	ESP_EARLY_LOGW(TAG, "tx failed, error %d", error);
}

// The SFD field of the frame was transmitted.
void esp_ieee802154_transmit_sfd_done(uint8_t *frame)
{
	// ESP_EARLY_LOGI(TAG, "tx sfd done");
}



//=========================================================================================
// Callback for received frames.
static void rx_callback(ieee802154_frame_t *frame, esp_ieee802154_frame_info_t *frame_info, void *user_data)
{
    // ESP_LOGI(TAG, "Received.");
    // ESP_LOGI(TAG, "payloadLen: %d", frame->payloadLen);

    // Send to another channel
    esp_err_t ret;
    ret = ieee802154_transceiver_transmit_channel(frame, TX_CHANNEL);
    if (ret) {
        ESP_LOGE(TAG, "transmit failed.");
    }

    // Wait for the end of transmitting
    transmitting = true;
    while (transmitting) { vTaskDelay( 1 / portTICK_PERIOD_MS ); }

    // Back to receiving channel
    ret = ieee802154_transceiver_set_channel(RX_CHANNEL);
    if (ret) {
        ESP_LOGE(TAG, "recover channel failed.");
    }
}


//=========================================================================================
// main
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

    // Prepare performance log
    log_queue = xQueueCreate(10, sizeof(float));
    if (log_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create log queue");
        return;
    }
    xTaskCreate(log_task, "log_task", 2048, NULL, 5, NULL);


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

    ESP_LOGI(TAG, "esp_ieee802154_get_pending_mode: %d", esp_ieee802154_get_pending_mode());
    ESP_LOGI(TAG, "esp_ieee802154_get_txpower: %d", esp_ieee802154_get_txpower());
}