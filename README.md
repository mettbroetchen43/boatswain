Control Elgato Stream Deck devices.

[<img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png' />](https://flathub.org/apps/details/com.feaneron.Boatswain)

## Features

 * Organize your actions in pages and profiles
 * Set custom icons to buttons
 * Play sound effects during your streams
 * Control OBS Studio using Stream Deck (requires the obs-websocket extension)

![Boatswain Screenshot](https://gitlab.gnome.org/World/boatswain/-/raw/main/data/stream-deck-original.png)

## Code of conduct

Boatswain follows the GNOME project [Code of Conduct](./code-of-conduct.md). All
communications in project spaces, such as the issue tracker or
[Discourse](https://discourse.gnome.org) are expected to follow it.

## udev rules

Most Elgato Stream Deck devices should be compatible starting from udev v250. 
Stream Deck Pedal is not yet in any udev release. Stream Deck XL v2 is only
available with udev v252. If your version of udev is older than that, add the
following content to `/etc/udev/rules.d/50-elgato.rules`:

```
# Elgato Stream Deck Mini
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0063", TAG+="uaccess"

# Elgato Stream Deck Mini (v2)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0090", TAG+="uaccess"

# Elgato Stream Deck Original
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0060", TAG+="uaccess"

# Elgato Stream Deck Original (v2)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006d", TAG+="uaccess"

# Elgato Stream Deck XL
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006c", TAG+="uaccess"

# Elgato Stream Deck XL (v2)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="008f", TAG+="uaccess"

# Elgato Stream Deck MK.2
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0080", TAG+="uaccess"

# Elgato Stream Deck Pedal
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0086", TAG+="uaccess"
```

Then reload with:

```
# udevadm control --reload-rules
```

Finally, reboot your computer. You should be able to use Boatswain now.


## Testing without devices

Boatswain allows emulating an arbitrary number of devices. This might be helpful
for designers and translators to test the application. To force Boatswain to use
fake devices, you must build Boatswain with `-Dprofile=development`, and launch
it as follows:

```
$ env BOATSWAIN_EMULATE_DEVICES=1 boatswain
```

You can have multiple fake devices by setting the `BOATSWAIN_N_DEVICES` variable
to a number.
