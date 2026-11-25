/* v1.2.21 size shrink: drop Serial log strings in project sources.
 * Forced in via build_src_flags only. Host sim is unchanged. No UI/pixel effect.
 * `Serial` is painted blue after expansion so the real HardwareSerial is used
 * only inside `if (0)`, and -Os deletes the calls and their string literals. */
#pragma once
#define Serial if (0) Serial
