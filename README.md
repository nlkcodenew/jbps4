# jbps4 — ESP32-C3 PS4 9.00 Jailbreak Host

Firmware ESP32-C3 (Arduino + PlatformIO) đóng vai trò **captive portal + web server**,
phục vụ payload **PSFree-Lapse** cho **PS4 9.00** (WebKit exploit + Lapse kernel exploit).

Khi PS4 kết nối WiFi `jbps4` và mở browser, DNS wildcard + captive-portal redirects
sẽ dẫn PS4 tới `http://10.1.1.1/index.html` để chạy exploit chain và load **GoldHEN**.

## Thông số

| Mục | Giá trị |
|---|---|
| Target MCU | ESP32-C3 (board `esp32-c3-devkitm-1`) |
| Framework | Arduino (espressif32) |
| Filesystem | LittleFS |
| App partition | `0x10000` (1.25 MB) |
| LittleFS partition | `0x150000` (2.75 MB) |
| AP SSID | `jbps4` |
| AP Password | `jbps4000` |
| AP IP | `10.1.1.1/24` |
| DNS port | 53 (wildcard → `10.1.1.1`) |
| HTTP port | 80 |

## Cấu trúc repo

```
.
├── platformio.ini              # cấu hình PlatformIO, env=esp32c3
├── partitions.csv              # app0 + LittleFS layout
├── src/
│   └── main.cpp                # AP + DNS wildcard + captive portal + LittleFS serve
├── data/                       # LittleFS payload (LittleFS image)
│   ├── index.html              # entry: PSFree-Lapse Exploit For PS4 9.00
│   ├── about.html
│   ├── alert.mjs, cache.html, config.mjs, send.mjs
│   ├── lapse.mjs, psfree.mjs   # JS exploit chain
│   ├── module/                 # 9 module: chain, mem, view, utils, int64, ...
│   ├── rop/900.mjs             # ROP chain cho FW 9.00
│   ├── kpatch/900.elf          # kernel patch ELF
│   ├── goldhen.bin             # GoldHEN payload (290 KB)
│   ├── aio_patches.bin
│   └── fonts/LiberationMono-Regular.ttf
└── firmware/                   # prebuilt binaries (flash thẳng, không cần build)
    ├── firmware.bin            # application @ 0x10000 (824 KB)
    └── bootloader.bin          # bootloader @ 0x0000 (13 KB)
```

## Flash nhanh (không cần build)

Cắm ESP32-C3 vào `/dev/ttyACM0`, rồi:

```bash
# Qua PlatformIO
pio run -t upload

# Hoặc dùng esptool.py trực tiếp với file đã build sẵn
esptool.py --chip esp32-c3 \
  --port /dev/ttyACM0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash 0x0000 firmware/bootloader.bin \
             0x10000 firmware/firmware.bin \
             0x8000  partitions.csv
```

> Lưu ý: `data/` được build thành LittleFS image và flash riêng (offset `0x150000`).
> Nếu chỉ flash app + bootloader thì phải tự `pio run -t uploadfs` hoặc
> `pio run -t upload` (mặc định PlatformIO sẽ upload cả LittleFS).

## Build từ source

```bash
pio run                  # build app
pio run -t upload        # flash app + LittleFS data
pio run -t uploadfs      # chỉ flash LittleFS data
pio device monitor       # Serial 115200
```

## Cách dùng trên PS4

1. Cắm ESP32-C3 vào nguồn USB.
2. Trên PS4 (FW 9.00), vào **Settings → Network → Set Up Internet → Use Wi-Fi**,
   chọn `jbps4`, nhập password `jbps4000`.
3. PS4 sẽ mở captive portal tự động → exploit chain chạy → GoldHEN load xong
   sẽ hiện thông báo "GoldHEN" trên PS4. Hoặc nếu không mở thì dùng tay cầm vào www của ps4 rồi gõ url 10.1.1.1 là tự động hack
4. Nếu PS4 không tự mở portal, vào **Settings → User's Guide / User's Help**.
   Trên một số FW có thể cần **Settings → Network → Test Internet Connection**.

## Nguồn / Credits

- Exploit payload: [PSFree-Lapse](https://github.com/abc/psfree) (anonymous, AGPL-3.0).
- GoldHEN: [SiSTR0/GoldHEN](https://github.com/SiSTR0/GoldHEN).
- ESP32 captive portal / DNS wildcard pattern: triển khai riêng cho `jbps4`.

## License

Phần firmware ESP32-C3 (code trong `src/`, `platformio.ini`, `partitions.csv`) do tác giả
repo này viết, xem header từng file. Payload trong `data/` thuộc bản quyền các tác giả
tương ứng (xem `about.html`).
