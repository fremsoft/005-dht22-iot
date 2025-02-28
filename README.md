# 005-dht22-iot
Dispositivo per la trasmissione della temperatura e umidità di un locale su sito web

Il dispositivo è composto da 4 elementi:

* https://amzn.to/3aGO4Sa : Scatole con spina 230V CONRAD, Strapubox STECKER-GEH
* https://amzn.to/3RDnE4i : TECNOIOT 5 pz AC-DC 5V 700mA AC 220V a 5V DC 3.5W Step Down Modulo di Alimentazione
* https://amzn.to/3ESbvGP : IoT WLan WiFi DHT11 + ESP01 modulo sensore umidità

oppure
* https://s.click.aliexpress.com/e/_opMvvCm : IoT WLan WiFi DHT22 + ESP01 modulo sensore umidità
* https://amzn.to/3IJ6aQ4 : SDENSHI DHT22 - Modulo sensore di temperatura digitale e umidità, 34 x 15 x 8 mm
* https://amzn.to/3Obofr5 : Programmatore ESP-01S USB

!!! ATTENZIONE !!! 

Il kit è molto pericoloso perchè è collegato alla tensione di rete. È un kit per addetti ai lavori. Se vuoi realizzare questo kit, devi sapere come maneggiare la corrente elettrica.

## Video dimostrativi 
Il kit viene illustrato nei più piccoli particolari nei seguenti video:
* https://youtu.be/USjC7U5dfC0 : parte 1 - protocollo di comunicazione DHT22 con Arduino UNO 
* https://youtu.be/TniLj2XxBDA : parte 2 - creazione webservice su Hosting Web


## Software
Il software si suddivide in due parti:

### Lato client ESP-01S
Questo programma consente al dispositivo di trasmettere le informazioni di temperatura e umidità a un webserver via Wi-Fi.

Se la Wi-Fi non è opportunamente configurata, il dispositivo si mette in ascolto per essere configurato via Wi-Fi in modalità Soft-AP tramite una pagina web via smartphone.

### Lato server
Un semplice script PHP riceve i dati dal modulo Wi-Fi ESP-01S e li registra in un file in formato J-SON.

Collegandosi alla stessa pagina web senza inviare dei dati, la pagina risponde mostrando le temperature e le umidità registrate nel file J-SON.

Una macchina potrebbe dialogare direttamente con il file J-SON per gestire i dati ed eventualmente attivare automaticamente sistemi di riscaldamento o raffrescamento.
