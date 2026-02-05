# Irrigation System

Hirrigação automática em agricultura familiar: Uso de Sistemas Embarcados como facilitador na vida rural.

Projeto criado como trabalho final da Capacitação em Sistemas Embarcados EmbarcaTech.

## Requisitos
- [x] Utilizar Bitdoglab (RP 2040);
- [x] Programação de microcontroladores em C/C++;
- [x] Utilizar Display Gráfico;
- [x] Interface UART;
- [x] Uso de RTOS (FreeRTOS);
  - [x] Uso de Interrupções;
- [x] Integração de sensores e atuadores;
  - [x] Uso de pelo menos 1 (um) atuador;
    - [x] Relé 220V;
    - [x] Buzzer;
    - [x] LED;
  - [x] Uso de pelo menos 2 (dois) sensores do Kit EmbarcaTech;
    - [x] Sensor de Temperatura
    - [x] Sensor de Umidade
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

## Acesso HTTP

### Rede Local

Para acesso em rede local é nescessário saber o `IP` do aparelho. A nível de Monitor Serial isso é informado quando a conexão com Wi-Fi é estabelecida.

#### Endpoints

Método | Endpoint | Descrição | Entrada | Retorno
-------|----------|-----------|---------|--------
`GET`  | `/`      | Retorna informações sobre o dispositivo. | | `{...}`
`POST` | `/serial`| Destinado a teste de conexão. Imprime os dados enviados no monitor serial.| `{author: string, message: string}` | `{status: string}`
`POST` | `/clock` | Configura o relógio manualmente (sem internet). | `{year: int, month: int, day: int, hour: int, min: int, sec: int}` | `{status: string}`
`GET`| `/schedule` | Retorna todo o calendário de horários de irrigação. | | `[{index: int, hour: int, minute: int, duration: int, active: int},...]` 
`POST` | `/schedule` | Atualiza um item do agendamento. | `{index: int, hour: int, minute: int, duration: int, active: int}` | `{status: string}`

## Autor

* **Robson Gomes**
* Email: robson.mesquita56@gmail.com
* GitHub: [github.com/rob-ec](https://github.com/rob-ec)
