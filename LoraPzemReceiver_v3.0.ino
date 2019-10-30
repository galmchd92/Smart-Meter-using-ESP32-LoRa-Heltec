/* TCC - Medidor de consumo elétrico - PROJETO 8
/*
   Testes de utilização do ESP32 com o sensor PZEM
*/

// --------------------- RECEIVER ---------------------------

// --------------------- Bibliotecas utilizadas ---------------------------

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <SPI.h> //responsável pela comunicação serial
#include <LoRa.h> //responsável pela comunicação com o WIFI Lora
#include <Wire.h>  //responsável pela comunicação i2c
#include "SSD1306.h" //responsável pela comunicação com o display

// Variáveis para armazenar e tratar leituras do PZEM
  float v, i, p, e, consumo, custo, tempo, date = 0; 

// Definição das variáveis do APP Blynk
  float voltage_blynk=0;
  float current_blynk=0;
  float power_blynk=0;
  float energy_blynk=0;
  float consumo_blynk=0;
  float custo_blynk=0;
  float tempo_blynk=0;
  float date_blynk=0;
  float falta_blynk=0;

// Colocar o Auth Token disponível no Blynk App
/* Inserir o "Auth Token" obtido no Blynk App. Vá até "Project Settings" (ícone parafuso),
* entre "" a seguir.
* É enviado ao e-mail cadastrado ou pode ser copiado para área transferência SmartPhone.
* WiFi: Fazer o NodeMCU Lolin conectado na Rede,
* SSID e Senha devem ser escritos entre respectivas "" a seguir.
*/
  char auth[] = "ReTdIP_oDz5MDJ37-vQ3GLTrtRfV0DDM"; 

// Credenciais da Wifi Local
  char ssid[] = "Blues Power";         //colocar o ssid da rede
  char pass[] = "petrus12";          //colocar a senha da sua rede

// Configuração do tempo a ser medido
  unsigned long lastMillis = 0;

//NTP
  BlynkTimer timer;

//OTHERS
  unsigned long timerun, prun = 0; // Usado para contar o tempo completo de cada ciclo de loop

/*==================================================================================================================
* DEFIINIÇÃO DAS VARIÁVEIS DO LORA
*/
// Definição dos pinos 
  #define SCK     5    // GPIO5  -- SX127x's SCK
  #define MISO    19   // GPIO19 -- SX127x's MISO
  #define MOSI    27   // GPIO27 -- SX127x's MOSI
  #define SS      18   // GPIO18 -- SX127x's CS
  #define RST     14   // GPIO14 -- SX127x's RESET
  #define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

// Frequência do radio - podemos utilizar ainda : 433E6, 868E6, 915E6
  #define BAND    915E6  
  #define PABOOST true

// Variável responsável por armazenar o valor do contador (enviaremos esse valor para o outro Lora)
  unsigned int counter = 0;

// Parâmetros: address,SDA,SCL 
  SSD1306 display(0x3c, 4, 15); //construtor do objeto que controlaremos o display
  String rssi = "RSSI --";
  String packSize = "--";
  String packet ;

/*==================================================================================================================
* SETUP, INICIALIZAÇÃO DAS VARIÁVEIS, FUNÇÕES E BIBLIOTECAS
*/

void setup() {
  
//Setup OLED
  pinMode(16,OUTPUT); //RST do oled
  digitalWrite(16, LOW);    // Reseta o OLED
  delay(50); 
  digitalWrite(16, HIGH); // Enquanto o OLED estiver ligado, GPIO16 deve estar HIGH
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
  delay(1500);
  display.clear();

// Indica no display que inicilizou corretamente.
  display.drawString(20, 10, "Processo Iniciado com sucesso");
  display.drawString(20, 30, "Preparando Periféricos...");
  display.display();
  delay(000);

// Setup LoRa
  Serial.begin(115200); // Inicialização da comunicação serial
  SPI.begin(SCK,MISO,MOSI,SS); //inicia a comunicação serial com o Lora
  LoRa.setPins(SS,RST,DI00); //configura os pinos que serão utlizados pela biblioteca (deve ser chamado antes do LoRa.begin)

// Debug Lora
  Serial.println("Iniciando LoRa ...");           //Acompanhamento no serial
  display.clear();
  display.drawString(0, 0, "Iniciando LoRa...");
  display.display();
  delay(1000);
  
// Inicializa o Lora com a frequência específica.
  if (!LoRa.begin(BAND)) // PABOOST não funciona
  {
    Serial.println("Inicialização do LoRa falhou !");           //Acompanhamento no serial
    while (1);
  }
  delay(1000);
  
// Indica no display que inicilizou corretamente.
  Serial.println("LoRa iniciado com sucesso!");           //Acompanhamento no serial
  display.clear();
  display.drawString(0, 0, "LoRa iniciado com sucesso!");
  display.display();
  delay(1000);

//LoRa.onReceive(cbk);
  LoRa.receive(); // Habilita o Lora para receber dados

//NTP - Contagem do tempo
//  timerun = millis();

// Início do APP Blynk
  Serial.println("Iniciando Blink...");
  display.clear();
  display.drawString(0, 0, "Iniciando Blink...");
  display.display();
  delay(1000);
  Blynk.begin(auth, ssid, pass); //Faz a conexão com o Blynk
  Serial.println("Blink Iniciado!");
  display.clear();
  display.drawString(0, 0, "Blink Iniciado!");; //Imprime Sucesso na conexão...
  display.display();
  delay(500);
  
// Cálculo e apresentação do horário
 // prun = (millis() - timerun)/1000;
//  Serial.print(prun); Serial.println("s ");
// Inicialização do Blynk
  Blynk.run();
  Serial.println("Blink run...");
  display.clear();
  display.drawString(0, 0, "Blink run...");
  display.display();
  delay(1000);

    
//NTP ==== A verificar o seu funcionamento
timer.run();
Serial.println("timer run...");
}

