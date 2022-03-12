Control Elgato Stream Deck devices.

## Features

 * Organize your actions in pages and profiles
 * Set custom icons to buttons
 * Play sound effects during your streams
 * Control OBS Studio using Stream Deck (requires the obs-websocket extension)

## udev rules

Add the following content to `/etc/udev/rules.d/50-elgato.rules`:

```
# Elgato Stream Deck Mini
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0063", TAG+="uaccess"

# Elgato Stream Deck Original
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0060", TAG+="uaccess"

# Elgato Stream Deck Original (v2)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006d", TAG+="uaccess"

# Elgato Stream Deck XL
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006c", TAG+="uaccess"

# Elgato Stream Deck MK.2
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0080", TAG+="uaccess"
```

Then reload with:

```
# udevadm control --reload-rules
```

Finally, reboot your computer. You should be able to use Boatswain now.
