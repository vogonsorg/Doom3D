Doom3D Version 1.16 Readme
**************************

Doom3D is a Win32/DirectX port of Doom, Id Software's FPS.

System Requirements
-------------------

- Windows 9x/2000
- DirectX 6.0 or higher

Reccomended
-----------

- Sound Card
- Direct3D compatible 3D graphics card

Features
--------

- High resolution modes
- Runs in a window
- Full TCP-IP Network support (including multi-monitor 360 degree capability)
- Compressed WAD file support
- Matrix support(see website)
- Sound+Music
- Hardware accelerated version(requires Direct3D compatible 3D card)
- optional MD2 model replacements for sprites
- Complies to GL-Friendly Nodes (v2) specification
- Splitscreen multiplayer - including Dual Mouse support!
- console and quake-style key bindings

Dll Descriptions
----------------

c_sw8:  'Original'(software) 8-bit renderer, also works in a window on a 32-bit
        color desktop.
c_sw16: Above renderer 'Fudged' into 16-bit color (still only 256 actual colors!),
        Runs faster than c_sw8 when in window.
c_d3d:  All-new(ish) Direct3D Hardware renderer. Requires 3D card.
        Adjusting the advanced settings can often give improved results.
		Now allows replacing sprites with MD2 models.
		I reccommend you use glBSP to generate GL-Friendly nodes as this
		fixes several problems.

MD2 model support
-----------------

see md2.txt for more information

You may also need to increase the heap size when using MD2 models.

GL-Friendly Nodes
-----------------

Early versions of Doom3D had trouble rendering some floors/ceilings, resulting
in gaps through which the sky was visible. This problem is fixed by supporting the use of GL-Friendly nodes.

To take advantage of this feature you will need to generate nodes using
a GL-friendly node builder.

The only node builder currently available is glBSP, and is available from
http://www.netspace.net.au/~ajapted/glbsp.html

glBSP cannot read compressed WAD files, but you can compress WAD and GWA files
after running glBSP.

PWADs
-----

PWADs can be used in the same way as with the original Doom, ie. by adding the
-file <filename> parameter either in a shortcut or in the parameters box in
the setup program.

Enabling the Fix Sprites option allows Doom3D to use the sprites in some
PWAD's without needing to use DEUSF, or similar. WADs that contain additional
S_START/S_END or SS_START/SS_END lumps _should_ work properly, but I haven't
tested them. any feedback would be appreciated

Using the WAD file compressor
-----------------------------

Doom3D supports compressed WAD files. To create a compressed WAD file use
COMPWAD.EXE (included). This takes 2 command line parameters, the first is the
name of the original WAD file, the socon is the name of the new compressed wad
file. This should work with all WAD files (including PWADS). Decompressing
WAD files to their original form is not currently supported.
Adding '-fast' gives a significant speed increase, with only slightly less
compression.

NOTE: compressed WAD files only work with Doom3D, and the original Doom
executables will NOT be able to read them. YOU HAVE BEEN WARNED. You are also
advised to make a backup first, just in case something goes wrong.

Splitscreen Multiplayer
-----------------------

Doom3D supports multiplayer games with more than one person per machine using
splitscreen mode. This can be enabled either with the '-split' command line or
through the setup utility by altering the 'Local Players' netgame setting.

A maximum of 4 players can play on each machine.

Doom3D also supports using a second mouse. To use this, plug a searial mouse
into a spare serial port, and set the COM port in the controls section of the
setup utility. Many PS2 mice come with an adapter that allows you to plug them
into a serial port. Some non-microsoft mice (mostly old 3-button ones I think)
work in mouse systems mode, to use these set the com port to a negative value,
ie -1 if on com1, -2 if on com 2, etc. You may also want to change which player
the second mouse controls.

Splitscreen mode can also be used with conventional netgames, the players on
each machine are numbered sequentialy, although the total maximum of 4 players 
still applies, this may be increased in future versions.

Keyboard Bindings & Console Commands
------------------------------------

If a key is bound to "+command", then "+command" will be executed when the key
is pressed, and "-command" will be executed when the key is released.
Multiple commands can be seperated by a semicolon (;).
To bind or alias a command consisting of more than one word, the command must
be surrounded by quotes.

Available commands:

alias <alias> <command> - create alias, omitting parameters will display
    current aliases
bind <key> <command> - bind command to key, bind <key> will display current
    binding
exec <filename> - execute command script

The following accept an optional parameter specifying which player they apply to
(for splitscreen mode)
autorun - toggles autorun on/off
weapon <number> - select weapon by number (1=fist, 2=pistol, etc.)
nextwep - select next available weapon
+fire, +run, +use, +forward, +back, +left, +right, +strafe, +strafeleft,
+straferight - player controls

Most controls are also configurable via. the options menu.

Advanced Settings
-----------------
(Direct3D renderer only)

*UseColorKey
If non-zero will use color keying rather than alpha information for
trasparency effects. Can be faster but doesn't work on all 3D cards. Can also
cause sprites to have a pink halo when combined with bilinear filtering.

*BilinearFiltering
Zero to use point filtering, 1 to use bilinear filtering.
Enabling bilinear filtering 'smoothes' the textures. Disabling can give
a performance increase, and looks more like the original.

