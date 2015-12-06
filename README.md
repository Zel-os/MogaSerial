<pre>
------------------------------------------
  Moga Serial to vJoy Interface, v 0.9.4 
------------------------------------------

The Moga line of controllers are neat pieces of kit, with one glaring issue.
They don't work right in Windows!  No native driver means being forced to use
generic HID mode, which unfortunately comes with a whole host of problems.

Connection issues aside, the Moga identifies the triggers via the HID codes
AXIS_GAS and AXIS_BRAKE.  Windows DirectInput doesn't recognise these, and
thusly ignores them, despite the controller actually reporting trigger values.

Fortunately the vJoy driver project gives an alternative.  This tool connects
to the Moga directly via its mode A serial interface, then feeds controller
data to the vJoy driver through the provided api.


SETUP
=====
Both of the following need to be installed:
 - Visual C++ Redistributable Packages for Visual Studio 2013 
 - vJoy 2.0.5 or above (vjoystick.sourceforge.net)

Run the vJoyConf configuration tool and set controller 1 as follows:
 - Axes: X Y Z Rx Ry Rz    - Buttons: 10    - POV Hat: 1 Continuous

Pair the Moga controller with the computer in MODE A.  Pin is 1234, if needed.


USAGE
=====
Make sure bluetooth is enabled, and ensure the Moga is switched to MODE A.
Then launch MogaSerial.exe, and select the bluetooth name that matches the
mode A pairing for the controller.  After a couple seconds, it should connect
and begin feeding data to the vJoy driver.

If the Moga disconnects due to sleeping, being shut off, or a bluetooth error,
the tool will reset the connection, wait a few seconds, and try to reconnect.
To quit, just close the window, or focus it and hit ctrl-c.


NOTES
=====
vJoy gives a warning when using the 2.0.5 dll with newer driver versions.
vJoy outright fails when using it with older driver versions.  If you've got
the 2.1.6 driver installed, you can get rid of the warning message by replacing
the vJoyInterface.dll with the updated version inside the vJoy installation
directory.  The older dll still appears to work fine however.

The Moga responds to polling at approximately 100 updates per second.

Curiously, there seems to be no way to get battery status through the serial
interface.  It's reported as a byte code when in HID mode B, but not here.


TO-DO
=====
A proper GUI or system tray dock would be nice to have.  Support for more than
one controller would be nice, too.  A switch to choose which controller number
the Moga lights up when connected would just be frivolous fun.


== Changes from 0.9.3
--- Fixed controller state not being properly zero'd on init and disconnect.
== Changes from 0.9.2
--- vJoy device id selection routine - added by badfontkeming@gmail.com
== Changes from 0.9.0
--- Listen mode seems to be working consistently now.  Reduced active polling to once
     every two seconds to check for disconnects.  This should reduce bluetooth network
     traffic and prevent any chance of missed inputs.
--- General code cleanups.

------------------------

Thanks to badfontkeming@gmail.com for first making a similar tool that works
with vJoy and the Moga in HID mode B, and inspiring me to finish this project
here.  Thanks as well to the ZeeMouse author for showing
that a serial mode A connection ought to be straightforward.
http://ngemu.com/threads/moga-pro-power-triggers-not-detected.170401/


Moga controllers are (c) PowerA and MogaAnywhere.com
This application is (c) Jake Montgomery - jmont@sonic.net
</pre>
