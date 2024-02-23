Boatswain uses HID to control Stream Decks. The various commands are documented
around the internet, but centralizing that here too is helpful.

# Models

As of today (03-10-2022) there are 6 known Stream Deck models: Original (v1 and
v2), Mini, XL, MK.2, and Pedal.

These are the specs of each device and its buttons:

| Model         | Product ID | Layout     | Format | Size    | Transform             |
|---------------|------------|------------|--------|---------|-----------------------|
| Mini          | 0x0063     | 6  (2 x 3) | BMP    | 80x80   | Y-flipped, rotated 90 |
| Mini (v2)     | 0x0090     | 6  (2 x 3) | BMP    | 80x80   | Y-flipped, rotated 90 |
| MK.2          | 0x0080     | 15 (3 x 5) | JPEG   | 72x72   | X-flipped, Y-flipped  |
| Original (v1) | 0x0060     | 15 (3 x 5) | BMP    | 72x72   | X-flipped, Y-flipped  |
| Original (v2) | 0x006d     | 15 (3 x 5) | JPEG   | 72x72   | X-flipped, Y-flipped  |
| Pedal         | 0x0086     | 3  (1 x 3) | none   | none    | none                  |
| Plus          | 0x0084     | 8  (2 x 4) | JPEG   | 120x120 | none                  |
| XL            | 0x006c     | 32 (4 x 8) | JPEG   | 96x96   | X-flipped, Y-flipped  |

# Commands

