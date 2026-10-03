## Deadline Trophy Smiuluator
To simulate the [Demoparty Berlin](https://www.demoparty.berlin/) Trophy hardware without having one (yet!).

> [!IMPORTANT]
> **You still need an ESP32 controller** running the WLED fork for your release:
> [WLEDLine Trophies firmware](https://github.com/qm210/wledline-trophies).
> 
> The simulator replaces the trophy hardware, not the device running the firmware.

It does so by receiving the color info from the controller via network (UDP). 

## Get started & Build

1. Prepare an ESP32 with the firmware for your compo
   - https://github.com/qm210/wledline-trophies
2. Build simulator:
   - **Windows:**  
     - Run [`build_windows.ps1`](https://github.com/qm210/dltrophy-simulator/blob/main/build_windows.ps1)
	 - if you use CLion, it should just work by loading this project (i.e. with its `CMakeLists.txt`)
   - **Linux:**
     - Try [`build_linux.sh`](https://github.com/qm210/dltrophy-simulator/blob/main/build_linux.sh)
	 - or via the Dockerfile (adjust base image if Ubuntu isn't right):
	   ```
	   docker build -t simulator-build .
	   docker run --rm -v $PWD/build:/mnt simulator-build
	   ```
	 - see also the Linux Specifics section below.

Doesn't work? Write to qm210 via Demoscene discord, or manage somehow to *write him some hate mail.*

### Might look like:
![Simulator Screenshot](https://github.com/qm210/dltrophy-simulator/raw/main/screenshot_smiuluator_jul16.jpg)

- Tested on Windows, Linux (Fedora, Ubuntu)
- I'm glad to improve support **if you tell me where it fails** :) 

### Dependencies
*not that many!*
* OpenGL 3.3 (+ GLAD + glm)
* glfw
* Dear ImGui
* MinimalSocket (low-weight abstraction)
* nlohmann::json

## Peek Preview within QM's WLED fork
For starting *somehow*, you might not even need the Simulator; as the [Deadline Trophy WLED fork]([WLEDline](https://github.com/qm210/wledline-trophies)) supports a basic preview within the Web UI itself!

0. you already have it your release installed on a suitable ESP32
1. you configure it's network access (see [WLED's Getting Started page](https://kno.wled.ge/basics/getting-started/))
2. you open the Web UI (see [also official WLED docs](https://kno.wled.ge/basics/web-ui/))
3. -> Settings
4. -> Usermods
   a) all the way down, you enter the IP of the machine where the Simulator runs
   b) match this port with the port given in the Simulator UI

The socket communication might need your corresponding port open:
https://www.techopedia.com/definition/4961/administrative-privileges

5. Then check with the WLED Web UI "Peek" screen, whether _something_ should be on the LEDs. Should run under
> http://<WLED-IP>/liveview

### Did someone spell "Smiuluator" wrong?
Why... would you...

... ask?

# Linux Specifics

## Fedora
Building on Fedora is straightforward - says Korkenzieher/team420. He will personally come to your home and gladly help you with any problems!! (I suppose)

Install Build Dependencies (assuming fedora 42)
```bash
sudo dnf install -y libXi-devel libXcursor-devel libXinerama-devel libXrandr-devel libxkbcommon-devel wayland-devel mesa-libGL-devel mesa-libGL gcc-c++ cmake git
```
## Debian / Ubuntu
-> look at the `Dockerfile`

# Generic Linux Build
```bash
# get trophy-simulator code
git clone https://github.com/qm210/dltrophy-simulator
cd dltrophy-simulator

./build_linux.sh

# will per default build both for X11 and Wayland backends. To build just one, use:
./build_linux x11
./build_linux wayland
```