/*==================================================================================================================
* LAÇO DE EXECUÇÃO DAS ROTINAS
*/

void loop() {

// Inicialização do Blynk
  Blynk.run();
//parsePacket: checa se um pacote foi recebido
//retorno: tamanho do pacote em bytes. Se retornar 0 (ZERO) nenhum pacote foi recebido
  int packetSize = LoRa.parsePacket();
//caso tenha recebido pacote chama a função para configurar os dados que serão mostrados em tela
  if (packetSize) { 
    cbk(packetSize);  
  }
}

//função responsável por recuperar o conteúdo do pacote recebido
//parametro: tamanho do pacote (bytes)
void cbk(int packetSize) {
  packet ="";
  packSize = String(packetSize,DEC); //transforma o tamanho do pacote em String para imprimirmos
  for (int i = 0; i < packetSize; i++) { 
    packet += (char) LoRa.read(); //recupera o dado recebido e concatena na variável "packet"
  }
  rssi = "RSSI=  " + String(LoRa.packetRssi(), DEC)+ "dB"; //configura a String de Intensidade de Sinal (RSSI)
  //mostrar dados em tela
  loraData();
}


//função responsável por configurar os dadosque serão exibidos em tela.
//RSSI : primeira linha
//RX packSize : segunda linha
//packet : terceira linha
void loraData(){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0 , 18 , "Adquirido "+ packSize + " bytes de dados");
  display.drawString(0 , 39 , packet);
  display.drawString(0, 0, rssi);  
  display.display();
  blynk();
}

// -------------------------------------- BLYNK ----------------------------------------
void blynk (){
  int tamanho = packet.length(); // Indica o tamanho total do pacote recebido/posição do final
  // stringaAProcurar.indexOf(stringProcurada) 
  // Verifica qual é o parametro/ o número resultante indica a posição que a variável foi achada
  // Se o retorno for -1 que dizer que não foi encontrado
  int teste = packet.indexOf("(V)");
  Blynk.virtualWrite(V9, 0); // apenas preparação pra futuro evento de falta de energia 
  if (teste > -1) 
      {
          // String procurada encontrada
          int doispontos = packet.indexOf(": "); // Procura 2 posições anteriores ao valor do parametro pois o valor sempre estará após um : e um espaço (2 posições)
          String voltage_blynk = packet.substring(doispontos + 2, tamanho); // Monta o valor que existe duas posições apó o : até a posição ao final do pacote
          Serial.println("Tensão recebida");
          voltage_blynk.toFloat();
          Serial.println(voltage_blynk);
          Blynk.virtualWrite(V1, voltage_blynk); // Envia o valor para o Blynk
      }
  teste = packet.indexOf("(A)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String current_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Corrente recebida");
          current_blynk.toFloat();
          Serial.println(current_blynk);
          Blynk.virtualWrite(V2, current_blynk);
      }
  teste = packet.indexOf("(W)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String power_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Potência recebida");
          power_blynk.toFloat();
          Serial.println(power_blynk);
          Blynk.virtualWrite(V3, power_blynk);
      }
  teste = packet.indexOf("(kWh)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String energy_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Energia recebida");
          energy_blynk.toFloat();
          Serial.println(energy_blynk);
          Blynk.virtualWrite(V4, energy_blynk);
      }
  teste = packet.indexOf("(kWh/mês)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String consumo_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Consumo recebido");
          consumo_blynk.toFloat();
          Serial.println(consumo_blynk);
          Blynk.virtualWrite(V5, consumo_blynk);
      }
  teste = packet.indexOf("(R$/mês)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String custo_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Custo mensal recebido");
          custo_blynk.toFloat();
          Serial.println(custo_blynk);
          Blynk.virtualWrite(V6, custo_blynk);
      }
  teste = packet.indexOf("(DD/MM/YYYY)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String tempo_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Data recebida");
          Serial.println(tempo_blynk);
          //Blynk.virtualWrite(V7, tempo_blynk);
      }
  teste = packet.indexOf("(HH:MM:SS)");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String date_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Horário recebido");
          Serial.println(date_blynk);
          //Blynk.virtualWrite(V8, date_blynk);
      }
  teste = packet.indexOf("Falta de energia");
  if (teste > -1)
      {
          int doispontos = packet.indexOf(": ");
          String falta_blynk = packet.substring(doispontos + 2, tamanho);
          Serial.println("Falta de energia detectada");
          Blynk.virtualWrite(V9, 255); //Enviar 255 pra setar o LED em NL alto e 0 pra NL baixo
          Blynk.virtualWrite(V1, 0.0); // Atualizar o valor da tensão pra 0
          Blynk.virtualWrite(V2, 0.0); // Atualizar o valor da corrente pra 0
      }
}


// EVENTOR -- Colocar o Push Notification para notificação no celular
  // Utilizar o V6 = Custo mensal para definir o limite do consumo mensal
  // Utilizar o V9 = Falta de Energia para informar ao cliente uma possível falta de energia no local
 
// TESTAR O WIDGET RTC
  // https://github.com/blynkkk/blynk-library/blob/master/examples/Widgets/RTC/RTC.ino
 
