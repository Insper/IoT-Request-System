# IoT-Request-System
<p>Developers: Pedro Paulo Telho and Matheus Pellizzon.</p>
<br />
<h3>Description:</h3>
<p>Our project consists of getting digital and analogical signals from a SAM E70 ARM PROCESSOR and sending it via REST API to an EC2 server disposed at AWS structure.</p>
<h4>Required system:</h4>
<ul>
  <li>Windows 10</li>
  <li><a href="http://studio.download.atmel.com/7.0.2397/as-installer-7.0.2397-web.exe">Atmel Studio</a></li>
  <li><a href="https://gallery.microchip.com/api/v2/package/EFC4C002-63A3-4BB9-981F-0C1ACAF81E03/2.8.4">Serial Port for AtmelStudio</a></li>
</ul>
<h4>Components:</h4>
<ul>
  <li>1x SAM E70 Xplained (ATMEL)</li>
  <li>1x WIFI - ATWINC1500-XPRO (Wifi module)<br />
    Specifications:
    <ul>
      <li>IEEE 802.11 b/g/n 20MHz (1x1) solution</li>
      <li>Supports IEEE 802.11 WEP, WPA, and WPA2 Security</li>
      <li>SPI, UART, and I2C host interface</li>
    </ul>
  </li>
  <li>Jumpers</li>
  <li>1x potentiometer B10K</li>
</ul>
<h4>First steps:</h4>
<p>To implement our code it is necessary to connect the Wifi module at port EXT1 and the potentiometer in EXT2 as follows on image below.</p>
<p align="center">
  <img src="IoT-system.png" title="IoT-connections">
</p>
<p>Potentiometer:</p>
<ol>
  <li>VCC</li>
  <li>PC 13</li>
  <li>GND</li>
</ol>
<h4>How interruptions work?</h4>
<p>All code was made using interruptions to get them data.</p>

<p>Interruptions are basically a signal/warning that some action was taken, be it by pressing a button or receiving/sending something via wi-fi. For this project, the information collected was sent every second, updating the data displayed online.</p>

<p>The data collected refers to the digital and analogical signals, along with a timestamp and an ID of the processor.</p> 
