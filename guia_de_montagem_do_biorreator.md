# Guia de Montagem — Biorreator Automatizado (ESP32)

> [!IMPORTANT]
> **Regra Obrigatória (Terra Comum):** Antes de energizar o sistema, conecte o **GND da fonte** a um pino **GND do ESP32**. Sem essa referência comum, os sinais de controle dos relés não funcionarão corretamente.

---

## 1. Sensores (Direto no ESP32)

| Componente | Fio / Pino do Sensor | Pino no ESP32 | Detalhe Crítico |
| :--- | :--- | :--- | :--- |
| **Temperatura (DS18B20)** | Vermelho (VCC)<br>Preto (GND)<br>Amarelo (Dados) | `3V3`<br>`GND`<br>`D4` (GPIO 4) | Inserir resistor *pull-up* de $4.7\text{ k}\Omega$ entre o fio de Dados (`D4`) e o `3V3`. |
| **Turbidez** | VCC<br>GND<br>Sinal (Analógico) | `3V3`<br>`GND`<br>`D34` (GPIO 34) | Pino exclusivo de entrada analógica. |
| **Fluxo (YF-S401)** | Vermelho (VCC)<br>Preto (GND)<br>Amarelo (Sinal) | `VIN` (5V)<br>`GND`<br>`D15` (GPIO 15) | Instalar em linha na mangueira de saída da bomba dosadora. |

---

## 2. Módulo Relé Mecânico (Atuadores 12V)

### A) Sinal e Alimentação (ESP32 -> Módulo Relé)
*Mantenha o jumper entre `VCC` e `JD-VCC` encaixado na placa do módulo.*

| Pino no Módulo Relé | Pino no ESP32 | Atuador Controlado |
| :--- | :--- | :--- |
| `VCC` | `VIN` (5V) | Alimentação das bobinas |
| `GND` | `GND` | Terra comum |
| `IN1` | `D21` (GPIO 21) | Bomba Peristáltica 1 |
| `IN2` | `D12` (GPIO 12) | Bomba Peristáltica 2 |
| `IN3` | `D27` (GPIO 27) | Bomba de Aeração |
| `IN4` | `D26` (GPIO 26) | Agitador Magnético (Fan) |

### B) Potência 12V (Repetir para cada um dos 4 atuadores)
* **Positivo `+12V` (Fonte)** -> borne **`COM`** do relé.
* **Borne `NO` (Relé)** -> fio **Positivo (`+`)** do atuador.
* **Fio Negativo (`-`) do atuador** -> **`GND` (Fonte 12V)**.

---

## 3. Módulo SSR e Resistências (Aquecimento 110V)

### A) Controle (ESP32 -> Módulo SSR)
* **`3V3` (ESP32)** -> **`DC+`** (SSR)
* **`GND` (ESP32)** -> **`DC-`** (SSR)
* **`D23` (GPIO 23)** -> **`CH1`** (SSR)

### B) Potência em Série (Tomada 110V -> SSR -> Resistências)
1. Fio **FASE** da tomada -> parafuso 1 do **Canal [1]** do SSR.
2. Parafuso 2 do **Canal [1]** do SSR -> terminal 1 da **Resistência 1**.
3. Terminal 2 da **Resistência 1** -> terminal 1 da **Resistência 2** *(ponte em série)*.
4. Terminal 2 da **Resistência 2** -> fio **NEUTRO** da tomada.

```text
[Tomada FASE] ---> [SSR Canal 1] ---> [Resistência 1] ---> [Resistência 2] ---> [Tomada NEUTRO]
```

---

## 4. Resumo das Fontes de Alimentação

* **ESP32:** Alimentado via cabo USB (5V).
* **Módulo Relé Mecânico + Sensor YF-S401:** Alimentados pelo pino **`VIN`** (5V) do ESP32.
* **Módulo SSR + Sensores DS18B20 e Turbidez:** Alimentados pelo pino **`3V3`** do ESP32.
* **Bombas e Fan:** Alimentados pela **Fonte Chaveada 12V** (chaveados pelos relés).

---

## 5. Checklist Rápido Antes de Ligar

- [ ] O `GND` da fonte 12V está interligado ao `GND` do ESP32?
- [ ] O resistor de $4.7\text{ k}\Omega$ está conectado entre `D4` e `3V3` no sensor DS18B20?
- [ ] O jumper `VCC / JD-VCC` está no lugar no módulo relé mecânico?
- [ ] Todas as conexões de 110V (SSR e resistências) estão firmes e bem isoladas?
