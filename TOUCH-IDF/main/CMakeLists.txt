set(srcs "main.c"
    "lib/decode_png.c"
    "lib/decode_jpeg_v5.c"
    "lib/pngle.c"
    "lib/touch.c"
    "lib/lvgl_display.c"
    )

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")
