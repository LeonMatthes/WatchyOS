#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
CXXFLAGS += -std=gnu++17 -Wall -Wextra
COMPONENT_SRCDIRS := . ble screens
COMPONENT_PRIV_INCLUDEDIRS := .
