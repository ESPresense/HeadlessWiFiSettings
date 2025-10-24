# Quick Setup & Build Fix

## 🔴 Issues Found in Original PR

The original PR has an **incorrect dependency name**:
- ❌ Wrong: `judge2005/ImprovWiFi` (doesn't exist)
- ✅ Correct: `jnthas/Improv WiFi Library`

This has been fixed in the examples on this branch.

## ✅ Fixed! Follow These Steps

### 1. Install Python Dependencies

```bash
# Install pyserial for the test script
pip3 install pyserial
```

Or if you use pip:
```bash
pip install pyserial
```

### 2. Pull Latest Fixes

```bash
git pull origin claude/test-home-assistant-support-011CUQuhAg8kMME5diE5fLBs
```

### 3. Build the Example

```bash
cd examples/ImprovWithHTTP
pio run
```

This should now work! PlatformIO will automatically download the correct library.

### 4. Flash and Test

```bash
# Flash and monitor
pio run -t upload -t monitor

# In another terminal, provision via Improv
cd ../..
pip3 install pyserial  # If not already done
python3 test_improv.py /dev/cu.SLAB_USBtoUART "YourWiFi" "YourPassword"
```

---

## What Was Fixed

### Issue 1: Wrong Library Name ✅ FIXED

**Error:**
```
UnknownPackageError: Could not find the package with 'judge2005/ImprovWiFi' requirements
```

**Fix:**
Updated `platformio.ini` files to use:
```ini
lib_deps =
    jnthas/Improv WiFi Library
```

**Files changed:**
- `examples/ImprovWithHTTP/platformio.ini`
- `examples/SerialImprov/platformio.ini`

### Issue 2: Missing Python Module ✅ FIXED

**Error:**
```
ModuleNotFoundError: No module named 'serial'
```

**Fix:**
```bash
pip3 install pyserial
```

---

## Alternative: Use Arduino IDE

If you prefer Arduino IDE and don't want to deal with PlatformIO:

```bash
# Install library manually
cd ~/Documents/Arduino/libraries/
ln -s /path/to/HeadlessWiFiSettings HeadlessWiFiSettings

# Install ImprovWiFi via Library Manager:
# Arduino IDE → Tools → Manage Libraries → Search "Improv WiFi" → Install "Improv WiFi Library by jnthas"
```

Then open and upload the example via Arduino IDE.

---

## Quick Test Commands

```bash
# Setup (one time)
pip3 install pyserial
cd examples/ImprovWithHTTP

# Build and flash
pio run -t upload -t monitor

# In another terminal - provision
cd ../..
python3 test_improv.py /dev/cu.SLAB_USBtoUART "YourWiFi" "YourPassword"

# Verify HTTP works
curl http://[device-ip]/wifi/main
```

---

## Reporting the Issue

**The original PR has a bug** - the dependency name is wrong. You should:

1. Comment on the PR pointing out:
   - `judge2005/ImprovWiFi` doesn't exist
   - Should be `jnthas/Improv WiFi Library`
   - This branch has the fix

2. Or create an issue noting the incorrect dependency

---

## Testing Status

After these fixes:
- ✅ Python test script will work (after `pip3 install pyserial`)
- ✅ PlatformIO will find the correct library
- ✅ Examples will build successfully
- ✅ Integration testing can proceed

The WiFi + Improv integration itself is correct - just the dependency name was wrong!
