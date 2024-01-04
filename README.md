# CSS-external
A simple CS:S external cheat

This cheat is based of [ImGuiExternal](https://github.com/furkankadirguzeloglu/ImGuiExternal) which is the overlay part of the program.<br>
It also uses [valve-bsp-parser](https://github.com/ReactiioN1337/valve-bsp-parser) to parse BSP maps.<br>
Also i only use RPM so i don't write anything to the game, i still open a handle and ReadProcessMemory but it's fine for CS:S ig.<br>
I've tested the cheat on multiple servers, and on LAC, it is undetected even chen playing with the aimbot.<br>
The code isn't very good nor optimized but at least i have the good offsets.<br>

# Features
* Visibility check with BSP parsing
* Ally/Enemy ESP
* Skeleton ESP
* Box ESP
* Healthbar
* Name ESP
* Legit aimbot (MOUSE1)
* Crosshair

# Usage
`git clone https://github.com/ALittlePatate/CSS-external --recursive`<br>
Compile in x64 Release mode.<br>
Start the game then launch the cheat.<br>
Press INSERT to open the menu.<br>
Press DELETE or "unhook" in the menu to unload the cheat.<br>
Press HOME to trigger the aimbot (keeping this keybind for testing)<br>
As i use an overlay you can easily stream your game with obs/discord without anyone seeing the ESP.<br>

# Screenshots
[Demo video](https://streamable.com/8dg58q)<br>

ESP :<br>
![ESP.png](Screenshots/ESP.png)

Menu :<br>
![menu.png](Screenshots/menu.png)

# Known issues
* Performances can be better
* The ESP has a bit of latency
* Skeleton ESP can be buggy in some frames for some reasons