*TextureMip
*SpriteMip
Hardware rendering requires texture dimensions to be exact powers of 2.
A value of 0 rounds dimensions up, 1 rounds down, so some loss of quality on
non-power-of-2 sized textures. A value of 2 uses half the size of 1, 3 uses
a quarter, etc. Can inprove performance, especialy if funning low on texture
memory.
Cards with 4MB or more texture memory should be OK with a value of 0, but 2MB
cards need a value of at least 1. If you see white/gray untextured walls try
increasing this. A negative value indicates use the actual size, which almost
certainly won't work.
Using MD2 models generaly requires much more texture memory than sprites.

*ForceSquareTextures
If non-zero forces all textures to be square in size. Most (all?) 3D cards
require this to be set.

*MinTextureSize
Sets the minimum size for textures. Should be a power of 2. Not really sure why
you'd want to, it's just a left over debugging option

*MinTextureSize
Sets the maximum size for textures. Should be a power of 2. Many cards don't
suport textures larger than 256.

*MD2ini
The ini file to use to determine how to use MD2 models. see md2.txt

*NoMD2Weapons
Use 2D (sprite) weapons even if MD2 models available

*Skin<n>
Skins to use for other players in netgames.

*3D Display Device
Select which Direct3D device to use for rendering

Contact Info
------------

WebPage:

http://www.redrival.com/fudgefactor/

Any comments, questions, bug reports, money, etc. welcome.

email: pbrook@bigfoot.com

***When sending bug reports, please run with -debug parameter, and include
debug.txt with your bug report.

Command line options
--------------------

These can either be typed in on the command line, entered into the Parameters
box in the setup utility, or added to any shortcuts you create.

Most of these are options are also configuable via. the setup utility.

Memory:
-heapsize <n>
	Allocate an <n> MB heap (Default=8).
	Hardware renderer needs less memory than software.
	A setting of >half physical memory will probably cause 
	excessive swapfile paging and deteriorated preformance.
	Can also be set width setup utility.

Network: - Note network games can be started via. the setup program
-split [n]
	Splitscreen mode with n players (max. 4).
-net <playernum> [hostname[:port]] ...
	Starts a network game.
	<playernum> indicates your player (1-4), or first player if using
	splitscreen.
	You must include all network nodes(ie. players+drones) in this list
-port <portnum>
	sets the TCP/IP port to use, default 1638 decimal (666 hex:-)
-players <num>
	Set total number of players
-right
-left
-back
	For 3(4?)-monitor mode. Same as -drone -nogun -viewangle <90|270|180>
	If you've got a couple of spare computers lying about then you can use 2
	as drone screens to give 270/360 degree vision-quite an advantage in
	deathmatch!
-drone
	drone mode, follows annother player, most useful in conjunction with
	-viewangle & -viewoffset
-netcheat
	Enable cheats. Must enabled by all players to take effect.

Graphics:
-fps
	Display framerate counter. Can also toggle with 'idfps' cheatcode during
	play
-matrix
	enable matrix mode
-minterleave <lines>
	default=10
-msep <distance>
	default=30
-nogfn
	disable use of GL-Friendly nodes(Direct3D only)

-viewangle <theta>
	offsets the view by theta degrees
	ie. 0=forward(default), 90=right, 180=backwards, 270=left
-viewoffset <dist>
	offsets the view sideways (+ve ot right, -ve to left)
	For creating right and left eyes, Idealy with 2 projectors through
	polariod and a pair of polatising glasses(like in the big cinemas:)
-nogun
	don't draw player's gun sprite on screen.

Sound:
-nosound
	Disable sound interface, use this if you're having problems such as
	"Unable to create primary sound buffer".
-nomusic
	Disable ingame music

Misc:
-config <filename>
	use alternate config file.
-debug
	Create debug.txt file
-iwad <filename>
    specify main iwad name, allows you to use doom1&2 in same directory
-file <name>.wad
	specify pwad name

Experimental PVS support
------------------------

Doom3D v1.16 contains some experimental code for performing PVS calculations.
This generates information on which parts of the level are visible from
different locations, allowing the renderer to skip drawing those which are
not visible, resulting in an overall increase in game speed.
As the code is still not fully tested, and generation of this information
can take some time exery time a level is loaded (a few seconds on a p3-550!)
it is disabled by default. To enable PVS generation add -genpvs to the command
line.
Please don't send me bug reports on this as originaly it wasn't going to be
in this release at all.


Known Bugs
----------

- Sometimes throws up an error on Alt-Tab in fullscreen mode
- May crash with many voodoo based systems in Direct3D mode-please let me know
  if you do get Doom3D to run on a Voodoo system
- automap doesn't work in splitscreen, and only partialy in Direct3D mode
- music volume slider not fully implemented (either full on or off)
- MD2 HUD weapons can sink into walls&floors
- Some bits of the map are visible where there shoud only be sky - I'm working on this

New in version 1.16
-------------------

- Console (press '~')
- Quake-style key bindings
- Controls added to options menu
- Splitscreen multiplayer
- Dual mouse support

New in version 1.14
-------------------

- MD2 model support fixed
- Keypad keys now recognised properly
- Fixed crashes on some levels when using 'GL-Friendly Nodes'

New in version 1.12
-------------------

- MD2 model support
- Support of GL_Friendly nodes
- Customisable controls
- Autorun
- Joystick support
- Fixed 'Plane vertex overflow' error
- Parameters box in setup program now works
- Network code now supports IP addresses for opponents
- Lots of bug fixes (honest)
- Episode 4 of Ultimate Doom now works

New in version 1.10
-------------------

- Music works!
- Improved sound support
- Improved texture handling when using Direct3D
- Gamma correction more effective with Direct3D
- Much improved setup utility
