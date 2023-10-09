# Low-cost GNSS positioning for glaciology

This repository describes the build and configuration of the low-cost GNSS positioning units used in the publication:

[Still, H., Odolinski, R., Bowman, H., Hulbe, C. and Prior, D. (Under review) Observing glacier dynamics with low-cost, multi-GNSS positioning in Victoria Land, Antarctica. Submitted to the _Journal of Glaciology_](https://drive.google.com/file/d/1XmEQSZw7YCs4UeDsx9XjOYceR0UcZ_Ou/view?usp=drive_link)


<figure>
![Priestley Glacier, Victoria Land, Antarctica](/Documentation/Images/DSC_0474_crop.jpeg)
</figure>

## Table of contents

## Motivation

GNSS (global navigation satellite system) positioning is ubiquitous in the cryospheric sciences, with uses ranging from routine field site navigation tasks to precise monitoring and measurement of deformation. Glaciological applications typically use geodetic or ‘survey-grade’ GNSS receivers that retail for >$20,000 NZD per unit. High equipment costs can be prohibitive to scientific discovery, limiting the concurrent deployment of multiple GNSS receivers over large areas of interest and restricting access to users with well-financed research programs.


 Low-cost, mass-market, open-source GNSS chip devices - a relatively new and rapidly developing technology - are an alternative to the proprietary systems typically used in glacier and other cryosphere studies. Coupled with a low-cost patch antenna, these systems are light and compact, with relatively low power consumption and a cost of entry around 500 USD.  Here we demonstrate the set-up of a low-cost, u-blox GNSS units for use in glaciated environments. Our work shows that alternatives to expensive survey-grade systems are readily available and we encourage everybody to adopt them.   

## Overview of the low-cost GNSS system

- Each low-cost GNSS installation includes a receiver, antenna, data logger and power source (12 V battery and solar panel) (Table \ref{tbl:hardware}). The u-blox ZED-F9P GNSS receiver module is capable of tracking GPS (L1/L2), GLONASS (L1/L2), Galileo (E1/E5b), Beidou (B1/B2), and QZSS (L1/L2) systems and frequencies \citep{ublox2022c}. 

- The receiver is configured to log all available satellites and frequencies at 1 Hz using the software U-center v22.07 \citep{ublox2022b}.  RXM-RAWX messages (raw carrier phase, pseudorange, Doppler and signal quality information) and RXM-SFRBX messages (broadcast navigation data) are enabled and the raw binary u-blox files are stored with an Arduino data logger to micro SD card. 


## Hardware components


| Component                                                                                | Description        | Serial number     | Cost (EUR)  |
|------------------------------------------------------------------------------------------|--------------------|-------------------|-------------|
| [U-blox ZED-F9P receiver](/Documentation/Manuals/ZED-F9P-04B_DataSheet_UBX-21044850.pdf) | GNSS receiver      |  x                |             |


We also evaluate the performance of two low-cost multiband antenna models:  the u-blox ANN-MB patch antenna \citep{ublox2022a} and an Eltehs multiband (ELT0123) standard surveying antenna \citep{Eltehs2023} (Table \ref{tbl:hardware}). 

A detailed list of components is provided [here](/Hardware). 


## How does it work?

1. Configure the u-blox receiver with a CONFIG.txt file. We use the freely-available software [u-center](https://www.u-blox.com/en/product/u-center) to generate the CONFIG.txt file and write the configuration to the receiver. A CONFIG.txt file looks like this:

In this case, we enable the u-blox receiver to log GPS, GLONASS, Galileo, Beidou, QZSS and SBAS constellations. 


2. The first processing step involves a conversion from the proprietary u-blox and Trimble raw data file formats to standard RINEX 3.03 (Receiver Independent Exchange) files. U-blox data streams are converted using open-source RTKLIB tools \citep{Takasu2009}. 


## Power consumption

The low-cost GNSS units are powered by two 10 W, 12 V solar panels and a 12 V, 18 A h SLA battery. The rate of power consumption is relatively low (0.57 W for the u-blox ZED-F9P module + patch antenna + Arduino Cortex M0 logger, versus 1.25 W for a Trimble R10 system, and 3.67~W for a Trimble NetR9 system).

## Resources


## Contributors


## Citation



