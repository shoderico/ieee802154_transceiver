#include <stdio.h>
#include "unity.h"

#include "ieee802154_transceiver.h"

#include "nvs_flash.h"

#define TEST_CHANNEL 11

void setUp(void) {

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

}

void tearDown(void) {
    // Do nothing
}

TEST_CASE("IEEE 802.15.4 Transceiver Initialization", "[valid]") {
    // Initialize transceiver
    esp_err_t ret = ieee802154_transceiver_init(TEST_CHANNEL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Deinitialize transceiver
    ret = ieee802154_transceiver_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("IEEE 802.15.4 Transceiver Invalid Channel", "[invalid]") {
    // Attempt to initialize with invalid channel
    esp_err_t ret = ieee802154_transceiver_init(10); // Invalid channel
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("IEEE 802.15.4 Transceiver Set Channel", "[valid]") {
    // Initialize transceiver
    esp_err_t ret = ieee802154_transceiver_init(TEST_CHANNEL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Set new channel
    ret = ieee802154_transceiver_set_channel(12);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Deinitialize transceiver
    ret = ieee802154_transceiver_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("IEEE 802.15.4 Transceiver Set RX Callback", "[valid]") {
    // Initialize transceiver
    esp_err_t ret = ieee802154_transceiver_init(TEST_CHANNEL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Set RX callback
    ret = ieee802154_transceiver_set_rx_callback(NULL, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Deinitialize transceiver
    ret = ieee802154_transceiver_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}
