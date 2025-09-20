## Deadline Trophy Smiuluator
I built this so anyone with a ESP32 microcontroller can aspire to be one of these
absolute elite of carefully selected indviduals that actually own a Deadline 2024 Trophy.

To simulate, you know?

![Simulator Screenshot](https://github.com/qm210/dltrophy-simulator/raw/main/screenshot_smiuluator_jul16.jpg)
Read below.

Or don't, if you rather choose (I don't mind).


### Tested on
- so far, only my Windows machine. 

I'm glad to improve support for any platform you tell me your desires about 

### Dependencies
Triyn' to be minimal:
* OpenGL 3.3 (+ GLAD + glm)
* glfw
* Dear ImGui
* MinimalSocket (low-weight abstraction)
* nlohmann::json

## UDP packets from QM's WLED fork
As of now, the official WLED release does not give you realtime updates of all
currently displayed color
(except for their "peek" preview via the Web UI,
for which I might add WebSocket support in the future).

The QM-Deadline-Fork [WLEDline](https://github.com/qm210/wledline-trophies) is equipped with
that missing feature:
1. you install that onto a ESP32 and run it
2. you open the Web UI (is it clear how?)
3. -> Settings
4. -> Usermods

   a) all the way down, you enter the IP of the machine where the Simulator runs

   b) match this port with the port given in the Simulator UI

For the socket communication to work, you might need to open your corresponding port:
https://www.techopedia.com/definition/4961/administrative-privileges

You can check with the WLED Web UI "Peek" screen, whether _something_ should be on the LEDs.

### What's with the "Smiuluator" word?
I guess I have to admit that I'm a highly handicapped worst-kind-of-autistic individual with no respect
or any regard for the emotions and demands of wellbeing of anyone, anything that ever existed
and this is why I allow myself (and yourself!) to also use that spelling of the word "simulator". 

# Linux dependencies 

## Fedora
Building on Fedora is straight forward, says the drawer of Korkens.

Install Build Dependencies (assuming fedora 42)
```bash
sudo dnf install -y libXi-devel libXcursor-devel libXinerama-devel libXrandr-devel libxkbcommon-devel wayland-devel mesa-libGL-devel mesa-libGL gcc-c++ cmake git
```
## Debian / Ubuntu
have a look at the Dockerfile for inspiration

# Generic Linux Build

```bash
# get trophy-simulator code
git clone https://github.com/qm210/dltrophy-simulator

# make build directory && enter
mkdir dltrophy-simulator/build
cd dltrophy-simulator/build

# Build wayland && x11:
cmake ../. 

# Build Software, with 10 cores:
make -j10
```
## Disable X11 or Wayland Build

```bash
# Disable Wayland
cmake ../. -D GLFW_BUILD_WAYLAND=0

# Disable x11
cmake ../. -D GLFW_BUILD_X11=0
```
