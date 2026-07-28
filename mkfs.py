import struct, os
BASE = 256 #the FS starts at this disk sector
SECTOR = 512
MAGIC = 0x53465331 #'SFS1'

files = {
    "/etc/passwd": b"root:x:0:0:root:/root:/bin/sh\nadmin:x:1000:1000:Administrator:/home/admin:/bin/sh\n",
    "/etc/motd": b"ASUS RT-AC66U firmware 3.0.0.4_374 -- authorized access only\n",
    "/www/index.html": b"<html><body><h1>Router Admin</h1><form action=/login></form></body></html>\n",
    "/readme.txt": b"simplefs works: this file was read off the disk through the VFS.\n",
}

dir_sec = bytearray(SECTOR)
struct.pack_into("<II", dir_sec, 0, MAGIC, len(files))
off = 8
data = bytearray()
cur = BASE + 1
for name, content in files.items():
    nb = name.encode()[:55].ljust(56, b"\x00")
    struct.pack_into("<56sII", dir_sec, off, nb, cur, len(content))
    off += 64
    padded = content + b"\x00" * ((-len(content)) % SECTOR)
    data += padded
    cur += len(padded) // SECTOR

os.makedirs("build", exist_ok=True)
open("build/fs.img", "wb").write(bytes(dir_sec) + bytes(data))
print("fs.img:", len(dir_sec)+len(data), "bytes,", len(files), "files")