
## Moga Serial to Windows Interface

The Moga line of Android controllers are neat pieces of kit, with one glaring issue.  They don't work right in Windows!  They could easily be a great all-in-one wireless controller, but no native driver means being forced to use generic HID mode, and unfortunately this comes with a whole host of problems.

Connection issues aside, the Moga identifies the triggers via the HID codes `AXIS_GAS` and `AXIS_BRAKE`.  Windows DirectInput doesn't recognize these, and thusly ignores them, despite the controller actually reporting trigger values.  Fortunately there are alternatives!

MogaSerial connects to the Moga gamepad directly via its mode A serial interface.  It can then feed controller data into either the vJoy driver for full DirectInput support, or into the SCP driver for native XInput support.

![MogaSerial](http://i63.tinypic.com/30b2rz6.png)

-----
### Download

The latest build of MogaSerial is 1.5.0, released Jan 6, 2016.  
![>](http://i64.tinypic.com/voad5u.png) [MogaSerial-v150.zip](https://github.com/Zel-os/MogaSerial/releases/download/v1.5.0/MogaSerial-v150.zip) - x86 for Windows 7, 8, and 10 


-----
### Setup

The Moga controller doesn't need to be paired with Windows, but if so it will appear on the Bluetooth device list by default.  Pin is `1234`, if needed.  

One (or both) of the interface drivers need to be installed:

##### XInput - SCP virtual bus driver
For modern games and Steam's Big Picture mode, the SCP driver emulates an Xbox 360 controller and provides full native XInput support.

This driver is included in the MogaSerial download.  Run ScpDriver.exe and click `Install`.  If you'd prefer to build the driver yourself, it can be obtained from <http://github.com/nefarius/ScpServer>.  

>If you're running Windows 7, also download and install the official [Xbox 360 Controller driver](http://www.microsoft.com/hardware/en-us/d/xbox-360-controller-for-windows).  This is pre-installed on both Windows 8 and 10.

##### DirectInput - vJoy virtual device driver
If you want full trigger support in older DirectInput games, or want to use your Moga alongside other controllers with [x360ce](http://www.x360ce.com/), the vJoy driver is a better option.  

Download and install vJoy 2.1.6 (or later) from <http://vjoystick.sourceforge.net/>.  Then run the vJoyConf configuration tool and set a controller as follows:

 - Axes: `X` `Y` `Z` `Rx` `Ry` `Rz`
 - Buttons: `12`
 - POV Hat: `1 Continuous`

> If you will only use the triggers as axes, buttons can be 10.  
> If you will only use the triggers as buttons, Z and Rz can be omitted.  
> Button mode for triggers ought to be compatible with the older Moga Pocket.


-----
#### Usage

Make sure Bluetooth is enabled, and ensure the Moga is switched to **MODE A**.

Then just select your controller from the Bluetooth drop-down and click the Moga button in the lower-right.  After a couple seconds, it will connect and begin feeding data to the selected driver.  If your Moga is not in the list, click the orange refresh button to re-scan for local Bluetooth devices, and after a few seconds it ought to appear.

The Xbox Guide button is emulated in XInput mode with SCP.  Press `Start + Select` together to trigger it.
  
Trigger mode is for vJoy only.  This determines how Windows will see `L2` and `R2`, either as the `Z` and `Rz` axis, as a combined `Z` axis, or as buttons `11` and `12`.  When using the SCP driver with DirectInput games, only combined trigger mode is available.

If the Moga disconnects due to sleeping, being shut off, or a Bluetooth error, the program will reset the connection, wait a few seconds, and try to reconnect.  Click the Moga button again to stop the controller interface.


-----
#### Notes

- The Moga responds to polling at approximately 100 updates per second.

- Curiously, there seems to be no way to get battery status through the serial interface.  It's reported as a byte code when in HID mode B, but not here.

- I only own a Moga Power Pro for testing, but MogaSerial ought to work with all Moga-brand controllers.  From what I can tell, the serial communication protocol hasn't changed between models.


------------------------
##### Changes

* 1.5.0
  * Added support for the SCP driver to get native XInput functionality.
* 1.4.0
  * GUI window now minimizes to the system tray.
* 1.3.2
  * Tweaks to try and address the reported input lag problem.  
* 1.3.0
  * Program settings are now stored in the system registry under HKCU\Software\MogaSerial.
  * Added 'Combined Axes' option for the trigger mode.  This mimics how the XBox360 controller behaves under DirectInput.
* 1.2.0
  * Debug switch added to display raw controller output. 
  * First public release of the MFC GUI version of MogaSerial.

------------------------

Thanks to badfontkeming@gmail.com for first making a similar tool that works with vJoy and the Moga in HID mode B, and inspiring me to finish this project here.  Thanks as well to the ZeeMouse author for showing that a serial mode A connection ought to be straightforward.  
<http://ngemu.com/threads/moga-pro-power-triggers-not-detected.170401/>


Moga controllers are (c) PowerA and MogaAnywhere.com  
This application is (c) Jake Montgomery - jmont@sonic.net
