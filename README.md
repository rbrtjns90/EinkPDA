# EinkPDA
[Please note that this project is a work in progress and incomplete]

@Ashtf 2025

![IMG20250114202659](https://github.com/user-attachments/assets/c39189e0-8c66-448f-a0ef-99eec2af6684)

![IMG20250114202546](https://github.com/user-attachments/assets/446d8884-8f92-4619-9335-53f150a269c2)

# [Project Summary]
  This project is a PDA powered by an ESP32-S3 running a custom "OS" written in C++ using the Arduino IDE. This project utilizes an E-Ink and OLED screen in tandem to mitigate the refresh rate restrictions of an E-Ink panel while retaining the aesthetics and benefits of using E-Ink. This project is a work in progress and currently amounts to a simple GUI for navagating between apps, a text (.txt) file editor, and a basic file manager. More applications are planned for the future and a list of TO-DOs can be found below.

  At this point, the plan for this project is to get the software and hardware to maturity and once a finished product is achieved, a few things will happen. First, the project files (Code, KiCad schematics, 3D files) will be open-sourced and free for anyone to access. Next, I will likely begin selling kits that allow people to purchase all the parts required to build a device for themselves if they don't want to source the parts themselves. Finally, the community will be encouraged to help develop the software for the device and hopefully push it even further than I could myself.

  I will try my best to keep this GitHub up to date with the development of the device.

# [TO-DO]
- Calendar app
- Tasks app
- Scrolling w/ OLED preview
- Plugging in a USB automatically backs up the files
- Transfer to/from PC via BT/USB
- fix e-ink refresh on every button press
- Rework refresh system with display(true/false) and safety measures
- Bluetooth keyboard support
- Battery icon for charging
- Refresh() and refreshPartial() using new display() knowledge
- Add cooldown on partial and full refresh, look up times
- Add safeguards, counter on partial that forces full update
- Debug refresh happening on each button press, use serial monitor


# License
All files are distrubuted under GNU GPLv3 license:

EInkPDA - A small note-taking and productivity device using E-Ink and OLED.
Copyright (C) 2025 Ashtf

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
