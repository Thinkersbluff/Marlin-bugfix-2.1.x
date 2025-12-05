# Marlin Unit Tests

This directory contains native unit tests for Marlin firmware components that can be compiled and run on a host machine (Linux, macOS, or Windows with appropriate toolchain).

## Test Files

### test_queue.cpp
Tests for the G-code command queue ring buffer (`Marlin/src/gcode/queue.h`):
- Basic enqueue/dequeue operations
- FIFO ordering
- Ring buffer wraparound behavior
- Full and empty state detection
- Edge cases (empty queue operations, special characters)

**Tested components:**
- `GCodeQueue::RingBuffer::enqueue()`
- `GCodeQueue::RingBuffer::advance_r()`
- `GCodeQueue::RingBuffer::advance_w()`
- `GCodeQueue::RingBuffer::clear()`
- `GCodeQueue::RingBuffer::full()`
- `GCodeQueue::RingBuffer::empty()`

### test_m1125.cpp
Tests for M1125 pause/resume command filtering logic:
- Detection and filtering of pause-triggering commands (M600, M1125)
- Preservation of normal G-code commands
- Case-insensitive matching
- Handling of parameters, comments, and whitespace
- Edge cases (empty strings, null pointers, substring confusion)

**Tested logic:**
- `m1125_should_skip_saved_command()` - prevents saved M600/M1125 from being replayed after resume
- `m1125_command_matches()` - case-insensitive command name matching

**Context:** The M1125 pause handler preserves commands from the SD ring buffer during pause and restores them on resume. These tests verify that pause-triggering commands are correctly filtered to prevent infinite pause loops.

## Running Tests

### Local Execution (Linux/macOS or Windows with GCC toolchain)

Run all tests:
```bash
make unit-test-all-local
```

Or use PlatformIO directly:
```bash
pio test -e linux_native_test
```

Run a specific test:
```bash
pio test -e linux_native_test -f test_queue
pio test -e linux_native_test -f test_m1125
```

### First-time Windows (WSL) Quickstart

If you're on Windows we recommend using WSL (Windows Subsystem for Linux). The following steps are copy-paste friendly — run them inside your WSL distro (for example, open `wsl` or `wsl -d Ubuntu`). Do NOT paste the bash commands into PowerShell directly.

1) Change into the repository

```bash
cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x
```

2) Install system dependencies (requires `sudo` inside WSL)

sudo apt install -y build-essential git python3 python3-venv python3-pip cmake gcc g++
```

3) Create and activate a Python virtual environment (recommended)

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip setuptools wheel
pip install -U platformio
```

4) Verify `pio` is available and run a single focused test (faster)

```bash
pio --version
pio test -e linux_native_test -f test_queue -vvv | tee pio_test_queue.log

The repo also includes a full-suite runner for convenience:

- `scripts/run_test_full.sh` — activates the repo `.venv` and runs the full native test suite (`pio test -e linux_native_test -vvv`), saving `pio_test_full.log` into the repo `test/` directory.
- `run_test_full.ps1` — PowerShell wrapper that invokes the WSL full-runner from Windows.

Run the full suite from WSL (may take longer):

```bash
cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x
./scripts/run_test_full.sh
```

Or from Windows PowerShell:

```powershell
cd C:\Users\BadBa\Marlin-bugfix-2.1.x
.\run_test_full.ps1
```
```

5) Run the full native test suite when ready

```bash
pio test -e linux_native_test -vvv | tee pio_test_full.log
```

Notes:
- If you prefer system-wide installation instead of a venv: `pip3 install --user platformio` and add `~/.local/bin` to your PATH in `~/.profile`.
- If `pio` is not found after installing in the venv, ensure you ran `source .venv/bin/activate` in the same shell.


```powershell
wsl bash -lc "cat > /tmp/marlin_wsl_setup.sh << 'EOS'
set -e
cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x
sudo apt install -y build-essential git python3 python3-venv python3-pip cmake gcc g++
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip setuptools wheel
pip install -U platformio
bash /tmp/marlin_wsl_setup.sh
"
```

After that, run tests from PowerShell like this (runs inside WSL):

wsl bash -lc "cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x && source .venv/bin/activate && pio test -e linux_native_test -f test_queue -vvv | tee pio_test_queue.log"
```

### Troubleshooting (WSL / Windows)

- If `sudo` failed earlier inside PowerShell, it's because the commands were run in the Windows shell — they must run inside WSL.
- If `python3` is not found, install it inside WSL with `sudo apt install python3`.
- If `pio` installation warns about scripts installed to `~/.local/bin`, add the following to `~/.profile` (in WSL) and re-open your shell:

```bash
export PATH="$HOME/.local/bin:$PATH"

