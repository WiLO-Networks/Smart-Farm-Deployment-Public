# Smart-Farm-Deployment-Public

## Setup Environment (PC)

### 1. Install VS Code

Official website for downloading: <https://code.visualstudio.com/>

### 2. Download This Repo

### 3. Open Project

Unzip the downloaded project and open it in VS Code.

You may need to install extension 'PlatformIO' in VS Code. Do it as instructed by VS Code (a pop-up window at right-bottom conner), or manually search it in the marketplace: <https://code.visualstudio.com/docs/editor/extension-marketplace>

Make sure the 'PlatformIO' is working by checking the bottom toolbar of VS Code. If it's blue (not purple), you are in the PlatformIO environment now.

The toolbar should looks like: <https://docs.platformio.org/en/latest//integration/ide/vscode.html#platformio-toolbar>

More information on this tutorial for PlatformIO: <https://docs.platformio.org/en/latest//integration/ide/vscode.html#installation>

## Program MCU

### 1. Change Longitude and Latitude (Node)

Line 1002 and 1003 of file `duke-smart-farm-deployment`:

```cpp
prep_data_json["lat"] = "40.8067311";
prep_data_json["lng"] = "-73.9566026";
```

When you program the 'nodes', change the latitude and longitude according to the Google Map localizaiton on your smart-phone.

### 2. Change Configuration (Node and Gateway)

There are two sets of configurations in the file `platformio.ini`, you may need to:

1. Comment the 'node', leave 'gateway' active, when programming the gateway (on the wall)
2. Comment the 'gateway', leave 'node' active, when programming the node (in the field)

On Mac, select multiple lines, use `command + /` to do quick comment.

### 3. Upload Code (Node and Gateway)

Connect the MCU to your laptop with the USB cable.

(You may need to choose the upload_port in `platformio.ini`, but this is usually automatically done by PlatformIO)

Click the "Upload" button in PlatformIO toolbar: <https://docs.platformio.org/en/latest//integration/ide/vscode.html#platformio-toolbar>

## Check Battery and Reboot (Node)

Refer to file `test/node.jpg`

![alt text](./test/node.jpg?raw=true)

Check the connection of battery according. Pay attention to the positive and negative.

Click the 'Reset button' to do a easy reboot.
