Pseudo-Theremin

This is a guide to make a DIY theremin.

The Pseudo-Theremin is a DIY version of the Leo Teremin's Theremin. It uses Time of flight sensors to measure the distance of the users
hands from the instrument. The right hand controls the pitch and left hand controls the amplitude. 

Note: We will be writing code in Supercolider & Arduino. A lttle bit of programming experience will help but feel free to copy paste 
the code from the software folder

List of components
1. Raspberry pi 0 two w
2. SD card (8 gb or above)
3. ESP32 Dev Kit Module
4. Female to Female jumper wires
5. VL53L0X time of flight sensrs x 2
5. OTG cable/converter
6. USB A to micro USB cable x 2
7. laptop

On your laptop download the raspberry pi imager from the raspberry pi official website. Select the OS of your laptop. 
Now install the imager on your laptop. Connect the SD card to your laptop. Once your laptop reads the SD card open the Raspberry pi imager.

Now we will put an Operating system on the SD card and later put that in the Raspberry pi. We will also put the initial configuration
for the raspberry pi.

Choose Device : Raspberry Pi zero 2 w
Operating System : Raspberry Pi OS Lite (64 bit)
Choose Storage : "Select your SD Card"

Now while you edit settings

Tick set hostname 

Fill in: pizero // this makes an alias for the IP address of the pi by the name of pizero.local

Fill username & password

tick Configure Network

SSID : "Put the name of the wifi that your computer is connected to" // This will connect the raspberry pi to the wifi
Password : "Put in password for your wifi" // This will help with authentication.

Please enter your timezone for the pi.

In the services section, you can put your SSH key


Flash Debian GUI-less OS to the Pi

Using Raspberry Pi Imager offload Debian GUI-less OS on it using an SD card.

Raspberry Pi Imager: Enable SSH explicitly

Even though you mention SSH keys, you didn’t specify:

Enable SSH

Set password or SSH key

Set hostname

Without explicitly enabling SSH, many Pis won’t allow first-boot SSH.
Put in Wifi Connections details that is wifi and password Choose a username and a name for the IP Address name in the raspberry pi imager. Put the sd card in your pi and put in a power cable

Connect your mac to the same wifi as your pi.

Bash:

ssh “put the raspberry pi username here”@” put the IP address here”


It might ask you for a password.

NOTE: If this fails try puting your computer’s ssh key into the sd card using the imager and offload the OS again.

Now you can access the pi using your laptop.


1. Download the missing kernel modules + overlays manually

You pulled the Raspberry Pi device tree overlays package from the Raspberry Pi GitHub kernel repository:

git clone https://github.com/raspberrypi/linux.git

Then you navigat to the overlays:

cd linux/arch/arm/boot/dts/overlays


From there you copied the relevant overlays to your Debian /boot/overlays directory:

sudo cp iqaudio-dacplus.dtbo /boot/overlays/
sudo cp hifiberry-dac.dtbo /boot/overlays/


(I remember you had to create the directory on Debian)

sudo mkdir -p /boot/overlays

2. Copy the missing PCM5122 codec kernel modules

Debian did NOT ship these:

snd-soc-pcm512x.ko

snd-soc-pcm512x-i2c.ko

snd-soc-iqaudio-dac.ko

You downloaded them from the same kernel repo:

cd ~/linux/sound/soc/codecs


Then copied them into your Debian kernel module directory:

sudo cp pcm512x.ko /lib/modules/$(uname -r)/kernel/sound/soc/codecs/
sudo cp pcm512x-i2c.ko /lib/modules/$(uname -r)/kernel/sound/soc/codecs/


(And for some boards:)

sudo cp snd-soc-iqaudio-dac.ko /lib/modules/$(uname -r)/kernel/sound/soc/bcm/

3. Ran depmod to update kernel module list
sudo depmod -a

4. Added the overlay manually to Debian’s /boot/firmware/config.txt

Open config file 


sudo nano /boot/config.txt or sudo nano /boot/firmware/config.txt

Refer to the config.txt file in the folder and make changes accordingly                 

5. Rebooted
sudo reboot


After this reboot, Debian finally loaded the PCM5122 driver.


Make the following ToF sensor connections
D18 > SDA pins on both the Tof sensors
D23 > SCL pins on both the Tof sensors
D14 > Xshut pin on sensor 1
D27 > Xshut pin on sensor 2
3v  > Both VCC
GND > Both GND

ESP32 Setup

Connect ESP32 to your laptop using a Micro USB data cable
Download Arduino IDE

On your other computer offload the following code to the esp32. You will have to download the board manager for ESP32 and adafruit_VL53L0X library in Arduino IDE.

Add this in:

arduino > Setting > Additional Board Manager URLs

https://espressif.github.io/arduino-esp32/package_esp32_index.json


In Arduino IDE offload the below code. Make sure you select the right port and board (ESP32 Dev Module).


Test audio

Mount the PCM 5122 board on the pi and solder the pins on the pi. Now run

aplay -l


which showed the card as:

card 1: sndrpihifiberry


Play audio:

aplay -D hw:1,0 Free_Test_Data_500KB_WAV.wav

Make the following ToF sensor connections
D18 > SDA pins on both the Tof sensors
D23 > SCL pins on both the Tof sensors
D14 > Xshut pin on sensor 1
D27 > Xshut pin on sensor 2
3v  > Both VCC
GND > Both GND

ESP32 Setup

Connect ESP32 to your laptop using a Micro USB data cable
Download Arduino IDE

On your other computer offload the following code to the esp32. You will have to download the board manager for ESP32 and adafruit_VL53L0X library in Arduino IDE.

Add this in:

arduino > Setting > Additional Board Manager URLs

https://espressif.github.io/arduino-esp32/package_esp32_index.json


In Arduino IDE offload the below code. Make sure you select the right port and board (ESP32 Dev Module).

Refer to arduino.ino file for the code that has to be offloade

NOTE: the offload speed should be 115200.

Check serial output

You should see something like:

102a23b
99a44b
106a50b

Connect ESP32 to the Pi

Using OTG connector, plug ESP32 into the Pi and run:

ls /dev/ttyUSB*


It should show:

/dev/ttyUSB0


Print incoming data:

sudo stty -F /dev/ttyUSB0 115200 raw -echo
cat /dev/ttyUSB0

Download Jack
sudo apt install jackd2 alsa-utils


Verify:

jackd --version


List audio devices:

aplay -l


Start jack (example):

jackd -dalsa -dhw:0 -r48000 -p256 -n3

Download SuperCollider

Updated the system:

sudo apt update
sudo apt upgrade -y


Installed SuperCollider:

sudo apt install supercollider supercollider-server sc3-plugins


Verified install:

sclang -v


Installed Qt headless deps:

sudo apt install qt5-default qtwayland5


Run SuperCollider headless:

QT_QPA_PLATFORM=offscreen sclang

Then:

Once this is done

s.boot; // This will boot the server

NOTE: You might have to reboot the server because of a possible glitch

s.reboot;

Now run the below commands to make to read and assign the input values to freq of a sin wave and amplitude

Refer to the Supercolider.scd file for the code.

