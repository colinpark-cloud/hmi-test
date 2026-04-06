HMI Test Tool (Qt5) - initial

Overview
- Qt5 C++ application providing hardware test modules for i.MX8M Plus HMI boards.
- Modules: Display & Touch, GPIO/Buzzer, Serial (RS232/485), Storage (eMMC/SD), Performance (CPU/GPU)

Build (host cross-build)
- Requires Qt5 for target in Yocto image. To cross-build with your toolchain:

mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-aarch64-poky.cmake
make -j4

- The produced binary is bin/hmi-test (install via your Yocto recipe or copy to target)

Runtime requirements
- libgpiod (for GPIO) available on target
- Permissions to access /dev/gpiochip*, /dev/tty*, /dev/input/event*
- ALSA for audio tests
- GL drivers for GPU performance test

Yocto integration
- Add a recipe that installs the binary and required libraries (Qt5, libgpiod, alsa-lib).
- Example: create hmi-test_0.1.bb with SRC_URI pointing to this source and DEPENDS = "qtbase libgpiod alsa-lib"

Notes
- Device nodes are selectable at runtime. The UI defaults are common but may need adjustment per board (e.g., /dev/ttymxc2)
- Perf test CPU thread stop is simplified; for production implement proper thread control and monitoring.
