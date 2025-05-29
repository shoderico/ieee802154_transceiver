idf_component_register(
    SRCS "simple_transceiver.c"
    INCLUDE_DIRS "."
    REQUIRES ieee802154_transceiver nvs_flash
)