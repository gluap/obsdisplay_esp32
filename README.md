# OpenBikeSensor display

This repo contains firmware and code for a presentation display for the OpenBikeSensor.

The code is supposed to be built and flashed using [platformio](https://platformio.org/).

There are two case options:

## Lasercut Wood 
![]()

## Fully 3D-Printed
![](media/3DPrint_case.jpg)


## Build and flash

Open your shell in the root of this project (folder with ``platformio.ini``).

### Create virtual environment with platformio
- Create and activate a virtual environment:

   - macOS/Linux:
        ```bash
        python3 -m venv .venv
        source .venv/bin/activate
        ```

    - Windows (PowerShell):
        ```powershell
        py -m venv .venv
        .\.venv\Scripts\Activate.ps1
        ```

- Install PlatformIO inside the virtual environment:
  ```bash
  pip install --upgrade pip
  pip install platformio
  ```

### Build and flash the ESP32

> [!NOTE]
>
> - Activate the virtual environment in each new terminal session before running pio commands.
> - To deactivate when you’re done:
>   ```bash
>   deactivate
>   ```
>   or just close the terminal.

- Build the project:
  ```bash
  pio run
  ```
 
After connecting the ESP32 board to your computer:

- Flash to the connected board:
  ```bash
  pio run --target upload
  ```

- If needed for debugging or defvelopment: Monitor serial output:
  ```bash
  pio device monitor
  ```