- If `CC is not recognized` on Windows: make sure you're running within WSL or have MSYS2/Mingw on PATH; see the Windows Requirements section above.

### Before closing WSL / reboot (short checklist)

If you plan to stop your machine or close WSL, follow these quick steps so you can resume testing without re-running setup:

- **Save in-progress changes:** commit or stash changes in the repo:

```bash
git status
git add -A && git commit -m "WIP: save progress" || echo "No changes to commit"
# or: git stash
```

- **Save test logs you want to keep:** copy logs into the repo (optional):

```bash
cp -v pio_test_queue.log test/ 2>/dev/null || true
cp -v pio_test_full.log test/ 2>/dev/null || true
```

- **Stop long-running processes** you started (optional):

```bash
ps aux | grep pio
# kill <pid>  # only if necessary
```

- **Deactivate virtualenv (optional):**

```bash
deactivate  # if you activated one
```

- **Exit WSL cleanly:**

```bash
exit
```

- **(Optional) Shutdown WSL from Windows** to stop the VM:

From an elevated PowerShell prompt on Windows:

```powershell
wsl --shutdown
```

What persists across reboots
- System packages installed with `apt` remain in your WSL distro.
- The project files under `/mnt/c/...` remain on the Windows drive.
- The Python virtual environment (`.venv`) in the repo remains and will be usable after reboot.

How to resume testing after reboot

Open WSL and run the following from the repo directory:

```bash
cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x
source .venv/bin/activate
command -v pio || echo "pio not on PATH; activate venv or reinstall"
pio test -e linux_native_test -f test_queue -vvv | tee pio_test_queue.log
```

If `pio` is not found but you used `pip3 install --user platformio`, ensure `~/.local/bin` is on your PATH (see the Troubleshooting section above).

Tests run automatically via GitHub Actions on pull requests and pushes to `bugfix-2.1.x` branch.

See `.github/workflows/ci-unit-tests.yml` for CI configuration.

Native tests require GCC. Options:
1. **WSL (Windows Subsystem for Linux)** - recommended
2. **MSYS2** with MinGW-w64:
   ```bash
   pacman -S mingw-w64-x86_64-toolchain
   ```
   Add `C:\msys64\mingw64\bin` to PATH
3. **Docker** - use the included Dockerfile:
   ```bash
   make unit-test-all-local-docker
   ```

## Test Framework

Tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework (v2.5.2+).

### Writing New Tests

1. Create a new file in `test/` directory: `test_<component>.cpp`

2. Include the test framework and component headers:
```cpp
#include "../test/unit_tests.h"
#include "src/<path>/<header>.h"
```
MARLIN_TEST(suite_name, test_name) {
  // Setup
  int result = function_under_test(42);
  
  // Assertions
  TEST_ASSERT_TRUE(condition);
  TEST_ASSERT_FALSE(other_condition);
  TEST_ASSERT_EQUAL_STRING("expected", actual_string);
}
```

4. Run tests to verify:
```bash
pio test -e linux_native_test -f test_<component>
```

### Common Unity Assertions

- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`
- `TEST_ASSERT_EQUAL(expected, actual)`
- `TEST_ASSERT_EQUAL_STRING(expected, actual)`
- `TEST_ASSERT_NULL(pointer)`
- `TEST_ASSERT_NOT_NULL(pointer)`
- `TEST_ASSERT_EQUAL_FLOAT(expected, actual, tolerance)`

See [Unity documentation](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md) for complete reference.

## Test Stubs

`test_stubs.cpp` provides minimal stub implementations for hardware-dependent symbols needed by tests but not directly tested. Stubs are only compiled for `linux_native_test` environment.

Add new stubs here when tests require hardware-dependent symbols (UI, SD card, thermal management, etc.).

## Coverage Targets

Priority areas for test coverage:
- [ ] G-code parsing and command handling
- [x] Queue/ring buffer operations
- [x] M1125 pause/resume filtering
- [ ] Host notification caching and suppression
- [ ] M73 progress reporting
- [ ] Temperature management logic
- [ ] Motion planning helpers
- [ ] SD card command processing

## Troubleshooting

**"CC is not recognized" error on Windows:**
- Install MSYS2/MinGW or use Docker/WSL
- Or skip local testing and rely on CI

**Tests fail to compile:**
- Check for missing stubs in `test_stubs.cpp`
- Verify includes are correct in test files
- Use `-vvv` flag for verbose output: `pio test -e linux_native_test -vvv`

**Tests compile but fail:**
- Check assertion messages for details
- Add debug output with `SERIAL_ECHOLNPGM()` (may not work in native tests)
- Use Unity's `TEST_MESSAGE()` for debugging
