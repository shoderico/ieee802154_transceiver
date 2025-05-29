# IEEE 802.15.4 Transceiver Component

This ESP-IDF component provides a simple API for IEEE 802.15.4 communication using the ESP32's built-in IEEE 802.15.4 radio. It supports initializing the transceiver, transmitting and receiving frames, setting receive callbacks, and dynamically switching channels (11-26) in promiscuous mode.

## Features

- Initialize the IEEE 802.15.4 radio in promiscuous mode for flexible frame capture.
- Transmit and receive IEEE 802.15.4 frames with support for custom frame structures.
- Register callbacks to process received frames with RSSI and LQI information.
- Dynamically switch channels (11-26) without reinitializing the radio.
- Built on top of ESP-IDF's `esp_ieee802154` component and `shoderico/ieee802154_frame` for frame handling.

## Requirements

- ESP-IDF v5.0 or later (tested with v5.4.1).
- ESP32 with IEEE 802.15.4 support (e.g., ESP32-C6, ESP32-H2).
- Non-Volatile Storage (NVS) initialized before using the component.
- FreeRTOS for task and message buffer management.

## Installation

This component is available on the [ESP Component Registry](https://components.espressif.com/). To add it to your ESP-IDF project, run:

```bash
idf.py add-dependency "shoderico/ieee802154_transceiver^1.0.0"
```

Alternatively, clone the component into your project's `components` directory and update your `CMakeLists.txt` to include it.

## Usage

1. **Initialize NVS**:
   Initialize NVS before using the component:
   ```c
   esp_err_t ret = nvs_flash_init();
   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
       ESP_ERROR_CHECK(nvs_flash_erase());
       ret = nvs_flash_init();
   }
   ESP_ERROR_CHECK(ret);
   ```

2. **Initialize the Transceiver**:
   Set up the transceiver on a specific channel (11-26):
   ```c
   esp_err_t ret = ieee802154_transceiver_init(11);
   if (ret != ESP_OK) {
       // Handle error
   }
   ```

3. **Set a Receive Callback**:
   Define a callback to process received frames:
   ```c
   void rx_callback(ieee802154_frame_t *frame, esp_ieee802154_frame_info_t *frame_info, void *user_data) {
       printf("Received frame: payloadLen=%d, RSSI=%d, LQI=%d\n", frame->payloadLen, frame_info->rssi, frame_info->lqi);
   }

   ieee802154_transceiver_set_rx_callback(rx_callback, NULL);
   ```

   When a frame is received, the ESP-IDF's `esp_ieee802154_receive_done` interrupt function is triggered. By calling `ieee802154_transceiver_handle_receive_done` from this function, the registered callback (e.g., `rx_callback`) is invoked:
   ```c
   void esp_ieee802154_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info) {
       ieee802154_transceiver_handle_receive_done(frame, frame_info);
   }
   ```

4. **Transmit a Frame**:
   Create and send an IEEE 802.15.4 frame:
   ```c
   uint8_t payload[] = "Hello, IEEE 802.15.4!";
   ieee802154_frame_t frame = {
      .fcf = {
            .frameType = IEEE802154_FRAME_TYPE_DATA, // 001
            .securityEnabled = 0,                    // 0
            .framePending = 0,                       // 0
            .ackRequest = 1,                         // 1
            .panIdCompression = 1,                   // 1
            .reserved = 0,                           // 0
            .sequenceNumberSuppression = 0,          // 0
            .informationElementsPresent = 0,         // 0
            .destAddrMode = IEEE802154_ADDR_MODE_SHORT, // 10
            .frameVersion = IEEE802154_VERSION_2006,    // 01
            .srcAddrMode = IEEE802154_ADDR_MODE_SHORT   // 10
      },
      .sequenceNumber = 0x02,
      .destPanId = 0x1234,
      .destAddress = {0xFF, 0xFF}, // Broadcast
      .srcPanId = 0x1234,
      .srcAddress = {0x9A, 0xBC},
      .payloadLen = strlen((char *)payload),
      .payload = payload
   }

   ieee802154_transceiver_transmit(&frame);
   ```

5. **Deinitialize**:
   Clean up resources when done:
   ```c
   ieee802154_transceiver_deinit();
   ```

## Examples

1. **simple_transceiver**

The `examples/simple_transceiver` directory includes a sample project that demonstrates:
- Initializing the transceiver on channel 11.
- Logging received frames via a callback.
- Periodically transmitting a test frame every 5 seconds.

To build and run the example:
```bash
cd examples/simple_transceiver
idf.py set-target esp32c6
idf.py build flash monitor
```

2. **ieee802154_sniffer**

The `examples/ieee802154_sniffer` directory includes another project that demonstrates:
- Initializing the rx transceiver on channel 11.
- Tracing the binary dump of received frames

To build and run the example:
```bash
cd examples/ieee802154_sniffer
idf.py set-target esp32c6
idf.py build flash monitor
```

3. **ieee802154_bridge**

The `examples/ieee802154_bridge` directory includes another project that demonstrates:
- Initializing the rx transceiver on channel 11.
- Once received a frame, transmitting the same frame to another channel 13.
- Measuring performance and trace average time from receiving to transmitting.

To build and run the example:
```bash
cd examples/ieee802154_bridge
idf.py set-target esp32c6
idf.py build flash monitor
```

## Testing

The `test` directory contains Unity-based unit tests to verify the component's functionality, including:
- Transceiver initialization with valid and invalid channels.
- Channel switching.
- Receive callback registration.

Each test case explicitly initializes and deinitializes the transceiver to ensure resource cleanup. To run the tests:
```bash
cd test_runner
idf.py set-target esp32c6
idf.py build flash monitor
```

## Dependencies

- `shoderico/ieee802154_frame`: Handles IEEE 802.15.4 frame parsing and building.
- ESP-IDF's `esp_ieee802154`: Provides low-level radio control.
- FreeRTOS: Manages tasks and message buffers.
- NVS: Stores radio configuration.

## License

Licensed under the [MIT](LICENSE).

## Contributing

Contributions are welcome! Submit issues or pull requests to the repository at [your repository URL].

## Troubleshooting

- **Initialization Fails**: Ensure NVS is initialized and the channel is between 11 and 26.
- **No Frames Received**: Verify the radio is in promiscuous mode and the channel matches the sender.
- **Resource Errors**: Call `ieee802154_transceiver_deinit` after use to free resources.