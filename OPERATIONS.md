# Operations

How to build, test, flash, run and release the firmware. Commands are for macOS; Linux is the same apart from the package manager.

## Contents

1. [Install the tools](#1-install-the-tools)
2. [Run the unit tests](#2-run-the-unit-tests)
3. [Configure Wi-Fi and server](#3-configure-wi-fi-and-server)
4. [Build, flash and watch the log](#4-build-flash-and-watch-the-log)
5. [First run on the hardware](#5-first-run-on-the-hardware)
6. [Putting the repository on GitHub](#6-putting-the-repository-on-github)
7. [Releasing a version](#7-releasing-a-version)
8. [Updating a pinned dependency](#8-updating-a-pinned-dependency)
9. [Troubleshooting](#9-troubleshooting)

## 1. Install the tools

Only PlatformIO is required to build and flash. It needs Python 3.10 to 3.14 (`python3 --version` shows yours). Its installer puts everything into your home folder and needs no administrator rights:

```sh
cd ~
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
pio --version
```

The `echo` line adds PlatformIO's folder to the `PATH`, the list of places the shell searches for commands, so that `pio` is found in every new Terminal window. `get-platformio.py` can be deleted afterwards.

If macOS says `python3` or `git` needs the "command line developer tools", accept the installation (or run `xcode-select --install`). Those tools come from Apple and also contain the C++ compiler used for the unit tests.

If you use [Homebrew](https://brew.sh), this does the same and adds the optional tools: `brew install platformio cmake clang-format`.

| Tool | Needed | Used for |
| --- | --- | --- |
| PlatformIO Core (`pio`) | yes | Building and flashing the firmware. Downloads the compiler and libraries on first use. |
| git | yes | PlatformIO fetches the display library with it; the firmware version comes from it. |
| C++ compiler (`c++`) | for unit tests | Part of Apple's command line developer tools. |
| CMake | optional | The standard way to build the unit tests. Section 2 also shows a way without it. |
| clang-format | optional | Formatting the code, only needed when you change it. |

If you prefer an editor: install VS Code with the PlatformIO IDE extension and open the **`firmware/`** folder (not the repository root), because that is where `platformio.ini` is.

The Arduino IDE is not used for this firmware. It remains handy for small one-off test sketches.

## 2. Run the unit tests

No board needed. From the repository root:

```sh
cmake -S firmware/tests/host -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

To see each test by name, run the program directly: `./build/host-tests/host_tests`.

Without CMake, one compiler command builds the same tests (without the extra warnings and sanitizers):

```sh
c++ -std=c++17 -Ifirmware/src -Ifirmware/tests/host \
    firmware/src/pure/*.cpp firmware/src/app/*.cpp firmware/tests/host/*.cpp \
    -o /tmp/host_tests && /tmp/host_tests
```

The tests are built with sanitizers (extra run-time checks for memory errors). If your compiler does not support them, add `-DEPB_SANITIZE=OFF` to the first command.

## 3. Configure Wi-Fi and server

The firmware needs three settings: Wi-Fi name, Wi-Fi password and the address of the server. Until proper setup exists they are compiled in from a file that git ignores:

```sh
cd firmware
cp include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h`. On its next start the board copies the values into its settings storage.

- `secrets.h` is in `.gitignore`. Check with `git status` that it never shows up.
- A firmware file built with `secrets.h` contains your Wi-Fi password. Do not share such a `.bin`.
- Without `secrets.h` the firmware builds fine and shows "Setup needed" on the panel.

## 4. Build, flash and watch the log

All commands from the `firmware/` folder.

| Goal | Command |
| --- | --- |
| Build | `pio run` |
| Build and flash | `pio run -t upload` |
| Watch the serial log | `pio device monitor` |
| Flash, then watch | `pio run -t upload -t monitor` |
| Debug build (waits 2 s at each wake so the log is complete) | `pio run -e xiao-epaper-debug -t upload -t monitor` |
| Erase everything, including stored settings | `pio run -t erase` |
| Delete build output | `pio run -t clean` |

The first build downloads the toolchain and takes several minutes. Later builds take seconds.

**Upload tips**

- The right serial port is the one that appears only when the board is plugged in. PlatformIO usually finds it by itself.
- If no port appears, or the upload cannot connect: hold the **BOOT** button while plugging in the USB cable. This is also the way in when the firmware is asleep, because a sleeping board has no USB connection.
- "Hard resetting via RTS pin" is the normal last line of a successful upload.
- Use a USB-C cable that carries data. Many charge-only cables look identical.

**Reading the log**

Each line starts with the milliseconds since the wake began, so the log doubles as a timing profile:

```
[  2012 ms] e-paperBoard firmware v0.1.0
[  2040 ms] wake #1: reason=boot screen=home battery=3940 mV (68%)
[  3310 ms] wifi: connected, ip=192.168.1.57 rssi=-58 dBm
[  7950 ms] drew screen 'home'
[  7951 ms] sleeping for 1800 s
```

(Illustrative: the numbers will differ.) The board disconnects from USB when it sleeps and reconnects when it wakes; the serial monitor reconnects by itself.

## 5. First run on the hardware

A sensible order for the first session with a new board, or after a change to the hardware layer:

1. Run the unit tests (section 2).
2. Start the test server on a computer in the same Wi-Fi network: `python3 tools/test_server.py` (from the repository root). It prints its address. If macOS asks whether Python may accept incoming connections, allow it.
3. Put that address into `secrets.h` as `EPB_SERVER_URL`.
4. Flash the debug build and watch the log.

What to check, in order:

| Check | Expected |
| --- | --- |
| Boot | A version line and a `wake #1` line with a plausible battery voltage. |
| Wi-Fi | `wifi: connected`. |
| Download and draw | The test image on the panel: black frame, a square in the top-left corner, one square in the middle. Then `sleeping for 120 s`. |
| Timer wake | About two minutes later a new `wake` line with `reason=timer`, then `screen unchanged (304)` and no flash on the panel. |
| Buttons | Each key wakes the board; the log shows `reason=prev`, `home` or `next`. Left is home, middle is next, right is prev. The number of squares on the panel changes with the screen. |
| Fast reconnect | From the second wake on: `wifi: fast connect on channel N`. |
| Failure handling | Stop the test server, press a key: `cycle failed`, the old image stays, retry in 60 s, then 120 s. |
| On battery | Set the on-off switch to on, unplug USB and press a key: the board must behave the same with no computer attached. |
| No settings | Erase the board, flash a build without `secrets.h`: "Setup needed" appears once. |

Write down what does not match. Those findings are the work list for v0.1.

## 6. Putting the repository on GitHub

```sh
# 1. First commit, locally.
git add -A
git status                      # read the list: secrets.h and .pio/ must NOT be in it
git commit -m "feat: firmware skeleton"

# 2. Connect to the GitHub repository.
git remote add origin git@github.com:<user>/e-paperBoard.git
git fetch origin

# 3. Only if the GitHub repository already has commits (a README or license
#    created on the website): merge them in.
git merge origin/main --allow-unrelated-histories

# 4. Push and remember the upstream branch.
git push -u origin main
```

`--allow-unrelated-histories` is needed in step 3 because the two repositories were started separately and share no common commit. Git refuses to merge such histories unless told it is intended.

After the first push, check the **Actions** tab on GitHub: the CI workflow runs for the first time there.

## 7. Releasing a version

Versions follow [Semantic Versioning](https://semver.org): `vMAJOR.MINOR.PATCH`. "v0.1" is the tag `v0.1.0`.

1. Everything for the release is merged and CI is green.
2. The first-run checklist (section 5) passes on the board.
3. Update `CHANGELOG.md`: rename "Unreleased" to the version and date, start a new empty "Unreleased".
4. Commit.
5. Tag and push:

   ```sh
   git tag -a v0.1.0 -m "v0.1.0"
   git push origin main --follow-tags
   ```

The firmware picks its version up from the tag: a build made exactly at the tag reports `v0.1.0`; three commits later it reports `v0.1.0-3-g<commit>`.

## 8. Updating a pinned dependency

All versions are fixed in `firmware/platformio.ini`. To update one:

1. Change the version (the platform URL or the library commit hash).
2. `pio run -t clean`, then `pio run`.
3. Re-run the first-run checklist on the board, at least the display and sleep parts.
4. Note the update in `CHANGELOG.md`.

## 9. Troubleshooting

| Symptom | Likely cause and fix |
| --- | --- |
| Upload tries a port that is not the board (for example a Bluetooth device such as `/dev/cu.SomeHeadphones`) and ends with "No serial data received" | PlatformIO found no USB port for the board and fell back to another serial port. Check with `ls /dev/cu.usbmodem*`. If nothing is listed, hold BOOT while plugging in. Then name the port yourself: `pio run -t upload --upload-port /dev/cu.usbmodem*`. |
| Upload succeeds after using BOOT, but the board does nothing | A board put into download mode with the BOOT button stays there. Press the reset button once (or unplug and replug) to start the firmware. |
| Everything works on USB, but the board is dead once the cable is unplugged | The on-off switch is off. It connects the battery; on USB the board runs without it. |
| No serial port, upload cannot connect | The board is asleep or the cable is charge-only. Hold BOOT while plugging in; try another cable. |
| Log starts in the middle or is empty | USB reconnects after each wake and early lines are lost. Use the `xiao-epaper-debug` build. |
| Panel stays blank after "drew screen" | Ribbon cable not seated. Power off before touching it. |
| Link error `undefined reference to EPaper::...` | The display library was built without `driver.h`. `-I include` must be in `build_flags` in `platformio.ini`. |
| "Setup needed" on the panel | No settings stored. Create `secrets.h` (section 3) and flash again. |
| `wifi: could not join` | Wrong name or password, or a 5 GHz-only network. The ESP32 only speaks 2.4 GHz. |
| `http: body is -1 bytes` | The server sent no `Content-Length`. It must (see the server contract). |
| `http: request failed (connection refused)` | Wrong address or port, the server is not running, or the computer's firewall blocks it. |
| Board wakes immediately after sleeping, over and over | A key is stuck or its pin is floating. Check the log's `reason=`; see the pull-up notes in `hal/power.cpp`. |
| `AWAKE LIMIT reached` in the log | Something hung for 90 s. The lines before it show where. |
| `ERROR: Python version must be ...` | The ESP32 platform supports Python 3.10 to 3.14. Install one of those and run the PlatformIO installer again with it. |
| The same Python error, but only at "Looking for upload port" | An older ESP32 platform release is still installed. PlatformIO loads every installed platform while it searches for the port, and the old one rejects your Python. Look in `~/.platformio/platforms/` and delete the folder of the old release. |
| Build fails with "program size is greater than maximum" | The firmware outgrew its 3 MB slot (`partitions.csv`). |
| Unit tests fail to build with sanitizer errors | Configure with `-DEPB_SANITIZE=OFF`. |
| Unit tests fail to build because of a warning | Your compiler knows a warning this code has not met. Configure with `-DEPB_WERROR=OFF` to run the tests, and report the warning. |
