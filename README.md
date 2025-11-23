# Portfolio

1. Theremin

The theremin uses time of flight sensors to read the distance of two hands. One hand control pitch while the other controls volume of a skewed sine wave. Supercollider is running as the sound engine on the raspberry pi 0 two w and ESP32 is reading the sensor data. The resulting audio is sent out of the PCM5122 audio board. 

2. Offloading faust on ESP32 Lyrat mini V1.2 board and writing the codec

I wanted to run Faust DSP on the Lyra t mini V1.2 board. This was challengin because the current faust repository supports audio codecs that are WM8978. Whereas the Lyra mini board has the ES8311. As a result I wrote the codec for ES8311. 

I fixed the Faust-generated DSP code for the ESP32 LyraT Mini v1.2 by updating deprecated FreeRTOS APIs, expanding stack size, enabling PSRAM for large DSP objects, and turning on C++ exceptions. A full codec abstraction layer was built so Faust could run on the ES8311 instead of the expected WM8978. We cleaned up macro conflicts in headers, added correct I2S pin mappings for the LyraT Mini, and updated the audio HAL to match newer ESP-ADF changes. Task timing, watchdog settings, and sample-rate/I2S configs were aligned across the system. Finally, the project was reorganized into proper ESP-IDF components, resulting in a stable, working Faust DSP audio engine on the LyraT Mini.

The project is still ongoing and aims to contribute to the faust repository.

The below link includes the folder that was offloaded on the Lyra board along with audio samples and spectographic representation of the sound generated.
