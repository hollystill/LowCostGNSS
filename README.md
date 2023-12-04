# Low-cost GNSS positioning for glaciology

This repository describes the build and configuration of the low-cost GNSS positioning units used in the publication:

[Still, H., Odolinski, R., Bowman, M., Hulbe, C., & Prior, D. (2023). Observing glacier dynamics with low-cost, multi-GNSS positioning in Victoria Land, Antarctica. _Journal of Glaciology_, 1-40. doi:10.1017/jog.2023.101](https://doi.org/10.1017/jog.2023.101)

The low-cost, low-power GNSS units use u-blox ZED-F9P receivers and are designed to monitor the velocity of Antarctic glaciers and ice shelves. 


<!---![Priestley Glacier, Victoria Land, Antarctica](/Documentation/Images/DSC_0474_crop.jpeg)--->


<figure>
<img src="/Documentation/Images/DSC_0474_crop.jpeg">
<figcaption>
<b>Figure 1.</b> GNSS positioning tests conducted on Priestley Glacier in Victoria Land, Antarctica, November 2022.
</figcaption>
</figure>

## Contents
[Motivation](#motivation) | [Overview](#overview) | [Hardware](#hardware) | [How does it work?](#setup) | [Power consumption](#power) | [Example data](#example) | [Resources](#resources) | [License](#license) | [Contributors](#contributor) | [Citation](#citation)  

<a name="motivation"></a>
## Motivation

Global navigation satellite system (GNSS) positioning is ubiquitous in the cryospheric sciences, with uses ranging from routine field site navigation tasks to precise monitoring and measurement of deformation. Glaciological applications typically use geodetic or ‘survey-grade’ GNSS receivers that retail for >$20,000 NZD per unit. High equipment costs can be prohibitive to scientific discovery, limiting the concurrent deployment of multiple GNSS receivers over large areas of interest and restricting access to users with well-financed research programs.

 Low-cost, mass-market, open-source GNSS chip devices - a relatively new and rapidly developing technology - are an alternative to the proprietary systems typically used in glacier and other cryosphere studies. Coupled with a low-cost patch antenna, these systems are light and compact, with relatively low power consumption and a cost of entry around $500 NZD.  Here we demonstrate the set-up of a low-cost, u-blox GNSS unit for use in high-latitude glaciated environments. Our work shows that alternatives to expensive survey-grade systems are readily available and we encourage everybody to adopt them.   

 <figure>
<p align="center">
<img src="/Documentation/Images/cost_overview.png" style="width:80%">
</p>
<!---<figcaption>
 <strong>Figure 2.</strong> U-blox ZED-F9P receiver board.
</figcaption>--->
</figure>


<a name="overview"></a>
## Summary of the low-cost GNSS system

- Each low-cost, low-power GNSS installation includes a receiver, antenna, data logger and power source (12 V battery and solar panel). 

- Key components include a [u-blox ZED-F9P GNSS receiver module](https://www.u-blox.com/en/product/zed-f9p-module) and [u-blox patch antenna](https://www.u-blox.com/en/product/ann-mb-series).     

- The [u-blox ZED-F9P GNSS receiver module](https://www.u-blox.com/en/product/zed-f9p-module) is capable of receiving multi-GNSS signals: GPS (L1/L2), GLONASS (L1/L2), Galileo (E1/E5b), Beidou (B1/B2), and QZSS (L1/L2) systems and frequencies. 

- The receiver is configured to log multi-GNSS, dual frequency observations at 1 Hz.  RXM-RAWX messages (raw carrier phase, pseudorange, Doppler and signal quality information) and RXM-SFRBX messages (broadcast navigation data) are enabled and the raw binary u-blox files are stored with an Arduino data logger to micro SD card.

- The low-cost u-blox receiver + patch antenna system consumes <50% less power than survey-grade alternatives (e.g., Trimble NetR9 and R10 systems).

- **Our experiments show that the precision of the low-cost system is comparable to survey-grade alternatives [(Still et al., 2023)](/Paper/LowCostGNSSpaper.pdf)**

<figure>
<p align="center">
<img src="/Documentation/Images/static_GNSS_experiment.png" style="width:80%">
</p>
<figcaption>
<b>Figure 3.</b> A stationary comparison between low-cost (u-blox ZED-F9P) and survey-grade (Trimble R10) systems conducted in Terra Nova Bay, Antarctica.
</figcaption>
</figure>

<a name="hardware"></a>
## Hardware 


<figure>
<p align="center">
<img src="/Documentation/Images/ublox_fix.jpg" style="width:50%">
</p>
<figcaption>
 <strong>Figure 4.</strong> <a href="https://gnss.store/zed-f9p-gnss-modules/99-13-elt0087.html#/27-add_antenna-without_antenna">U-blox ZED-F9P receiver  and <a href="https://www.adafruit.com/product/2796">Adafruit Feather M0 Adalogger.</a> 
</figcaption>
</figure>



### Key components

- [U-blox ZED-F9P receiver board](https://www.u-blox.com/en/product/zed-f9p-module)
- [U-blox ANN-MB multiband patch antenna](https://www.u-blox.com/en/product/ann-mb-series)
- [Adafruit Feather M0 Adalogger](https://www.adafruit.com/product/2796) 

<!---!
### Table 1. Components to build a low-cost GNSS unit.

| Component                                                                                      | Description              | Serial number     | Cost (EUR)  |
|------------------------------------------------------------------------------------------------|--------------------------|-------------------|-------------|
| [U-blox ZED-F9P receiver board](/Documentation/Manuals/ZED-F9P-04B_DataSheet_UBX-21044850.pdf) | GNSS receiver            |  x                | 209.99      |
| [U-blox ANN-MB patch antenna](https://www.u-blox.com/en/product/ann-mb-series)                 | Multi-band GNSS antenna  |  x                | 60 USD      |
| [Eltehs surveying antenna](https://gnss.store/gnss-rtk-multiband-antennas/140-elt0123.html)    | Alternative GNSS antenna |  x                | 180.99      |
| [Adafruit Feather M0 Adalogger](https://www.adafruit.com/product/2796)                         | Data logger              |  x                | 19.95 USD   |
| [FeatherWing Proto Board](https://www.adafruit.com/product/2884)                               | x                        |  x                | 4.95 USD    |
--->


A detailed list of components is provided [here](/Hardware). 

<a name="setup"></a>
## How does it work?

### Configure the GNSS receiver

1. Configure the u-blox receiver with a [CONFIG.txt](/Software/Ublox-ZED-F9P-configuration/priestley_glacier.txt) file. We use the freely-available software [u-center](https://www.u-blox.com/en/product/u-center) to generate the CONFIG.txt file and write the configuration to the receiver.

2. In this case, we enable the u-blox receiver to log RXM-RAWX messages (raw carrier phase, pseudorange, Doppler and signal quality information) and RXM-SFRBX messages (broadcast navigation data) for the satellite constellations visible in the Ross Sea region of Antarctica:
   - GPS L1/L2, GLONASS (L1/L2), Galileo (E1/E5b), Beidou (B1/B2), and QZSS (L1/L2)

3. The u-blox ZED-F9P receivers tested in this project use the firmware version 1.32 (May, 2022) which can be downloaded from [here](https://www.u-blox.com/en/product-resources?query=ZED-F9P%2520HPG%25201.32%2520firmware&file_category=Firmware%2520Update&legacy=Current) and installed with [u-center](https://www.u-blox.com/en/product/u-center). Our configuration file is specific to this firmware version.     

### Data logging

1. The data-logger includes an [Arduino microcontroller Adafruit Feather Cortex M0 Adalogger (SAMD21 chip)](https://www.adafruit.com/product/2796), logging to a 32 GB micro-SD card.  A helpful overview is available [here:](https://learn.adafruit.com/adafruit-feather-m0-adalogger/)
<!---!
Components include a Cortex-M0+ microcontroller and a micro-SD card port.
--->

2. The data-logging system uses the [SparkFun u-blox GNSS Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library) and is inspired by [data logging example 3](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library/tree/main/examples/Example3_GetPosition) by Paul Clark.

3. The Arduino data-logging code is included the software directory. Further details can be found at https://github.com/HamishB/uBlox_PPP_logger



### File formats

1. Raw GNSS data is logged in the proprietary u-blox .ubx file format. 

1. U-blox data streams can be converted to standard RINEX 3.03 (Receiver Independent Exchange) format using [open-source RTKLIB tools](https://www.rtklib.com/). 

1. We prefer to use [RTKLIB tools](https://www.rtklib.com/) to post-process our GNSS datasets for flexibility in parameter settings and positioning method (e.g., single-baseline kinematic positioning or PPP). For simple and fast results, use the [CSRS-PPP service](https://webapp.csrs-scrs.nrcan-rncan.gc.ca/geod/tools-outils/ppp.php).

<a name="power"></a>
## Power consumption

The low-cost GNSS units are powered by two 10 W, 12 V solar panels and a 12 V, 18 A h SLA battery. The rate of power consumption is <50% lower than for survey-grade alternatives (0.57 W for the u-blox ZED-F9P module + patch antenna + Arduino Cortex M0 logger, versus 1.25 W for a Trimble R10 system, and 3.67~W for a Trimble NetR9 system).


<a name="example"></a>
## Example data:

We installed four u-blox and two Trimble GNSS stations along the left shear margin of Priestley Glacier, Antarctica, in November 2022. Our objective was to monitor tidally-modulated 3D ice motion with centimetre-level precision.


<figure>
<p align="center">
<img src="/Documentation/Images/dynamic_GNSS_experiment.png" style="width:80%">
</p>
<figcaption>
<b>Figure 5.</b> Along- and across-flow ice displacement (coloured line) and velocity (black line) measured by u-blox (Ub1) and Trimble (Tr1) stations installed on Priestley Glacier. The tide prediction is from the <a href="https://github.com/EarthAndSpaceResearch/TMD_Matlab_Toolbox_v2.5">CATS2008 tide model.</a> 
</figcaption>
</figure>


<figure>
<p align="center">
</br>
<img src="/Documentation/Images/place_map.png" style="width:60%">
</p>
<figcaption>
<b>Figure 6.</b> GNSS stations installed on Priestley Glacier. 
</figcaption>
</figure>
</br>

<a name="resources"></a>
## Resources

- [u-center GNSS evaluation software](https://www.u-blox.com/en/product/u-center) is used to configure u-blox receivers.

- [u-blox ZED-F9P GNSS receivers](https://www.u-blox.com/en/product/zed-f9p-module)

- [RTKLIB](https://www.rtklib.com/) is an open-source software library for GNSS data processing. 



<a name="license"></a>
## License

This project is released under the [MIT License](opensource.org/license/mit/).


<a name="contributor"></a>
## Contributor information

- :artificial_satellite: Holly Still is a PhD candidate at the School of Surveying, University of Otago, New Zealand. Email: holly.still@postgrad.otago.ac.nz

- :artificial_satellite: Hamish Bowman is a Computing and Numerical Simulation Technician at the University of Otago, New Zealand.


<a name="citation"></a>
## Citation

The _Journal of Glaciology_ paper can be acknowledged with the following citation:

- Still, H., Odolinski, R., Bowman, M., Hulbe, C., & Prior, D. (2023). Observing glacier dynamics with low-cost, multi-GNSS positioning in Victoria Land, Antarctica. Journal of Glaciology, 1-40. doi:10.1017/jog.2023.101

```
@article{still_odolinski_bowman_hulbe_prior_2023, 
  title={Observing glacier dynamics with low-cost, multi-GNSS positioning in Victoria Land, Antarctica}, 
  DOI={10.1017/jog.2023.101}, 
  journal={Journal of Glaciology}, 
  publisher={Cambridge University Press}, 
  author={Still, Holly and Odolinski, Robert and Bowman, M. Hamish and Hulbe, Christina and Prior, David J.}, 
  year={2023}, 
  pages={1–40}}
```

<figure>
<img src="/Documentation/Images/DSC_0319.jpg">
<figcaption>
<b>Figure 7.</b> A frozen meltwater pond on Priestley Glacier, November 2022.
</figcaption>
</figure>
