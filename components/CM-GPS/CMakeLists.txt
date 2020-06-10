
set(COMPONENT_SRCS "nmea_parser.c")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mconfig)

register_component()
