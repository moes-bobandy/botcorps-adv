/* LOCKED clip blob (tools/lock_enc). Path is relative to the project dir,
 * where PlatformIO runs the assembler. Lives in flash rodata. */
    .section .rodata.lk_blob
    .global lk_blob
    .balign 4
lk_blob:
    .incbin "src/lock/lock_blob.bin"
    .global lk_blob_end
lk_blob_end:
