# Zest Display LCD Demo
An example to demonstrate how to use the Zest_Display_LCD board.

## Requirements
### Hardware requirements
The following boards are required:
- *List Zest Display LCD Demo hardware requirements here*

### Software requirements
Zest Display LCD Demo makes use of the following libraries (automatically imported
by `mbed deploy` or `mbed import`):
- *List Zest Display LCD Demo software requirements here*

## Usage
To clone **and** deploy the project in one command, use `mbed import` and skip to the
target and toolchain definition:
```shell
mbed import https://gitlab.com/catie_6tron/zest-display-lcd-demo.git zest-display-lcd-demo
```

Alternatively:

- Clone to "zest-display-lcd-demo" and enter it:
  ```shell
  git clone https://gitlab.com/catie_6tron/zest-display-lcd-demo.git zest-display-lcd-demo
  cd zest-display-lcd-demo
  ```

- Deploy software requirements with:
  ```shell
  mbed deploy
  ```

- Set Mbed project root path:
  ```shell
  mbed config root .
  ```

Define your target (eg. `ZEST_CORE_STM32L4A6RG`) and toolchain:
```shell
mbed target ZEST_CORE_STM32L4A6RG
mbed toolchain GCC_ARM
```

Export to Eclipse IDE with:
```shell
mbed export -i eclipse_6tron
```

## Working from command line
Compile the project:
```shell
mbed compile
```

Program the target device (eg. `STM32L4A6RG` for the Zest_Core_STM32L4A6RG) with a J-Link
debug probe:
```shell
python dist/program.py STM32L4A6RG BUILD/ZEST_CORE_STM32L4A6RG/GCC_ARM/zest-display-lcd-demo.elf
```

Debug on the target device (eg. `STM32L4A6RG` for the Zest_Core_STM32L4A6RG) with a
J-Link debug probe.

- First, start the GDB server:
  ```shell
  JLinkGDBServer -device STM32L4A6RG
  ```

- Then, in another terminal, start the debugger:
  ```shell
  arm-none-eabi-gdb BUILD/ZEST_CORE_STM32L4A6RG/GCC_ARM/zest-display-lcd-demo.elf
  ```

*Note:* You may have to adjust your [GDB auto-loading safe path](https://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading-safe-path.html#Auto_002dloading-safe-path)
or disable it completely by adding a .gdbinit file in your $HOME folder containing:
```conf
set autoload safe-path /
```
