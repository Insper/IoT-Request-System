# IoT-Request-System
<p>Developers: Pedro Paulo Telho and Matheus Pellizzon.</p>
<br />
<h3>Description:</h3>
<p>Our project consists of getting digital and analogical signals from a SAM E70 ARM PROCESSOR and sending it via REST API to an EC2 server disposed at AWS structure.</p>
<br />
<h3>Descrição:</h3>
<p>Nosso projeto consiste em captar sinais digitais e analógicos do processador ARM SAM E70 e enviá-los via REST API para um servidor disponibilizado em uma máquina EC2 da AWS.</p>
<hr>
<h4>Required system:</h4>
<ul>
  <li>Windows 10</li>
  <li><a href="http://studio.download.atmel.com/7.0.2397/as-installer-7.0.2397-web.exe">Atmel Studio</a></li>
  <li><a href="https://gallery.microchip.com/api/v2/package/EFC4C002-63A3-4BB9-981F-0C1ACAF81E03/2.8.4">Serial Port for AtmelStudio</a></li>
</ul>
<br />
<h4>Sistema necessário:</h4>
<ul>
  <li>Windows 10</li>
  <li><a href="http://studio.download.atmel.com/7.0.2397/as-installer-7.0.2397-web.exe">Atmel Studio</a></li>
  <li><a href="https://gallery.microchip.com/api/v2/package/EFC4C002-63A3-4BB9-981F-0C1ACAF81E03/2.8.4">Serial Port for AtmelStudio</a></li>
</ul>
<hr>
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
<br />
<h4>Componentes:</h4>
<ul>
  <li>1x SAM E70 Xplained (ATMEL)</li>
  <li>1x WIFI - ATWINC1500-XPRO (Wifi module)<br />
    Especificações:
    <ul>
      <li>IEEE 802.11 b/g/n 20MHz (1x1) solution</li>
      <li>Supports IEEE 802.11 WEP, WPA, and WPA2 Security</li>
      <li>SPI, UART, and I2C host interface</li>
    </ul>
  </li>
  <li>Jumpers</li>
  <li>1x potenciometro B10K</li>
</ul>
<hr>
<h4>First steps:</h4>
<p>To use our code it is necessary to connect the Wifi module at port EXT1 and the potentiometer in EXT2 as follows on image below.</p>
<br />
<h4>Primeiros passos:</h4>
<p>Para utilizar nosso código é necessário conectar o módulo Wifi na porta EXT1 e o potenciometro na porta EXT2 como é apresentado na imagem abaixo.</p>
<p align="center">
  <img src="IoT-system.png" title="IoT-connections">
</p>
<p>Potentiometer:</p>
<p>Potenciometro:</p>
<ol>
  <li>VCC</li>
  <li>PC 13</li>
  <li>GND</li>
</ol>
<hr>
<h4>How interruptions work?</h4>
<p>All code was made using interruptions to get the data.</p>

<p>Interruptions are basically a signal/warning that some action was taken, be it by pressing a button or receiving/sending something via wi-fi. For this project, the information collected was sent every second, updating the data displayed online.</p>

<p>The data collected refers to the digital and analogical signals, along with a timestamp and an ID of the processor.</p> 
<br />
<h4>Como as interrupções funcionam?</h4>
<p>Todo o código foi criado utilizando interrupções para capturar os dados.</p>

<p>Interrupções são basicamente sinais/avisos que alguma ação foi tomada, seja pressionando algum botão ou recebendo/enviando um sinal via wi-fi. Para esse projeto, a informação coletada é enviada a cada segunda, atualizando os dados no display do servidor online.</p>

<p>Os dados coletados se referem aos sinais digitais e analógicos, com um timestamp e o ID do processador.</p> 
<hr>
<h4>How to run our code:</h4>
<p>Into directory python-server</p>
```bash
python3 server.py --host=0.0.0.0
```
<h4>Como rodar nosso código:</h4>
<p>Dentro do diretório python-server</p>
```bash
python3 server.py --host=0.0.0.0
```
<hr>
<h2>DON'T FORGET TO MAKE NECESSARY ADAPTIONS INTO "WIFI-RTOS-get/src/main.h"</h2>
<h2>NÃO SE ESQUEÇA DE FAZER AS ADAPTAÇÕES NECESSÁRIAS DENTRO DE "WIFI-RTOS-get/src/main.h"</h2>
