// ====================================================================
//           FIRMWARE - BIORREATOR DE CO-CULTURA MICROBIANA
// ====================================================================
// Autor: Guilherme Reis
// Data: 03/08/2025
// ====================================================================

// --- BIBLIOTECAS ---
#include <BluetoothSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>

// --- MAPEAMENTO DE PINOS (Conforme definido) ---
// Sensores
#define ONE_WIRE_BUS      4  // Sensor de Temperatura DS18B20 (Pino D4)
#define FLOW_SENSOR_PIN   15 // Sensor de Fluxo YF-S401 (Pino D15)

// Atuadores (Relés)
#define HEATER_PIN        23 // SSR do Aquecedor (Pino D23)
#define PUMP1_PIN         21 // Bomba 1 (Extração) (Pino D21)
#define PUMP2_PIN         14 // Bomba 2 (Refil) (Pino D14)
#define AERADOR_PIN       27 // Bomba de Aeração (Pino D27)
#define AGITADOR_PIN      26 // Agitador Magnético (Pino D26)

// --- OBJETOS E VARIÁVEIS GLOBAIS ---
// Bluetooth
BluetoothSerial SerialBT;
String deviceName = "Biorreator_IC_Final";
String btBuffer = ""; // Buffer para acumular dados do Bluetooth

// Sensores
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
volatile int pulseCount = 0;

// Parâmetros do Ciclo (recebidos do App)
float tempSetpoint = 30.0;
unsigned long tempoCicloTotalMs = 0;
float volumeRefilMl = 450.0;
float volumeExtracaoPercent = 90.0;

// Controle de Ciclo (Máquina de Estados)
enum EstadoCiclo { IDLE, ETAPA_1_CO_CULTURA, ETAPA_2_TRANSFERENCIA, ETAPA_3_PAUSA, ETAPA_4_REPOSICAO, FINALIZADO };
EstadoCiclo estadoAtual = IDLE;
unsigned long tempoInicioEtapa = 0;
String statusMessage = "Ocioso";

// Controle de Temperatura PID
double pidInput, pidOutput;
double Kp=255, Ki=2, Kd=0.1; // Seus valores de tuning validados!
PID myPID(&pidInput, &pidOutput, &tempSetpoint, Kp, Ki, Kd, DIRECT);
unsigned long pidWindowSize = 5000;
unsigned long pidWindowStartTime;

// Timers e Flags
unsigned long lastDataSend = 0;
unsigned long lastAeracaoToggle = 0;
bool aeradorLigado = false;
bool agitadorLigado = false;
bool bomba1Ligada = false;
bool bomba2Ligada = false;