All Stream Deck models (except Pedal) implement the following commands:

 * **Get Serial Number**: retrieves the serial number of the device
 * **Get Firmware Version**: retrieves the firmware version of the device
 * **Reset**: resets buttons to the Elgato logo
 * **Read Buttons**: reads the state of all buttons (1 for pressed, 0 otherwise)
 * **Set Brightness**: sets the brightness of all buttons (there's no per-button brightness)
 * **Upload Image**: sets the image of a button

## Stream Deck Mini / Mini (V2)

### Get Serial Number

 * **Type**: get feature report
 * **Input**: (none)
 * **Output**:
   * **serial number**: string from index 5 to 16
 * **Command**:
   ```
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```
 * **Notes**: values of 1 to 4 are garbage and should be ignored

### Get Firmware Version

 * **Type**: get feature report
 * **Input**: (none)
 * **Output**:
   * **serial number**: string from array index 5 to 16
 * **Command**:
   ```
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```
 * **Notes**: array values from 1 to 4 are garbage and should be ignored

### Read Buttons

 * **Type**: read
 * **Input**: unsigned char array of size 7
 * **Output**:
   * 1 for pressed, 0 otherwise
   * Starts at index 1
   * Button order: top left to bottom right
 * **Command**: (none)

### Reset

 * **Type**: send feature report
 * **Input**: (none)
 * **Output**: (none)
 * **Command**:
   ```
   0x0b, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Set Brightness

 * **Type**: send feature report
 * **Input**:
   * **brightness**: 0 (no brightness) to 100 (full brightness)
 * **Output**: (none)
 * **Command**:
   ```
    0x05, 0x55, 0xaa, 0xd1, 0x01, brightness, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Upload Image

 * **Type**: write
 * **Input**:
   * **button**: index of the button (0 to 5)
   * **upload_complete**: 1 if this is the last chunk of image, 0 otherwise
   * **current_chunk**: current chunk of the image
 * **Output**: (none)
 * **Package Layout**:
   * **Write size**: 1024 (16 header, 1008 image data)
   * **Header**:
     ```
      0x02, 0x01, current_chunk, 0x00, upload_complete, button + 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
     ```
 * **Pseudo-code**:
     ```
     packet = unsigned char array[1024]
     current_chunk = 0
     upload_complete = false
     
     while upload_complete == 0:
         upload_complete = remaining image data < 1008 bytes ? 1 : 0
         
         # Header
         packet[0:15] = [
             0x02, 0x01,
             current_chunk,
             0x00,
             upload_complete,
             button + 1,
             0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00
         ]
         
         # Image
         packet[16:1023] [ ... image data ... ]
         
         current_chunk += 1
         
         device.write(packet)
     ```

## Stream Deck Original (v1)

## Get Serial Number

Same as Stream Deck Mini

## Get Firmware Version

Same as Stream Deck Mini

### Read Buttons

 * **Type**: read
 * **Input**: unsigned char array of size 16
 * **Output**:
   * 1 for pressed, 0 otherwise
   * Starts at index 1
   * **Button order: top right to bottom left (IMPORTANT!)**
 * **Command**: (none)
## Reset

Same as Stream Deck Mini

## Set Brightness

Same as Stream Deck Mini

### Upload Image

 * **Type**: write
 * **Input**:
   * **button**: index of the button (0 to 15)
   * **upload_complete**: 1 if this is the last chunk of image, 0 otherwise
   * **current_chunk**: current chunk of the image
 * **Output**: (none)
 * **Package Layout**:
   * **Write size**: 8191 (16 header, 8175 image data)
   * **Header**:
     ```
      0x02, 0x01, current_chunk, 0x00, upload_complete, button + 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
     ```
 * **Pseudo-code**:
     ```
     packet = unsigned char array[1024]
     current_chunk = 0
     upload_complete = false
     
     while upload_complete == 0:
         upload_complete = remaining image data < 7803 bytes ? 1 : 0
         
         # Header
         packet[0:15] = [
             0x02, 0x01,
             current_chunk,
             0x00,
             upload_complete,
             button + 1,
             0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00
         ]
         
         # Image
         packet[16:8190] [ ... image data (maximum of image size / 2) ... ]
         
         current_chunk += 1
         
         device.write(packet)
     ```
  * **Notes**:
    * 72x72 BMP images always have 15606 bytes (that means the loop in the
      pseudo-code above always runs exactly 2 times)

## Stream Deck Original (v2) / XL / MK.2

Stream Deck Original (v2), XL, and MK.2 all seem to share the same USB protocol.

### Get Serial Number

 * **Type**: get feature report
 * **Input**: (none)
 * **Output**:
   * **serial number**: string from index 1 to 31
 * **Command**:
   ```
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Get Firmware Version

 * **Type**: get feature report
 * **Input**: (none)
 * **Output**:
   * **serial number**: string from index 1 to 31
 * **Command**:
   ```
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Read Buttons

 * **Type**: read
 * **Input**: unsigned char array of size n_buttons + 4
 * **Output**:
   * 1 for pressed, 0 otherwise
   * Starts at index 4
   * Button order: top left to bottom right
 * **Command**: (none)

### Reset

 * **Type**: send feature report
 * **Input**: (none)
 * **Output**: (none)
 * **Command**:
   ```
   0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Set Brightness

 * **Type**: send feature report
 * **Input**:
   * **brightness**: 0 (no brightness) to 100 (full brightness)
 * **Output**: (none)
 * **Command**:
   ```
    0x03, 0x08, brightness, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   ```

### Upload Image

 * **Type**: write
 * **Input**:
   * **button**: index of the button (0 to n_buttons - 1)
   * **upload_complete**: 1 if this is the last chunk of image, 0 otherwise
   * **current_chunk**: current chunk of the image
   * **current_chunk_size**: size current chunk of the image
 * **Output**: (none)
 * **Package Layout**:
   * **Write size**: 1024 (8 header, 1016 image data)
   * **Header**:
     ```
      0x02, 0x07, button, upload_complete, current_chunk_size & 0xff, current_chunk_size >> 8, current_chunk & 0xff, current_chunk >> 8
     ```
 * **Pseudo-code**:
     ```
     packet = unsigned char array[1024]
     current_chunk = 0
     upload_complete = 0
     remaining_image_data_size = (size of image data)
     
     while upload_complete == 0:
         upload_complete = remaining_image_data_size < 1016 bytes ? 1 : 0
         current_chunk_size = MIN (remaining_image_data_size, 1016);
         
         # Header
         packet[0:7] = [
             0x02, 0x07,
             button,
             upload_complete,
             current_chunk_size & 0xff,
             current_chunk_size >> 8,
             current_chunk & 0xff,
             current_chunk >> 8
         ]
         
         # Image
         packet[8:1023] [ ... image data ... ]
         
         remaining_image_data_size -= current_chunk_size
         current_chunk += 1
         
         device.write(packet)
     ```

## Stream Deck Pedal

Stream Deck Pedal seem to share the USB protocol with recent versions:
Original (v2), XL and MK.2. Difference is it only has 3 buttons which cannot
be changed visually (as it don't have a display)

## Stream Deck Plus

Stream Deck Plus shares the same USB protocol as of Original (v2), XL, and MK.2,
with the exception of reading buttons states, and with the addition of sending
and image to the touchscreen.

### Read Buttons

 * **Type**: read
 * **Input**: unsigned char array of size 14
 * **Output**:
   * `array[0]` is ignored
   * `array[1]` is the event type, which can be:
     * **0x00**: key button press or release
       * `array[4:11]` are the key buttons states (0: released, 1: pressed)
       * Button order: top left to bottom right
     * **0x02**: touchscreen event
       * All touchscreen events have a position
         * X position: `(array[7] << 8) + array[6]`
         * Y position: `(array[9] << 8) + array[8]`
       * `array[4]` is the touchscreen event type
         * **0x01**: touchscreen tap
         * **0x02**: touchscreen long press
         * **0x03**: touchscreen swipe
           * Release X position: `(array[11] << 8) + array[10]`
           * Release Y position: `(array[13] << 8) + array[12]`
     * **0x03**: dial event
       * `array[4]` is the dial event type
         * **0x00**: dial press or release
           * `array[5:8]` are the dial buttons pressed state (0: released, 1: pressed)
           * Button order: left to right
         * **0x01**: dial rotation
           * `array[5:8]` are the relative dial rotations
             * If value < `0x80`, this is a clockwise rotation by the same amount
             * If value >= `0x80`, this is a counter-clockwise rotation by the opposite amount (i.e. `0x100 - value`)
           * Button order: left to right
 * **Command**: (none)

### Upload Touchscreen Image

(TODO: Not implemented)
