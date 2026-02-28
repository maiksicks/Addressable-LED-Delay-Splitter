# Addressable LED Delay Splitter - Gallery

## Demo video

https://github.com/user-attachments/assets/cb7658ee-294e-4ca6-bfc9-5e69e2aed46c

In this example video, the device is configured to send the first 10 pixels to the primary output and the remaining pixels to the secondary output.
Additionally, the RGBW jumper is set.

The device visible in the bottom left corner is my ESP32-S3 based ARGB controller. It has two ARGB outputs, 4 IOs, Power measurment, Ethernet and some other features. It is not related to the delay splitter, but I thought it would be nice to show it as well. The controller is designed to be used as low-latency UDP to ARGB bridge and it runs ESPHome firmware with custom components. It can be also flashed with WLED firmware, but the support for the W5500 ethernet module is tricky.
(Separate project, will be published soon ...)

Direct links: [Video](../assets/media/demo.mp4), [Image](../assets/media/demo_setup.jpeg)

## Images

<img src="../assets/media/pcb_top.jpeg" alt="Top view of the delay splitter" width="600" />
<img src="../assets/media/pcb_bottom.jpeg" alt="Bottom view of the delay splitter" width="600" />

<br>
Note: I've hand soldered the PCB because I don't need more than a few devices. It is possible to get assembled PCBs. The size however requires the PCBs do be panelized, which increases the amount of final PCBs and parts a lot and thus the cost.

## Rendered images

<img src="../assets/render/pcb_3d.png" alt="3D view of the delay splitter" width="600" />
<br>
<img src="../assets/render/pcb_top.png" alt="Top view of the delay splitter" width="300" />
<img src="../assets/render/pcb_bottom.png" alt="Bottom view of the delay splitter" width="300" />