// --- FUNÇÃO DE INTERRUPÇÃO (ISR) ---
// Chamada automaticamente a cada pulso do sensor de fluxo
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ====================================================================
//                             SETUP
// ====================================================================
void setup() {
  Serial.begin(115200);

  // Configura os pinos dos atuadores
  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(AERADOR_PIN, OUTPUT);
  pinMode(AGITADOR_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  
  // Garante que tudo comece DESLIGADO (Lógica Invertida para Low Level Trigger)
  digitalWrite(PUMP1_PIN, HIGH);
  digitalWrite(PUMP2_PIN, HIGH);
  digitalWrite(AERADOR_PIN, HIGH);
  digitalWrite(AGITADOR_PIN, HIGH);
  digitalWrite(HEATER_PIN, HIGH);

  // Inicialização dos Sensores e Comunicação
  sensors.begin();
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
  SerialBT.begin(deviceName);
  Serial.println("Firmware do Biorreator Final INICIADO. Aguardando conexão...");

  // Inicialização do PID
  pidWindowStartTime = millis();
  myPID.SetOutputLimits(0, pidWindowSize);
  myPID.SetMode(AUTOMATIC);
}

// ====================================================================
//                         FUNÇÕES AUXILIARES
// ====================================================================

// Envia o pacote de dados para o App
void sendDataPacket() {
    SerialBT.print("D;");
    SerialBT.print(pidInput, 1);
    SerialBT.print(";");
    SerialBT.print(agitadorLigado ? "1" : "0");
    SerialBT.print(";");
    SerialBT.print(aeradorLigado ? "1" : "0");
    SerialBT.print(";");
    SerialBT.print(bomba1Ligada ? "1" : "0");
    SerialBT.print(";");
    SerialBT.print(bomba2Ligada ? "1" : "0");
    SerialBT.print(";");
    SerialBT.print(statusMessage);
    SerialBT.print(";T\n"); // Marcador de fim de pacote
}

// Controla o aquecedor com base na saída do PID
void runPIDControl(unsigned long now) {
  sensors.requestTemperatures();
  pidInput = sensors.getTempCByIndex(0);

  if (pidInput != DEVICE_DISCONNECTED_C) {
    myPID.Compute();
    if (now - pidWindowStartTime > pidWindowSize) {
      pidWindowStartTime += pidWindowSize;
    }
    // Lógica Invertida para SSR Low Level Trigger
    if (pidOutput > (now - pidWindowStartTime)) {
      digitalWrite(HEATER_PIN, LOW); // LIGA
    } else {
      digitalWrite(HEATER_PIN, HIGH); // DESLIGA
    }
  } else {
    digitalWrite(HEATER_PIN, HIGH); // Segurança: desliga se o sensor falhar
  }
}

// Recebe e processa comandos do App
void handleBluetooth() {
  if (SerialBT.available()) {
    btBuffer += SerialBT.readString();

    // Procura pelo caractere de fim de comando
    int endIndex = btBuffer.indexOf(';');
    if (endIndex != -1 && btBuffer.startsWith("C;")) {
        // Encontrou um comando completo, ex: "C;30;240;450;90;"
        Serial.print("Comando Bruto Recebido: ");
        Serial.println(btBuffer);

        // Extrai os parâmetros
        String tempStr = btBuffer.substring(btBuffer.indexOf(';')+1, btBuffer.indexOf(';', 2));
        String tempoCicloStr = btBuffer.substring(btBuffer.indexOf(';', 2)+1, btBuffer.indexOf(';', 4));
        String volRefilStr = btBuffer.substring(btBuffer.indexOf(';', 4)+1, btBuffer.indexOf(';', 6));
        String volExtracaoStr = btBuffer.substring(btBuffer.indexOf(';', 6)+1, btBuffer.indexOf(';', 8));
        
        tempSetpoint = tempStr.toFloat();
        tempoCicloTotalMs = tempoCicloStr.toInt() * 3600000UL; // Converte horas para ms
        volumeRefilMl = volRefilStr.toFloat();
        volumeExtracaoPercent = volExtracaoStr.toFloat();

        // Inicia o ciclo
        estadoAtual = ETAPA_1_CO_CULTURA;
        tempoInicioEtapa = millis();
        Serial.println(">>> CICLO INICIADO <<<");
        
        btBuffer = ""; // Limpa o buffer
    } else if (btBuffer.length() > 50) {
        btBuffer = ""; // Segurança para limpar buffer inválido
    }
  }
}


// ====================================================================
//                              LOOP
// ====================================================================
void loop() {
  unsigned long now = millis();
  
  handleBluetooth(); // Ouve por comandos do App

  // Roda o PID continuamente, a menos que o ciclo esteja finalizado ou ocioso
  if(estadoAtual != IDLE && estadoAtual != FINALIZADO){
    runPIDControl(now);
  } else {
    digitalWrite(HEATER_PIN, HIGH); // Garante que o aquecedor esteja desligado
    pidInput = sensors.getTempCByIndex(0);
  }

  // --- MÁQUINA DE ESTADOS DO CICLO ---
  switch (estadoAtual) {
    case IDLE:
      statusMessage = "Ocioso";
      // Aguardando comando para iniciar
      break;

    case ETAPA_1_CO_CULTURA:
      statusMessage = "Etapa 1 - Co-cultura";
      agitadorLigado = true;
      digitalWrite(AGITADOR_PIN, LOW); // LIGA

      // A cada 2 minutos, liga a aeração por 30 segundos
      if (now - lastAeracaoToggle > 120000) { // 2 minutos
          aeradorLigado = !aeradorLigado;
          digitalWrite(AERADOR_PIN, aeradorLigado ? LOW : HIGH);
          lastAeracaoToggle = now;
      }
      
      // Verifica se o tempo da etapa acabou
      if (now - tempoInicioEtapa > tempoCicloTotalMs) {
        estadoAtual = ETAPA_2_TRANSFERENCIA;
        tempoInicioEtapa = now;
        pulseCount = 0; // Zera o contador para a medição
      }
      break;

    case ETAPA_2_TRANSFERENCIA:
      statusMessage = "Etapa 2 - Transferencia";
      bomba1Ligada = true;
      digitalWrite(PUMP1_PIN, LOW); // LIGA Bomba 1

      // Calcula o volume a ser extraído (90% do volume de refil, por exemplo)
      // Este cálculo pode ser mais sofisticado dependendo do volume total no reator
      float volumeExtracaoMl = volumeRefilMl * (volumeExtracaoPercent / 100.0);
      long pulsosNecessarios = (volumeExtracaoMl / 1000.0) * 5880; // YF-S401 calibration factor

      if (pulseCount >= pulsosNecessarios) {
        bomba1Ligada = false;
        digitalWrite(PUMP1_PIN, HIGH); // DESLIGA Bomba 1
        estadoAtual = ETAPA_3_PAUSA;
        tempoInicioEtapa = now;
      }
      break;

    case ETAPA_3_PAUSA:
      statusMessage = "Etapa 3 - Pausa";
      if (now - tempoInicioEtapa > 5000) { // Pausa de 5 segundos
        estadoAtual = ETAPA_4_REPOSICAO;
        tempoInicioEtapa = now;
        pulseCount = 0; // Zera o contador para a nova medição
      }
      break;

    case ETAPA_4_REPOSICAO:
      statusMessage = "Etapa 4 - Reposicao";
      bomba2Ligada = true;
      digitalWrite(PUMP2_PIN, LOW); // LIGA Bomba 2
      
      long pulsosRefil = (volumeRefilMl / 1000.0) * 5880;

      if (pulseCount >= pulsosRefil) {
        bomba2Ligada = false;
        digitalWrite(PUMP2_PIN, HIGH); // DESLIGA Bomba 2
        estadoAtual = FINALIZADO;
        tempoInicioEtapa = now;
      }
      break;
      
    case FINALIZADO:
      statusMessage = "Ciclo Finalizado";
      agitadorLigado = false;
      aeradorLigado = false;
      digitalWrite(AGITADOR_PIN, HIGH);
      digitalWrite(AERADOR_PIN, HIGH);
      // Fica neste estado até um novo comando
      break;
  }

  // Envia o pacote de dados para o App a cada 1 segundo
  if (now - lastDataSend > 1000) {
    sendDataPacket();
    lastDataSend = now;
  }
}
