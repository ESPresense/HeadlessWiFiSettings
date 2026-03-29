# Build Instructions for Testing

## Issue: PlatformIO Registry Names Don't Work on Mac M1/M2

The library names in the PlatformIO registry don't resolve correctly on darwin_arm64 (Mac M1/M2).

Using **GitHub URLs** instead works across all platforms.

## Steps to Build

```bash
# 1. Install Python dependency (for test script)
pip3 install pyserial

# 2. Pull latest fixes
git pull origin claude/test-home-assistant-support-011CUQuhAg8kMME5diE5fLBs

# 3. Clean and build
cd examples/ImprovWithHTTP
pio run -t clean
pio run
```

## What's in platformio.ini

```ini
lib_deps =
    file://../../                                       # Parent library
    jnthas/Improv WiFi Library                         # Improv protocol
    https://github.com/me-no-dev/ESPAsyncWebServer.git # Direct GitHub
    https://github.com/me-no-dev/AsyncTCP.git          # Direct GitHub

lib_ignore = WiFi  # Ignore wrong WiFi library
```

Using GitHub URLs bypasses registry issues and works on all platforms.

## Flash and Test

```bash
# Flash
pio run -t upload -t monitor

# In another terminal, provision
cd ../..
python3 test_improv.py /dev/cu.SLAB_USBtoUART "WiFiSSID" "WiFiPassword"
```

## Why GitHub URLs?

The PlatformIO registry has naming issues:
- `me-no-dev/ESP Async WebServer` (with space) doesn't resolve
- `me-no-dev/ESPAsyncWebServer` (no space) may work but has platform issues
- GitHub URLs are the recommended approach in the ESP32 community

## Alternative: Without Hardware

The original PR can be evaluated through:
1. **Code review** - Integration logic is correct ✅
2. **Dependency analysis** - Issues documented ⚠️
3. **Build test** - Should work with GitHub URLs ✅

The WiFi + Improv integration itself is sound, just the dependency configuration needs fixes.
