idf_component_register(
    SRCS "test_ieee802154_transceiver.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_common nvs_flash unity
        ieee802154_frame ieee802154_transceiver
    WHOLE_ARCHIVE
)
