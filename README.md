# Debugprobe

Firmware source for the Raspberry Pi Debug Probe SWD/UART accessory. Can also be run on a Raspberry Pi Pico or Pico 2.

[Raspberry Pi Debug Probe product page](https://www.raspberrypi.com/products/debug-probe/)

[Raspberry Pi Pico product page](https://www.raspberrypi.com/products/raspberry-pi-pico/)

[Raspberry Pi Pico 2 product page](https://www.raspberrypi.com/products/raspberry-pi-pico-2/)

## Documentation

Debug Probe documentation can be found at the [Raspberry Pi documentation](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html#about-the-debug-probe) and in the [Getting Started with Pico PDF](https://pip.raspberrypi.com/documents/RP-008276-DS).


# Quick Start

This repository supports two Raspberry Pi Pico CMSIS-DAP debug probe firmware variants for the longevity testing system.

| Probe role | USB VID:PID | Purpose |
|---|---|---|
| Sensor | `2e8a:000c` | Debugging and flashing sensor targets |
| Gateway | `2e8a:000d` | Debugging and flashing gateway targets |

The USB product ID identifies the probe role, allowing the longevity tester to select the appropriate probe automatically.

## Build environment

Enter the Nix development environment:

```bash
nix develop
```

The environment provides the Pico SDK, ARM compiler, CMake, `picotool`, and `just`.

No separate local Pico SDK installation is required.

## Build firmware

Build the sensor probe firmware:

```bash
just build-sensor
```

Build the gateway probe firmware:

```bash
just build-gateway
```

The builds use separate directories to avoid configuration conflicts:

| Variant | Build directory | UF2 firmware |
|---|---|---|
| Sensor | `build-sensor/` | `build-sensor/debugprobe_on_pico.uf2` |
| Gateway | `build-gateway/` | `build-gateway/debugprobe_on_pico.uf2` |

## Install and verify

Install the corresponding UF2 firmware onto each Pico using its UF2 bootloader.

After reconnecting the probes, verify USB identification:

```bash
probe-rs list
```

Expected identifiers:

- Sensor probe: `2e8a:000c`
- Gateway probe: `2e8a:000d`

Each probe also has a unique USB serial number, allowing individual probes to be selected for flashing.

## Integration with longevity testing

The `cmsis_longevity_testing` remote flashing pipeline uses the USB product ID to distinguish sensor and gateway probes.

- Sensor probes (`000c`) are used with nRF54L15 targets.
- Gateway probes (`000d`) are used with nRF52840 targets.

Both variants use CMSIS-DAP over SWD with `probe-rs`.

**Note:** The gateway USB PID `0x000D` is currently a project-specific provisional identifier and requires allocation approval before production use.

---

## Building for the Pico 1

If you want to create the version that runs on the Pico, then you need to invoke `cmake` in the sequence above with the `DEBUG_ON_PICO=ON` option:
```bash
cmake -DDEBUG_ON_PICO=ON ..
```

This will build with the configuration for the Pico and call the output program `debugprobe_on_pico.uf2`, as opposed to `debugprobe.uf2` for the accessory hardware.

Note that if you first ran through the whole sequence to compile for the Debug Probe, then you don't need to start back at the top. You can just go back to the `cmake` step and start from there.

## Building for the Pico 2

If using an existing debugprobe clone:
- You must completely regenerate your build directory, or use a different one.
- You must also sync and update submodules.
- `PICO_SDK_PATH` must point to a version 2.0.0 or greater install.

```bash
git submodule sync
git submodule update --init --recursive
mkdir build-pico2
cd build-pico2
cmake -DDEBUG_ON_PICO=1 -DPICO_BOARD=pico2 ../
```

This will build with the configuration for the Pico 2 and call the output program `debugprobe_on_pico2.uf2`.

## AutoBaud Mode

Mode which automatically detects and sets the UART baud rate as data arrives.

To enable AutoBaud, configure the USB CDC port to the following custom baud rate:
```
9728 (0x2600)
```
> **Note:** Some Linux serial tools cannot set custom baud values. PuTTY on Windows and any terminal that supports arbitrary baud rates works.

Changing the baud rate to any other value disables AutoBaud.

