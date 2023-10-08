# Low-cost GNSS positioning for glaciology

This repository describes the build and configuration of the low-cost GNSS positioning units used in the publication:

[Still, H., Odolinski, R., Bowman, H., Hulbe, C. and Prior, D. (Under review) Observing glacier dynamics with low-cost, multi-GNSS positioning in Victoria Land, Antarctica. Submitted to the _Journal of Glaciology_](https://drive.google.com/file/d/1XmEQSZw7YCs4UeDsx9XjOYceR0UcZ_Ou/view?usp=drive_link)

![Priestley Glacier, Victoria Land, Antarctica](/Documentation/Images/DSC_0474_crop.jpeg)


## Table of contents

## Motivation

GNSS (global navigation satellite system) positioning is ubiquitous in the cryospheric sciences, with uses ranging from routine field site navigation tasks to precise monitoring and measurement of deformation. While applications have proliferated, the cost of access to the state of the art has remained high with “survey” or “geodetic” grade receivers priced at 10s of thousands USD. Low-cost, mass-market, open-source GNSS chip devices - a relatively new and rapidly developing technology - are an alternative to the proprietary systems typically used in glacier and other cryosphere studies. Coupled with a low-cost patch antenna, these systems are light and compact, with relatively low power consumption and a cost of entry around 500 USD.  

High equipment costs can be prohibitive to scientific discovery, limiting the concurrent deployment of multiple GNSS receivers over large areas of interest and restricting access to users with well-financed research programs. Our work shows that alternatives are readily available and we encourage everybody to adopt them.   

## Overview of the low-cost GNSS system




## Hardware components

| Component               | Description        | Serial number     | Cost        |
|-------------------------|------------------------------------------------------|
| U-blox ZED-F9P receiver | |||




A detailed list of components is provided [here](/Hardware). 


## Set-up

1. Configure the u-blox receiver with a CONFIG.txt file. We use the freely-available software [u-center](https://www.u-blox.com/en/product/u-center) to generate the CONFIG.txt file and write the configuration to the receiver. A CONFIG.txt file looks like this:

In this case, we enable the u-blox receiver to log GPS, GLONASS, Galileo, Beidou, QZSS and SBAS constellations. 

## Resources


## Contributors


## Citation



