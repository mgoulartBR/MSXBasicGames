import re,sys
from pathlib import Path
src=Path(sys.argv[1]).read_text()
src=src.replace('#include "cart.c"','u8 cart_riding=255;u8 cart_cache_valid;static void CartStep(void){}static void CartContact(u16 o){(void)o;cart_riding=255;}\nvoid CartReset(void){}')
Path(sys.argv[2]).write_text(src)
