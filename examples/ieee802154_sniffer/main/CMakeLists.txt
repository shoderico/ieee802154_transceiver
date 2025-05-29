idf_component_register(
    SRCS "ieee802154_sniffer.c"
    INCLUDE_DIRS "."
    REQUIRES ieee802154_transceiver nvs_flash
)
