# Antho Foxo's Font Engine
An operating system and rendering api agnostic text rendering engine using signed distance fields, written in ansi c.

## Dependencies
* [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h)
* [stb_rect_pack.h](https://github.com/nothings/stb/blob/master/stb_rect_pack.h)

### Known bugs
* Each quad triggers a buffer flush
* Buffer size is calculated such that the entire buffer can never be used
* Text rendering does not handle tabs, carriage returns, or line feeds
