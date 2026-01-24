# ST77916 QSPI Display with LVGL 8 Gauge

A vehicle gauge implementation using the ST77916 360x360 round display with QSPI interface and LVGL 8 on ESP32-S3.

## Hardware

- **MCU:** ESP32-S3 DevKitC-1
- **Display:** ST77916 360x360 QSPI Round Display
- **Interface:** QSPI (Quad SPI)

### Pin Configuration

| Function | GPIO |
|----------|------|
| CS       | 5    |
| SCLK     | 6    |
| RST      | 7    |
| D0 (MOSI)| 8    |
| D1       | 9    |
| D2       | 10   |
| D3       | 11   |
| BL       | 4    |

## Software Requirements

- ESP-IDF v5.4.3 or later
- Python 3.8+

## Setup

1. **Clone the repository**
   ```bash
   git clone <your-repo-url>
   cd "ST77916 QSPI Test v1.0"
   ```

2. **Set up ESP-IDF environment**
   ```bash
   # Windows
   %IDF_PATH%\export.bat
   
   # Linux/Mac
   . $IDF_PATH/export.sh
   ```

3. **Install dependencies**
   ```bash
   idf.py reconfigure
   ```
   This will automatically download:
   - LVGL 8.3.0
   - espressif/esp_lcd_st77916

4. **Build the project**
   ```bash
   idf.py build
   ```

5. **Flash to ESP32-S3**
   ```bash
   idf.py -p COMx flash monitor
   ```
   (Replace `COMx` with your serial port)

## Project Structure

```
.
├── main/
│   ├── main.c              # Main application with gauge demo
│   ├── st77916_init.c      # ST77916 initialization library
│   ├── st77916_init.h      # ST77916 header file
│   ├── lv_conf.h          # LVGL 8 configuration
│   └── idf_component.yml   # Component dependencies
├── CMakeLists.txt
└── README.md
```

## Features

- ✅ Full QSPI initialization (193 manufacturer commands)
- ✅ LVGL 8 integration with meter widget
- ✅ Animated 270° speedometer gauge (0-120 MPH)
- ✅ Reusable ST77916 initialization library
- ✅ BGR color order support
- ✅ Double-buffered rendering

## Usage

The ST77916 initialization is encapsulated in a reusable library:

```c
#include "st77916_init.h"

esp_lcd_panel_handle_t panel_handle;
esp_err_t ret = st77916_init_panel(io_handle, rst_gpio, &panel_handle);
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

[Your chosen license - e.g., MIT, Apache 2.0]

## Credits

- Display initialization sequence from manufacturer's reference code
- Built with ESP-IDF and LVGL
