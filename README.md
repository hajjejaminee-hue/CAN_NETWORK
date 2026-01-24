# CAN Network - Réseau de Communication CAN avec STM32F407

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![STM32](https://img.shields.io/badge/STM32-F407VGTx-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f4-series.html)
[![CAN](https://img.shields.io/badge/Protocol-CAN-orange.svg)](https://en.wikipedia.org/wiki/CAN_bus)
[![MQTT](https://img.shields.io/badge/Protocol-MQTT-green.svg)](https://mqtt.org/)
[![Node-RED](https://img.shields.io/badge/Dashboard-Node--RED-red.svg)](https://nodered.org/)

## 📋 Description

Ce projet implémente un réseau de communication **CAN (Controller Area Network)** distribué utilisant plusieurs microcontrôleurs **STM32F407VGTx**. Le système est composé de trois nœuds spécialisés qui communiquent via le bus CAN pour échanger des données de capteurs et effectuer des mesures.

### Fonctionnalités principales

- 🔄 Communication CAN bidirectionnelle à 250 kbit/s
- 📊 Acquisition de données ADC multi-canaux (3 canaux)
- 🌡️ Mesure de température via capteur I2C (DS1621)
- 🎛️ Nœud maître avec interface UART pour contrôle et monitoring
- 🔌 Support de requêtes/réponses CAN

## 🏗️ Architecture

Le projet est organisé en trois nœuds CAN distincts, avec une extension IoT pour la visualisation cloud :

```
┌─────────────────┐
│   Master_Node   │  ← Nœud maître (coordinateur)
│  (STM32F407)    │
└────┬──────┬─────┘
     │      │
     │ CAN  │ UART6
     │      │
     │  ┌───▼──────┐
     │  │  ESP32   │  ← Gateway IoT
     │  │ (WiFi)   │
     │  └────┬─────┘
     │       │ MQTT
     │       │
     │  ┌────▼──────────┐
     │  │ Mosquitto     │  ← Broker MQTT
     │  │ (test.mosq...)│
     │  └────┬──────────┘
     │       │ MQTT
     │  ┌────▼──────────┐
     │  │  Node-RED     │  ← Dashboard
     │  │  Dashboard    │
     │  └───────────────┘
     │
     │ CAN Bus (250 kbit/s)
     │
┌────┴────┐
│         │
┌───▼───┐ ┌──▼────┐
│ADC_Node│ │I2C_Node│
│(STM32) │ │(STM32) │
└────────┘ └────────┘
```

### Nœuds du réseau

#### 1. **Master_Node** 🎛️
- **Rôle** : Nœud coordinateur du réseau
- **Fonctions** :
  - Envoi de requêtes CAN aux nœuds esclaves
  - Réception et traitement des données des nœuds
  - Interface UART pour commandes et monitoring (115200 baud)
  - Agrégation des données ADC et température
  - Transmission des données agrégées vers ESP32 via UART6
- **Technologie** : STM32CubeIDE + HAL Library
- **Interfaces** :
  - CAN1 (PB8/PB9)
  - USART3 (debug/commande)
  - **USART6 (PC6/PC7)** : Communication avec ESP32 pour MQTT

#### 2. **ADC_Node** 📊
- **Rôle** : Acquisition de données analogiques
- **Fonctions** :
  - Acquisition ADC sur 3 canaux (channels 4, 5, 6)
  - Conversion DMA pour acquisition continue
  - Réponse aux requêtes CAN avec valeurs ADC
- **Technologie** : Keil uVision + Standard Peripheral Library
- **Interfaces** :
  - CAN1 (PB8/PB9)
  - ADC1 avec DMA2
  - TIM2 (trigger ADC)
  - LEDs de statut (PD12/13/14)

#### 3. **I2C_Node** 🌡️
- **Rôle** : Mesure de température
- **Fonctions** :
  - Lecture température via capteur DS1621 (I2C)
  - Réponse aux requêtes CAN avec valeur de température
- **Technologie** : Keil uVision + Standard Peripheral Library
- **Interfaces** :
  - CAN1 (PB8/PB9)
  - I2C1 (PB6/PB7) pour DS1621
  - USART3 (debug)

## 🔌 Configuration Matérielle

### Matériel requis

- 3x STM32F407VGTx Discovery ou cartes compatibles
- 2x ESP32 (ESP32-WROOM-32 ou compatible) - Un pour ADC, un pour I2C
- Transceivers CAN (ex: MCP2551, TJA1050)
- Capteur de température DS1621 (pour I2C_Node)
- Résistances de terminaison CAN (120Ω aux extrémités du bus)
- Câbles CAN (paire torsadée recommandée)
- Accès Wi-Fi pour les ESP32

### Connexions CAN

Tous les nœuds partagent le même bus CAN :

| Signal | Pin STM32 | Description |
|--------|-----------|-------------|
| CAN_TX | PB8       | Transmission CAN |
| CAN_RX | PB9       | Réception CAN |
| CAN_H  | Transceiver | Ligne CAN haute |
| CAN_L  | Transceiver | Ligne CAN basse |

### Connexions I2C_Node (DS1621)

| Signal DS1621 | Pin STM32 | Description |
|---------------|-----------|-------------|
| SDA           | PB7       | Données I2C |
| SCL           | PB6       | Horloge I2C |
| VDD           | 3.3V      | Alimentation |
| GND           | GND       | Masse |
| A0, A1, A2    | GND       | Adresse 0x48 |

### Connexions STM32-ESP32 (UART6)

**Note** : Le Master_Node peut communiquer avec deux ESP32 différents (un pour ADC, un pour I2C) ou utiliser un seul ESP32 qui gère les deux types de données.

| Signal | Pin STM32 | Pin ESP32 | Description |
|--------|-----------|-----------|-------------|
| TX     | PC6 (USART6_TX) | GPIO26 (RX2) | Transmission STM32 → ESP32 |
| RX     | PC7 (USART6_RX) | GPIO25 (TX2) | Réception STM32 ← ESP32 |
| GND    | GND       | GND       | Masse commune |
| 3.3V   | 3.3V      | 3.3V      | Alimentation (optionnel) |

**Note** : Les niveaux logiques sont compatibles (3.3V pour les deux). Vitesse : 115200 baud, 8N1.

## 📡 Protocole CAN

### Configuration du bus

- **Vitesse** : 250 kbit/s
- **Mode** : Normal
- **Prescaler** : 2
- **Bit Timing** : BS1=10TQ, BS2=5TQ, SJW=1TQ

### Identifiants CAN (Standard ID)

| ID CAN | Direction | Description | Format |
|--------|-----------|-------------|--------|
| `0x60` | Master → ADC | Filtre de réception ADC_Node | - |
| `0x61` | ADC → Master | Données canal ADC 4 | `[0x61, LSB, MSB]` |
| `0x62` | ADC → Master | Données canal ADC 5 | `[0x62, LSB, MSB]` |
| `0x63` | ADC → Master | Données canal ADC 6 | `[0x63, LSB, MSB]` |
| `0x100` | ADC → Master | Réponse ADC (ID de réponse) | `[Channel, LSB, MSB]` |
| `0x550` | Master → I2C | Requête température | - |
| `0x681` | I2C → Master | Réponse température | `[ MSB, LSB]` |

### Format des messages

#### Requête ADC (Master → ADC_Node)
```
ID: 0x61, 0x62, ou 0x63
DLC: 1-3
Data[0]: Identifiant du canal (0x61, 0x62, 0x63)
```

#### Réponse ADC (ADC_Node → Master)
```
ID: 0x100
DLC: 3
Data[0]: Identifiant du canal (0x61, 0x62, 0x63)
Data[1]: LSB de la valeur ADC (8 bits)
Data[2]: MSB de la valeur ADC (8 bits)
```

#### Requête Température (Master → I2C_Node)
```
ID: 0x550
DLC: Variable
```

#### Réponse Température (I2C_Node → Master)
```
ID: 0x681
DLC: 3
Data[1]: MSB de la valeur de température (8 bits)
Data[2]: LSB de la valeur de température (8 bits)
```

### 🔍 Importance des Masques ID CAN (Filtrage des Trames)

Les **masques ID CAN** sont essentiels pour optimiser le traitement des messages et réduire la charge CPU. Ils permettent de filtrer uniquement les trames souhaitées au niveau matériel, avant même qu'elles n'atteignent le CPU.

#### Principe du Filtrage CAN

Le contrôleur CAN STM32 dispose de **filtres matériels** qui peuvent accepter ou rejeter des messages en fonction de leur ID. Le filtrage se fait en mode **ID/Mask** :

- **ID** : L'identifiant CAN à filtrer (ou base d'identifiants)
- **Mask** : Le masque qui définit quels bits de l'ID sont significatifs
  - **Bit à 1 dans le masque** : Le bit correspondant dans l'ID reçu doit correspondre exactement
  - **Bit à 0 dans le masque** : Le bit correspondant est ignoré (don't care)

#### Configuration dans ADC_Node

```c
// ADC_Node - Filtre pour accepter 3 IDs : 0x61, 0x62, 0x63
Filter_InitStruct.CAN_FilterIdHigh = (0x60 << 5);  // Base ID : 0x60
Filter_InitStruct.CAN_FilterMaskIdHigh = 0x7FC << 5;  // Masque : 0x7FC

// Analyse du masque 0x7FC (binaire : 0111 1111 1100)
// Bits significatifs : 0x7FC = 0b011111111100
// Cela signifie que les bits 2-9 doivent correspondre
// Les bits 0-1 sont ignorés, permettant d'accepter :
// - 0x60 + 0x01 = 0x61 (canal ADC 4)
// - 0x60 + 0x02 = 0x62 (canal ADC 5)  
// - 0x60 + 0x03 = 0x63 (canal ADC 6)
```

**Avantages** :
- ✅ Un seul filtre accepte 3 IDs différents
- ✅ Réduction de la charge CPU (pas de vérification logicielle)
- ✅ Messages non pertinents rejetés au niveau matériel
- ✅ FIFO non encombré par des messages inutiles

#### Configuration dans Master_Node

```c
// Master_Node - Filtre pour accepter TOUS les messages
canfilterconfig.FilterIdHigh = 0x000;
canfilterconfig.FilterMaskIdHigh = 0x000;  // Masque = 0 : accepte tout
canfilterconfig.FilterMaskIdLow = 0x0000;
```

**Pourquoi accepter tout ?** :
- Le Master_Node est le coordinateur et doit recevoir **tous** les messages
- Il traite ensuite les messages selon leur ID dans le code logiciel
- Permet la flexibilité pour ajouter de nouveaux nœuds sans modifier les filtres

#### Comparaison des Approches

| Nœud | Masque | IDs Acceptés | Avantage |
|------|--------|--------------|----------|
| **ADC_Node** | `0x7FC << 5` | 0x61, 0x62, 0x63 | Filtrage précis, économie CPU |
| **I2C_Node** | Configuré pour 0x550 | 0x550 uniquement | Filtrage strict pour requêtes |
| **Master_Node** | `0x000` (tout) | Tous les IDs | Flexibilité maximale |

#### Impact sur les Performances

**Sans masque (filtrage logiciel uniquement)** :
- ❌ Tous les messages arrivent dans le FIFO
- ❌ Le CPU doit vérifier chaque ID
- ❌ Risque de débordement du FIFO
- ❌ Consommation CPU élevée

**Avec masque (filtrage matériel)** :
- ✅ Seuls les messages pertinents arrivent dans le FIFO
- ✅ Le CPU ne traite que les messages utiles
- ✅ Protection contre le débordement
- ✅ Consommation CPU minimale

#### Exemple de Calcul de Masque

Pour accepter les IDs `0x61`, `0x62`, `0x63` :

```
0x61 = 0b01100001
0x62 = 0b01100010
0x63 = 0b01100011
      ^^^^^^^^
      Bits communs : 0b0110000X (les 2 derniers bits varient)

Masque = 0b1111111100 = 0x7FC
        (1 = bit significatif, 0 = bit ignoré)
```

**Résultat** : Le filtre accepte tous les IDs de `0x60` à `0x63`, mais seul `0x61`, `0x62`, `0x63` sont utilisés dans le code.

## 🚀 Installation et Compilation

### Prérequis

- **STM32CubeIDE** (version 1.13.0 ou supérieure) pour Master_Node
- **Keil MDK-ARM** (version 5.29 ou supérieure) pour ADC_Node et I2C_Node
- **PlatformIO** (pour ESP32) - Extension VS Code ou CLI
- **STM32CubeMX** ( pour configuration Master_Node)
- **ST-Link Utility**  pour le flashage STM32
- **Node.js** et **Node-RED** (pour le dashboard)
- **Broker MQTT** : `test.mosquitto.org` (public) ou Mosquitto local

### Compilation

#### Master_Node

```bash
cd Master_Node
# Ouvrir le projet dans STM32CubeIDE
# Build: Project → Build All (Ctrl+B)
```

#### ADC_Node

```bash
cd ADC_Node
# Ouvrir ADC_Node.uvprojx dans Keil uVision
# Build: Project → Build Target (F7)
```

#### I2C_Node

```bash
cd I2C_Node
# Ouvrir le projet dans Keil uVision
# Build: Project → Build Target (F7)
```

### Flashage

1. Connecter la carte STM32 via ST-Link
2. Dans l'IDE :
   - **STM32CubeIDE** : Run → Debug (F11) ou Run → Run (Ctrl+F11)
   - **Keil uVision** : Flash → Download (F8)

## 💻 Utilisation

### Démarrage du système

1. **Alimenter tous les nœuds** (3.3V ou via USB)
2. **Vérifier les terminaisons CAN** (120Ω aux extrémités)
3. **Connecter le transceiver CAN** sur chaque nœud
4. **Programmer chaque nœud** avec le firmware correspondant

### Utilisation du Master_Node

Le Master_Node expose une interface UART (115200 baud) pour envoyer des commandes CAN :

#### Format de commande

```
send <id_hex> <data|remote> <dlc> <bytes..>
```

#### Exemples

```bash
# Envoyer une requête pour le canal ADC 4
send 61 data 1 61

# Envoyer une requête de température
send 550 data 1 00

```

#### Réception des données

Le Master_Node affiche automatiquement :
- Tous les messages CAN reçus (format détaillé)
- Les valeurs ADC agrégées (format CSV: `V1,V2,V3\n`)
- Les valeurs de température formatées (`Temp = X.X C\n`)

**Transmission vers ESP32** :
- Les données ADC sont envoyées via UART6 au format CSV dès que les 3 canaux sont reçus
- Les données de température sont également transmises via UART6
- Format : `V1,V2,V3\r\n` ou `Temp = X.X C\r\n`


### Visualisation Cloud (MQTT + Node-RED)

#### Configuration ESP32 ADC

1. **Modifier les credentials Wi-Fi** dans `ESP_ADC/Stm_ESP_ADC/src/main.cpp` :
   ```cpp
   const char* ssid     = "VOTRE_SSID";
   const char* password = "VOTRE_MOT_DE_PASSE";
   ```

2. **Compiler et flasher l'ESP32** avec PlatformIO :
   ```bash
   cd ESP_ADC/Stm_ESP_ADC
   pio run -t upload
   ```

3. **Vérifier la connexion** :
   - L'ESP32 se connecte au Wi-Fi
   - Connexion au broker MQTT `test.mosquitto.org:1883`
   - Publication automatique des données ADC reçues via UART

#### Configuration ESP32 I2C

1. **Modifier les credentials Wi-Fi** dans `ESP_I2C/ESP_STM/src/main.cpp` :
   ```cpp
   const char* ssid     = "VOTRE_SSID";
   const char* password = "VOTRE_MOT_DE_PASSE";
   ```

2. **Compiler et flasher l'ESP32** avec PlatformIO :
   ```bash
   cd ESP_I2C/ESP_STM
   pio run -t upload
   ```

3. **Vérifier la connexion** :
   - L'ESP32 se connecte au Wi-Fi
   - Connexion au broker MQTT `test.mosquitto.org:1883`
   - Publication automatique des données de température reçues via UART

#### Configuration Node-RED

1. **Installer Node-RED** :
   ```bash
   npm install -g node-red
   node-red
   ```

2. **Installer le nœud MQTT** (déjà inclus dans Node-RED)

3. **Créer un flow Node-RED** :
   - **Nœud MQTT In** : Se connecter à `test.mosquitto.org:1883`
   - **S'abonner aux topics** :
     - `stm32/pot/V1`
     - `stm32/pot/V2`
     - `stm32/pot/V3`
   - **Nœuds Dashboard** : Ajouter des gauges, graphiques, etc.
   - **Nœud Dashboard UI** : Interface web sur `http://localhost:1880/ui`

4. **Exemple de flow Node-RED** :
   ```json
   [
     {
       "id": "mqtt-in",
       "type": "mqtt in",
       "topic": "stm32/pot/V1",
       "broker": "mosquitto-broker",
       "name": "ADC Channel 1"
     },
     {
       "id": "gauge",
       "type": "ui_gauge",
       "topic": "stm32/pot/V1",
       "name": "V1",
       "group": "dashboard-group"
     }
   ]
   ```

#### Accès au Dashboard

- Ouvrir un navigateur : `http://localhost:1880/ui`
- Visualiser les données en temps réel
- Les valeurs ADC sont mises à jour automatiquement via MQTT

## 📁 Structure du Projet

```
CAN_Network/
├── Master_Node/              # Nœud maître (STM32CubeIDE)
│   ├── Core/
│   │   ├── Inc/              # En-têtes
│   │   └── Src/              # Sources (main.c, etc.)
│   ├── Drivers/              # HAL Drivers STM32
│   └── CAN_Normalmode.ioc    # Configuration CubeMX
│
├── ADC_Node/                 # Nœud ADC (Keil uVision)
│   ├── main.c
│   ├── ADC_Dma.c/h           # Gestion ADC + DMA
│   ├── stm32f4xx_can.c/h     # Driver CAN
│   └── ADC_Node.uvprojx      # Projet Keil
│
├── I2C_Node/                 # Nœud I2C/Température (Keil uVision)
│   └── projet/
│       ├── main.c
│       └── ...
│
├── ESP_ADC/                  # Gateway ESP32 pour ADC (PlatformIO)
│   └── Stm_ESP_ADC/
│       ├── src/
│       │   └── main.cpp      # Code ESP32 ADC (WiFi + MQTT)
│       ├── platformio.ini    # Configuration PlatformIO
│       └── .vscode/          # Configuration VS Code
│
├── ESP_I2C/                  # Gateway ESP32 pour I2C/Température (PlatformIO)
│   └── ESP_STM/
│       ├── src/
│       │   └── main.cpp      # Code ESP32 I2C (WiFi + MQTT)
│       ├── platformio.ini    # Configuration PlatformIO
│       └── .vscode/          # Configuration VS Code
│
      
```

## 🛠️ Technologies Utilisées

- **Microcontrôleurs** :
  - STM32F407VGTx (ARM Cortex-M4, 168 MHz)
  - ESP32 (Wi-Fi )
- **Protocoles** :
  - CAN 2.0 (250 kbit/s)
  - MQTT (Message Queuing Telemetry Transport)
  - UART (115200 baud, 8N1)
- **Libraries** :
  - STM32 HAL Library (Master_Node)
  - STM32 Standard Peripheral Library (ADC_Node, I2C_Node)
  - Arduino WiFi (ESP32)
  - PubSubClient (MQTT client pour ESP32)
- **Outils de développement** :
  - STM32CubeIDE
  - Keil MDK-ARM
  - PlatformIO (ESP32)
  - ST-Link
  - Node-RED (Dashboard)
  - Mosquitto MQTT Broker


```


## 📝 Notes de Développement

- Le Master_Node utilise **HAL Library** (STM32CubeIDE)
- ADC_Node et I2C_Node utilisent **Standard Peripheral Library** (Keil)
- Les deux approches sont compatibles au niveau CAN
- Les filtres CAN sont configurés différemment selon la library utilisée
- **Format de données UART6** : Les données ADC sont envoyées au format CSV `V1,V2,V3\r\n` dès que les 3 canaux sont reçus
- **Broker MQTT** : Le projet utilise `test.mosquitto.org` (broker public). Pour la production, utiliser un broker privé avec authentification
- **Topics MQTT** : Les topics suivent la hiérarchie `stm32/pot/Vx` pour faciliter la souscription avec des wildcards (`stm32/pot/#`)
- **Masques CAN** : L'utilisation de masques ID CAN est cruciale pour optimiser les performances. Voir la section "🔍 Importance des Masques ID CAN" pour plus de détails sur le filtrage matériel
- **Masques CAN** : L'utilisation de masques ID CAN est cruciale pour optimiser les performances. Voir la section "Importance des Masques ID CAN" pour plus de détails


