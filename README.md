# Irrigation System

Hirrigação automática em agricultura familiar: Uso de Sistemas Embarcados como facilitador na vida rural.

Projeto criado como trabalho final da Capacitação em Sistemas Embarcados EmbarcaTech.

## Requisitos
- [x] Utilizar Bitdoglab (RP 2040);
- [x] Programação de microcontroladores em C/C++;
- [ ] Utilizar Display Gráfico;
- [x] Interface UART;
- [x] Uso de RTOS (FreeRTOS);
  - [ ] Uso de Interrupções;
- [ ] Integração de sensores e atuadores;
  - [ ] Uso de pelo menos 1 (um) atuador;
  - [ ] Uso de pelo menos 2 (dois) sensores do Kit EmbarcaTech;
- [x] Comunicação sem fio com protocolos IoT;
  - [x] Wi-Fi/Bluetooth;
- [x] Uso de pelo menos um dos protocolos: MQTT, CoAP, UDP, TCP/IP ou HTTP;
- [ ] Aplicação de conceitos de segurança;

### Requisitos opcionais
- [ ] Uso de Machine Learning (TinyML ou outra biblioteca);
- [ ] Uso de Direct Memory Access (DMA);
- [ ] Utilização de linguagens de alto nível para software de apoio (dashboards, servidores, scripts de integração);

## Usando o projeto

### Configurações

#### Wi-Fi

Antes de compilar, mude as configurações de Wi-Fi para as configurações da sua rede em [src/wifi_connection.h](src/wifi_connection.h).

```c
#define WIFI_SSID "NOME_DO_WIFI"
#define WIFI_PASS "SENHA_DO_WIFI"
```

## Autor

* **Robson Gomes**
* Email: robson.mesquita56@gmail.com
* GitHub: [github.com/rob-ec](https://github.com/rob-ec)
