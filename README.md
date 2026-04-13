

# 📘 README — Sistemas de Aquisição com ESP32

Este projeto contém três exercícios práticos utilizando o ESP32 para aquisição e processamento de sinais via ADC, explorando diferentes técnicas de bufferização:

* 🔁 Buffer Circular
* 🔄 Buffer Ping-Pong (Double Buffering)
* 📊 Média Móvel com Buffer Circular

---

## 📌 Objetivo Geral

Implementar sistemas de aquisição de dados com leitura periódica do ADC (a cada 10 ms), utilizando interrupções e estruturas eficientes de armazenamento, além de realizar processamento dos dados em tempo real.

---

# 🧩 Tarefa 1 — Buffer Circular

## 🎯 Objetivo

Implementar um buffer circular com as funções:

* `get_size()`
* `push()`
* `pop()`

## ⚙️ Funcionamento

* O sistema lê uma amostra do ADC a cada **10 ms** (via interrupção).
* Cada amostra é armazenada em um **buffer circular de 1000 posições**.
* Quando houver **64 amostras disponíveis**, o sistema:

  * Calcula a **potência média** dessas amostras.
* Caso uma amostra ultrapasse um **limiar definido**, um alerta deve ser exibido.
* Se houver tentativa de inserir ou remover dados além da capacidade:

  * A função deve **não executar a operação** e indicar isso no retorno.

## 🧠 Conceito-chave

Buffer circular:

* Reaproveita memória continuamente
* Usa índices `head` e `tail`
* Evita deslocamento de dados

---

# 🔄 Tarefa 2 — Buffer Ping-Pong (Double Buffering)

## 🎯 Objetivo

Implementar um sistema de buffer duplo para separar aquisição e processamento.

## ⚙️ Funcionamento

* Utiliza **dois buffers de 64 posições**:

  * Um para **escrita (ADC)**
  * Outro para **leitura (processamento)**
* O sistema:

  1. Recebe uma amostra do ADC a cada **10 ms** (interrupção)
  2. Armazena no buffer de escrita
  3. Quando o buffer enche:

     * Ele é marcado como **pronto**
     * Os buffers são **trocados**
* A `main`:

  * Processa o buffer pronto
  * Calcula a **potência média das 64 amostras**
* Se uma amostra ultrapassar o limiar → **alerta**
* Se o buffer de leitura não for liberado a tempo → **overflow**

## 🧠 Conceito-chave

Double Buffering:

* Permite aquisição contínua sem interrupção
* Evita conflito entre leitura e escrita
* Muito usado em sistemas em tempo real

---

# 📊 Tarefa 3 — Média Móvel com Buffer Circular

## 🎯 Objetivo

Calcular a **média móvel** de um sinal usando buffer circular.

## ⚙️ Funcionamento

* O sistema:

  * Lê uma amostra do ADC a cada **10 ms** (interrupção)
  * Insere no buffer com `push()`
* A média é calculada sobre uma **janela deslizante de 16 amostras**

### 🔄 Janela deslizante

* Se ainda não há 16 amostras:

  * Média é feita com as disponíveis
* Quando atingir 16:

  * Cada nova amostra:

    * Entra com `push()`
    * A mais antiga sai com `pop()`

## 🚨 Regras adicionais

* Se a média ultrapassar um **limiar definido**:

  * Exibir alerta
* Se o buffer estiver cheio e chegar nova amostra:

  * Sinalizar **overflow**

## 🧠 Conceito-chave

Média móvel:

* Suaviza o sinal
* Reduz ruído
* Mantém resposta em tempo real

---

# 🏗️ Estrutura Geral do Sistema

```
ADC (10 ms via interrupção)
        ↓
Estrutura de Buffer
        ↓
Processamento (main/task)
        ↓
Saída (média / alerta)
```

---


# ✅ Comparação entre as abordagens

| Técnica         | Vantagem                     | Uso ideal                    |
| --------------- | ---------------------------- | ---------------------------- |
| Buffer Circular | Simples e eficiente          | Fluxo contínuo de dados      |
| Ping-Pong       | Sem conflito leitura/escrita | Tempo real com processamento |
| Média Móvel     | Suavização de sinal          | Filtragem de ruído           |


