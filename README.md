# Gemini Browser for webOS

A Gemini protocol browser for webOS (HP TouchPad).

![Gemini](package/icon-256.png)

## Features

- Full Gemini protocol support with TLS 1.2
- Touch-friendly interface optimized for tablets
- Kinetic scrolling with momentum
- Navigation history with back button
- Bookmarks with add/delete functionality
- Unicode fallback for modern emoji (webOS only supports Unicode 6.0)
- Dark theme

## Prerequisites

### Palm webOS PDK

Install the Palm PDK to `/opt/PalmPDK`. This provides the SDL libraries and webOS-specific headers.

The PDK can be obtained from:
- HP's archived developer resources
- Various webOS preservation projects

### Cross-Compilation Toolchain

This project requires the Linaro GCC 4.8 toolchain for ARM cross-compilation.

1. Download the toolchain:
   ```bash
   mkdir -p toolchains
   cd toolchains
   wget https://releases.linaro.org/components/toolchain/binaries/4.8-2015.06/arm-linux-gnueabi/gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi.tar.xz
   tar xf gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi.tar.xz
   mv gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi gcc-linaro
   ```

2. Verify the toolchain:
   ```bash
   ./toolchains/gcc-linaro/bin/arm-linux-gnueabi-gcc --version
   ```

### Device Requirements

The target device (HP TouchPad) must have:
- webOS 3.0.5 or compatible
- `com.nizovn.openssl` package installed (provides OpenSSL 1.0.2p for TLS 1.2 support)
- Visit [http://docs.webosarchive.org](http://docs.webosarchive.org) to find related packages

Install the OpenSSL package via Preware or manually:
```bash
palm-install com.nizovn.openssl_1.0.2p_armv7.ipk
```

## Project Structure

```
gemini-browser/
├── src/                    # Source code
│   ├── main.c             # Entry point
│   ├── gemini.c/h         # Gemini protocol + TLS
│   ├── document.c/h       # Gemtext parser
│   ├── render.c/h         # SDL rendering
│   ├── ui.c/h             # User interface + event handling
│   ├── history.c/h        # Navigation history
│   ├── url.c/h            # URL parsing
│   └── unicode.c/h        # Unicode 6.0 fallback for emoji
├── package/               # webOS app package contents
│   ├── appinfo.json       # App metadata
│   ├── lib/               # Bundled OpenSSL libraries
│   ├── *.ttf              # DejaVu fonts
│   └── icon*.png          # App + button icons
├── libs/
│   └── openssl/           # OpenSSL 1.0.2p for linking
├── toolchains/            # (not in git) Cross-compiler
├── Makefile               # Build configuration
└── build-arm.sh           # Alternative build script
```

## Building

1. Ensure prerequisites are installed (PDK + toolchain)

2. Build the binary:
   ```bash
   make clean
   make
   ```

3. Package for webOS:
   ```bash
   cp gemini package/
   palm-package package
   ```

   This creates `org.webosarchive.geminibrowser_1.0.0_all.ipk`

## Deployment

### Install to connected device

```bash
palm-install org.webosarchive.geminibrowser_1.0.0_all.ipk
```

### Launch the app

```bash
palm-launch org.webosarchive.geminibrowser
```

Or tap the Gemini icon in the webOS launcher.

### View logs (for debugging)

```bash
novacom get file://media/internal/gemini-log.txt
```

## Development Notes

### OpenSSL Libraries

The project includes OpenSSL 1.0.2p libraries in two locations:
- `libs/openssl/` - Used during linking (with symlinks for sonames)
- `package/lib/` - Bundled with the app for runtime

These were extracted from the `com.nizovn.openssl` package to support TLS 1.2, which is required by most Gemini servers.

### Unicode Compatibility

webOS only supports Unicode 6.0, but modern Gemini content often includes newer emoji. The `unicode.c` module provides fallback mappings:
- Emoji faces → ASCII emoticons (😀 → `:)`)
- Hearts → `<3`
- Common symbols → text descriptions (`[rocket]`, `[book]`, etc.)

To add new fallbacks, edit the `fallbacks[]` table in `src/unicode.c`.

### Touch Input

The UI is designed for capacitive touchscreens:
- Tap to follow links or activate buttons
- Drag to scroll with momentum
- Address bar buttons: `<` (back), `+` (add bookmark), `*` (view bookmarks)

### Debugging

Debug logs are written to `/media/internal/gemini-log.txt` on the device. The logging can be controlled via the `log_msg()` function in `ui.c`.

## Credits

- Heavily based on Lagrange: https://github.com/skyjake/lagrange
- Built with SDL 1.2 and SDL_ttf
- Uses OpenSSL for TLS connections
- DejaVu fonts for text rendering
- Developed for the webOS Archive project
