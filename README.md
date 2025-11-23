# Portfolio

1. Theremin

It is made of two time of flight sensors to read the distances of the player's hands. One hand control pitch while the other controls volume of a skewed sine wave. Supercollider is running as the sound engine on the raspberry pi 0 two w and ESP32 is reading the sensor data. The resulting audio is sent out of the PCM5122 audio board. 

Media files of Theremin

Tutorial: https://github.com/anantrohmetra/Portfolio/tree/main/Theremin/Software

Images: https://github.com/anantrohmetra/Portfolio/tree/main/Theremin/Hardware

2. Offloading faust on ESP32 Lyrat mini V1.2 board and writing the codec

This project began while exploring how to offload various libraries onto the Lyra-T Mini v1.2 board. Integrating Faust DSP was particularly challenging, as the official Faust repository currently supports only the WM8978 audio codec, while the LyraT Mini relies on the ES8311. To address this limitation, we wrote ES8311 codec and adapted the faust2esp32 generated files to function seamlessly on the LyraT board.

The Faust-generated DSP code was adapted for the ESP32 LyraT Mini v1.2 by updating deprecated FreeRTOS APIs, increasing stack size, enabling PSRAM for large DSP structures, and enabling C++ exceptions. Because the Faust repository supports only the WM8978 codec, we developed a codec abstraction layer so the system could run on the LyraT Mini’s ES8311 codec. Additional fixes included resolving macro conflicts, adding correct I2S pin mappings, and updating the audio HAL to match recent ESP-ADF changes. We also aligned task timing, watchdog settings, and sample-rate/I2S configurations throughout the project. The codebase was then reorganized into ESP-IDF components, resulting in a working Faust DSP engine on the LyraT Mini.

Codec, audio files and more: https://github.com/anantrohmetra/Portfolio/tree/main/Faust-%3EESP32-Lyra-t-mini-V-1.2

