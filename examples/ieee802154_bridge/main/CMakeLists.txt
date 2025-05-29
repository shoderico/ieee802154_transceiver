idf_component_register(
    SRCS "ieee802154_bridge.c"
    INCLUDE_DIRS "."
    REQUIRES ieee802154_transceiver nvs_flash esp_timer
)
