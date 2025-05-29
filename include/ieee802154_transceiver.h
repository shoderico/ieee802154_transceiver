#ifndef IEEE802154_TRANSCEIVER_H
#define IEEE802154_TRANSCEIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "ieee802154_frame.h" // From shoderico/ieee802154_frame
#include "esp_ieee802154.h"

/**
 * @brief Callback function type for received IEEE 802.15.4 frames.
 *
 * @param frame Parsed IEEE 802.15.4 frame.
 * @param frame_info Frame information (e.g., RSSI, LQI, channel).
 * @param user_data User-defined data passed to the callback.
 */
typedef void (*ieee802154_transceiver_rx_callback_t)(ieee802154_frame_t *frame,
                                                    esp_ieee802154_frame_info_t *frame_info,
                                                    void *user_data);

/**
 * @brief Initialize the IEEE 802.15.4 transceiver with a specified channel.
 *
 * @param channel Channel number (11-26) to set for the transceiver.
 * @note NVS must be initialized by the user before calling this function.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_init(uint8_t channel);

/**
 * @brief Deinitialize the IEEE 802.15.4 transceiver.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_deinit(void);

/**
 * @brief Set the callback function for received frames.
 *
 * @param callback Callback function to invoke on frame reception.
 * @param user_data User-defined data to pass to the callback.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_set_rx_callback(ieee802154_transceiver_rx_callback_t callback, void *user_data);


void ieee802154_transceiver_handle_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info);

/**
 * @brief Transmit an IEEE 802.15.4 frame on the current channel.
 *
 * @param frame Frame to transmit.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_transmit(const ieee802154_frame_t *frame);

/**
 * @brief Transmit an IEEE 802.15.4 frame on a specified channel.
 *
 * @param frame Frame to transmit.
 * @param channel Channel number (11-26) to use for transmission.
 * @note The channel is not restored after transmission; use ieee802154_transceiver_set_channel to restore it.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_transmit_channel(const ieee802154_frame_t *frame, uint8_t channel);

/**
 * @brief Set the IEEE 802.15.4 channel.
 *
 * @param channel Channel number (11-26).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ieee802154_transceiver_set_channel(uint8_t channel);

#endif // IEEE802154_TRANSCEIVER_H