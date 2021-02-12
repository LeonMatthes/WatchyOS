# BLE
## Basic protocol
Watchy may wake up at any time (typically once per minute + on user interaction) and start advertising a GATT Server via BLE.

It exposes a GATT characteristic for it's current State.
This state includes at least 2 values:
- Unsigned Byte: Connection State (i.e. Reboot, Fast Update, Extended connection)
- Unsigned Byte: The current ID of Watchy's State
    - The ID is only set by the GATT client (i.e. phone). It can be used to make sure Watchy is actually in the state expected by the Client. It is persisted between deep sleep.

After connecting, the Client should first read this state, determine all necessary updates and then write back to the characteristic with:
- Unsigned Byte: 0xFF (Connection State, indicating Watchy should disconnect)
    - 0xFF should only be written if the connection is supposed to be terminated - other values will be ignored by Watchy
    - Use this way of terminating the connection, instead of disconnecting from Android. Android takes a lot longer to disconnect, keeping Watchy awake unnecessarily and wasting battery life.
- Unsigned Byte: The new ID of Watchy's state - may be the same if no changes were written

Especially when Watchy is indicating a Fast Update, keep Read/Write Operations to a minimum, and disconnect as quickly as possible to save battery life.
