idf_component_register(
    SRCS
        "src/ieee802154_transceiver.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        ieee802154
        ieee802154_frame  # External component: shoderico/ieee802154_frame
)