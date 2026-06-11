# 🌿 IrrigaBot

> Irrigador automático de plantas com ESP32, sensor de umidade do solo e controle via app web.

# 🌿 Link do repositório do APP do IrrigaBot: https://github.com/gustavosx1/irrigabot-ap
---

## 📋 Visão geral

O IrrigaBot monitora a umidade do solo em tempo real e aciona automaticamente uma mini bomba d'água quando necessário. Um servidor HTTP embarcado no ESP32 expõe uma API REST que permite monitoramento e controle remoto via app web (Lovable / qualquer navegador na mesma rede).

---

## 🧰 Componentes

| Componente | Quantidade | Conexão |
|---|---|---|
| ESP32 Dev Module | 1 | — |
| Mini bomba d'água 5V | 1 | Via relé |
| Módulo relé 1 canal | 1 | GPIO25 |
| Sensor de umidade do solo | 1 | GPIO34 (AO) |
| Módulo LDR | 1 | GPIO35 (AO) |
| Fonte externa (4x pilhas AA) | 1 | VIN + GND |
| Protoboard + jumpers | — | — |

---

## 🔌 Diagrama de conexões

```
┌─────────────────────────────────────────────────────────────────┐
│                        PROTOBOARD                               │
│   (+) ──────────────────────────────────────────────────────    │
│   (–) ──────────────────────────────────────────────────────    │
└───┬─────────────────────────────────────────────────────────────┘
    │
    │  VCC (+)              GND (–)
    │
┌───┴──────────────────┐       ┌──────────────────────┐
│   4 pilhas AA (~6V)  │       │    Sensor solo       │
│                      │       │  VCC · GND · AO──────┼──► GPIO34
└──┬───────────────────┘       └──────────────────────┘
   │
   ├──► VIN (ESP32)            ┌──────────────────────┐
   │                           │    Módulo LDR        │
   └──► VCC (Relé)             │  VCC · GND · AO──────┼──► GPIO35
                               └──────────────────────┘
┌─────────────────────────────────────────────────────┐
│                      ESP32                          │
│                                                     │
│  VIN ◄── (+) fonte Proto   GPIO25 ──► IN  (Relé)    │
│  GND ◄── (–) fonte Proto   GPIO34 ◄── AO (Solo)     │
│                            GPIO35 ◄── AO (LDR)      │
│                                                     │
└─────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│                   Módulo Relé                        │
│                                                      │
│  VCC ◄── (+) fonte                                   │
│  GND ◄── (–) fonte                                   │
│  IN  ◄── GPIO25 (ESP32)                              │
│  COM ◄── (+) vermelho da bomba                       │
│  NO  ──► sem ligação (fecha ao acionar)              │
└──────────────────────────────────────────────────────┘

┌──────────────────────┐
│   Mini bomba 5V      │
│  (+) vermelho ──► COM (Relé)
│  (–) preto    ──► GND (Protoboard)
└──────────────────────┘
```

---

## ⚙️ Configuração dos pinos

```cpp
#define SOLO_PIN    34   // Sensor umidade solo (analógico)
#define LDR_PIN     35   // Módulo LDR (analógico)
#define RELE_PIN    25   // Módulo relé (ajuste se necessário)
#define LED_R       26   // LED RGB — vermelho
#define LED_G       33   // LED RGB — verde
```

---

## 📶 WiFi com WiFiManager

O projeto usa a biblioteca **WiFiManager** para conexão WiFi sem precisar hardcodar credenciais.

1. No primeiro boot (ou após `wm.resetSettings()`), o ESP32 cria uma rede chamada **`IrrigaBot`**
2. Conecte seu celular/notebook nessa rede
3. Uma página de configuração abre automaticamente — insira o nome e senha do seu WiFi
4. O ESP32 salva as credenciais e conecta. O IP aparece no **Serial Monitor** (115200 baud)

---

## 🌐 API REST

Após conectar, acesse pelo IP exibido no Serial Monitor (ex: `http://192.168.1.100`).

### `GET /status`
Retorna os dados dos sensores em JSON:
```json
{
  "solo": 48,
  "luz": 72,
  "bomba": false,
  "auto": true,
  "limiteMin": 40,
  "limiteMax": 65
}
```

### `GET /pump?state=1`
Liga (`state=1`) ou desliga (`state=0`) a bomba manualmente.

### `GET /config?min=40&max=65&auto=1`
Atualiza os limites de irrigação e o modo automático.

---

## 🤖 Lógica de automação

```
A cada 10 segundos:
  lê umidade do solo (0–100%)

  se autoMode = true:
    se solo < limiteMin  →  liga bomba
    se solo >= limiteMax →  desliga bomba

  segurança:
    se bomba ligada por > 30s  →  desliga forçado
```

---

## 🌱 Limites recomendados por tipo de planta

| Planta | Ligar abaixo de | Desligar acima de |
|---|---|---|
| Temperos (manjericão, hortelã) | 40% | 70% |
| Suculentas / cactos | 15% | 35% |
| Flores (violeta, gerânio) | 40% | 60% |
| Hortaliças (alface, espinafre) | 55% | 75% |
| Mudas jovens | 60% | 80% |

---

## 📦 Dependências (Arduino IDE)

Instale via **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas**:

| Biblioteca | Autor |
|---|---|
| `WiFiManager` | tzapu |
| `WebServer` | *(incluída no ESP32 core)* |

**Board:** ESP32 Dev Module  
**Baudrate Serial Monitor:** 115200

---

## 🔆 Indicador LED

| Cor | Estado |
|---|---|
| 🔴 Vermelho | Bomba irrigando |
| 🟢 Verde | Solo no intervalo ideal |
| Apagado | Aguardando / fora do intervalo |

---

## 🔒 Segurança

- A bomba é desligada automaticamente após **30 segundos** mesmo se o sensor falhar
- O relé opera em lógica invertida (`LOW` = ligado) — a bomba começa **desligada** no boot
- CORS habilitado para permitir acesso pelo app web

---

## 📱 App web

O app de monitoramento foi desenvolvido no **Lovable** e consome a API REST do ESP32.

Configure a variável de ambiente no Lovable:
```
VITE_ESP_IP=http://192.168.x.x
```

Funcionalidades do app:
- Monitoramento em tempo real (atualiza a cada 5s)
- Seletor de tipo de planta com limites automáticos
- Controle manual da bomba
- Toggle de irrigação automática
- Sliders para ajuste dos limites
- Log de atividades

---

## 🗂️ Estrutura do código

```
irrigabot.ino
│
├── lerUmidadeSolo()     lê GPIO34, converte raw → %
├── lerLuminosidade()    lê GPIO35, converte raw → %
├── ligarBomba(bool)     aciona relé + LED indicador
│
├── handleStatus()       GET /status → JSON
├── handlePump()         GET /pump?state=
├── handleConfig()       GET /config?min=&max=&auto=
│
├── setup()              WiFiManager + rotas HTTP
└── loop()               server.handleClient() + lógica auto
```

---
