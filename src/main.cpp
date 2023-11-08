#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// const char *ssid = "belinha13";
// const char *ssid = "belinha13";
// const char *password = "145312mau";

const char *password = "admin1234";
const char *ssid = "admin";

// const char *serverAddress = "192.168.15.3"; // IP do seu PC
const char *serverAddress = "picazoapi-production.up.railway.app"; // IP do seu PC
const int serverPort = 3000;                                       // Porta da API em seu PC

// Variáveis globais para armazenar os valores dos campos do JSON
bool isAProgressPainting = true;
bool isPaintPaused;
int plataformHeight;
int plataformWidth;
int plataformToRobot;
int percentStatus;
int newPercentStatus;
int lastPaintId; // Variável global para armazenar a resposta da última pintura

String lastPaintResponse; // Variável global para armazenar a resposta da última pintura

// Definir os pinos dos motores
const int motor1Pin1 = 32; // Pino 32 para motor 1
const int motor1Pin2 = 33; // Pino 33 para motor 1

const int motor2Pin1 = 27; // Pino 27 para motor 2
const int motor2Pin2 = 26; // Pino 26 para motor 2

const int motor3Pin1 = 12; // Pino 12 para motor 2
const int motor3Pin2 = 13; // Pino 13 para motor 2

const int sparyPin1 = 14; // Pino 14 para rele spray

bool shouldUpdatePercentStatus = false;

void fetchRobot()
{
  // Realize a requisição HTTP para obter o ID da última pintura do robô 310
  String robotUrl = "https://" + String(serverAddress) + "/robot/findOneRobot/" + 1;
  HTTPClient robotRequest;
  robotRequest.begin(robotUrl);
  int httpCodeRobotRequest = robotRequest.GET();

  if (httpCodeRobotRequest == HTTP_CODE_OK)
  {
    String robotResponse = robotRequest.getString();
    Serial.println("Resposta JSON do Robô:");
    Serial.println(robotResponse);

    DynamicJsonDocument robotDoc(1024); // Ajuste o tamanho conforme necessário
    DeserializationError errorRobot = deserializeJson(robotDoc, robotResponse);

    if (!errorRobot)
    {
      String lastPaintIdValue = robotDoc[0]["lastPaintId"];
      Serial.println("Última Pintura ID: " + lastPaintIdValue);

      if (!lastPaintIdValue.isEmpty())
      {
        lastPaintId = lastPaintIdValue.toInt();
      }
    }
    else
    {
      Serial.println("Erro ao analisar JSON da resposta do robô");
    }
  }
  else
  {
    Serial.println("Erro na requisição HTTP do robô");
  }

  robotRequest.end();
}

void fetchPaint()
{
  // Use o ID da última pintura para obter informações específicas da pintura
  String paintUrl = "https://" + String(serverAddress) + "/paint-config/" + lastPaintId;
  HTTPClient paintRequest;
  paintRequest.begin(paintUrl);
  // Verifique e imprima os dados da última pintura no loop principal

  int httpCodePaintRequest = paintRequest.GET();

  if (httpCodePaintRequest == HTTP_CODE_OK)
  {
    String paintResponse = paintRequest.getString();
    Serial.println("Resposta JSON da Pintura Específica:");
    Serial.println(paintResponse);

    // Armazene a resposta da pintura específica na variável global
    lastPaintResponse = paintResponse;

    if (!lastPaintResponse.isEmpty())
    {
      // Agora acessa o campo após desserializar
      DynamicJsonDocument paintDoc(1024); // Ajuste o tamanho conforme necessário
      DeserializationError errorPaint = deserializeJson(paintDoc, lastPaintResponse);

      if (!errorPaint)
      {

        isAProgressPainting = paintDoc[0]["isAProgressPainting"];
        isPaintPaused = paintDoc[0]["isPaintPaused"];
        plataformHeight = paintDoc[0]["plataformHeight"];
        plataformWidth = paintDoc[0]["plataformWidth"];
        plataformToRobot = paintDoc[0]["plataformToRobot"];
        percentStatus = paintDoc[0]["percentStatus"];
      }
      else
      {
        Serial.println("Erro ao analisar JSON da resposta da Pintura Específica");
      }
    }
  }
  else
  {
    Serial.println("Erro na requisição HTTP da Pintura Específica");
  }

  paintRequest.end();

  Serial.println("Current isAProgressPainting: " + String(isAProgressPainting));
  Serial.println("Current isPaintPaused: " + String(isPaintPaused));
  Serial.println("Current plataformHeight: " + String(plataformHeight));
  Serial.println("Current plataformWidth: " + String(plataformWidth));
  Serial.println("Current plataformToRobot: " + String(plataformToRobot));
  Serial.println("Current percentStatus: " + String(percentStatus));
  Serial.println(plataformToRobot);
}

void updatePercentStatus()
{
  Serial.println("percent:" + String(newPercentStatus));

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("https://" + String(serverAddress) + "/paint-config/UpdatePercentPaintConfig"); // Substitua com o endpoint real
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(1024);
    doc["id"] = lastPaintId; // Substitua com o ID apropriado, se necessário
    doc["percentStatus"] = newPercentStatus;

    String output;
    serializeJson(doc, output);

    int httpCode = http.POST(output);
    Serial.println("Estou dando o update");

    if (httpCode == HTTP_CODE_OK)
    {
      Serial.println("Atualizado com sucesso");
    }
    else
    {
      Serial.println("Falha na atualização: " + http.errorToString(httpCode));
    }

    shouldUpdatePercentStatus = false;
    http.end();
  }
  else
  {
    Serial.println("Não conectado ao WiFi");
  }
}

void updateProgressStatus(bool progressPaintStatus)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("https://" + String(serverAddress) + "/paint-config/UpdateProgressPaintConfig"); // Substitua com o endpoint real
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(1024);
    doc["id"] = lastPaintId; // Substitua com o ID apropriado, se necessário
    doc["isAProgressPaint"] = progressPaintStatus;

    String output;
    serializeJson(doc, output);

    int httpCode = http.POST(output);
    Serial.println("Estou dando o update");

    if (httpCode == HTTP_CODE_OK)
    {
      Serial.println("Atualizado com sucesso");
    }
    else
    {
      Serial.println("Falha na atualização: " + http.errorToString(httpCode));
    }

    http.end();
  }
  else
  {
    Serial.println("Não conectado ao WiFi");
  }
}

void moveMotorForward()
{
  digitalWrite(motor1Pin2, 0);
  digitalWrite(motor1Pin1, 1);

  digitalWrite(motor2Pin2, 1);
  digitalWrite(motor2Pin1, 0);
}

// Função para mover um motor para trás
void moveMotorBackward()
{
  digitalWrite(motor1Pin2, 1);
  digitalWrite(motor1Pin1, 0);

  digitalWrite(motor2Pin2, 0);
  digitalWrite(motor2Pin1, 1);
}

void moveMotorUpward()
{
  digitalWrite(motor3Pin2, 0);
  digitalWrite(motor3Pin1, 1);
}

// Função para mover um motor para trás
void moveMotorDownward()
{
  digitalWrite(motor3Pin2, 1);
  digitalWrite(motor3Pin1, 0);
}

// Função para parar um motor
void stopMotor()
{
  digitalWrite(motor1Pin1, 0);
  digitalWrite(motor1Pin2, 0);

  digitalWrite(motor2Pin1, 0);
  digitalWrite(motor2Pin2, 0);
}

void stopSpray()
{
  digitalWrite(sparyPin1, 0);
}
void startSpray()
{
  digitalWrite(sparyPin1, 1);
}

void paintWall()
{
  // if (isPaintPaused == true)
  // {
  //   stopSpray();
  // }
  // else
  // {
  //   startSpray();
  // }
  if (plataformHeight > 0)
  {
    int sobreposicao = 5;                   // sobreposição de 5cm para garantir cobertura completa
    int alturaSegmento = 45 - sobreposicao; // subir 40cm a cada vez
    int numSegmentosVerticais = ceil((float)plataformHeight / alturaSegmento);
    int tempoParaCobrirLargura = (plataformWidth * 200) / 10; // Tempo em ms para cobrir a largura total
    Serial.print("tempo para cobrir:");
    Serial.println(tempoParaCobrirLargura);
    /**
    * 200cm -- 4000ms
    * 10cm  -- 200ms
  ' *
    */

    int totalSegmentos = numSegmentosVerticais * 2; // Ida e volta em cada segmento vertical
    int segmentosCompletos = 0;
    int vert = 0;

    if (isAProgressPainting == true && isPaintPaused == false)
    {
      while (vert < numSegmentosVerticais + 2)
      {
        if (isPaintPaused == true)
        {
          stopSpray();
          stopMotor();
          return;
        }

        // Ida: Mover para frente pintando
        if (isPaintPaused == false)
        {

          startSpray();
          moveMotorForward();
          delay(tempoParaCobrirLargura);
        }
        else
        {
          stopSpray();
          stopMotor();
          return;
        }

        // if (vert < numSegmentosVerticais - 1)
        // { // Se não for a última passagem vertical
        // Subida: Mover spray para cima
        // stopMotor();
        // moveMotorUpward();
        // delay(1900);
        // stopSpray();
        // // }

        // Volta: Mover para trás pintando
        if (isPaintPaused == false)
        {
          moveMotorBackward();
          delay(tempoParaCobrirLargura);
        }
        else
        {
          stopSpray();
          stopMotor();
          return;
        }

        // Subida: Mover spray para cima
        if (isPaintPaused == false)
        {
          stopSpray();
          stopMotor();
          moveMotorUpward();
          delay(3500);
        }
        else
        {
          stopSpray();
          stopMotor();
          return;
        }

        // Atualizar status após cada ida e volta
        vert++;
        segmentosCompletos += 2; // Incrementar por ida e volta

        Serial.println("Numero de execução:" + String(vert));
        Serial.println("Numero de segmentos completos:" + String(segmentosCompletos));

        newPercentStatus = (segmentosCompletos * 100) / totalSegmentos ? totalSegmentos : 1;
        shouldUpdatePercentStatus = true;
      }
      // Finalizar pintura
      stopSpray();
      stopMotor();

      updateProgressStatus(false);
      moveMotorDownward();
      delay(3500 * vert);

      return;
    }
    else
    {
      stopSpray();
      stopMotor();
      return;
    }
  }
  else
  {
    stopSpray();
    stopMotor();
    return;
  }
}

void playSpray()
{
  if (isPaintPaused == true || isAProgressPainting == false)
  {
    stopSpray();
    stopMotor();
    Serial.println("Pintura não está em andamento");
  }
  else
  {
    paintWall();
  }
}

void loop2(void *parameter)
{
  for (;;)
  {
    // Adicione a verificação para isAProgressPainting aqui
    if (isAProgressPainting == true)
    {
      playSpray();
    }
    else
    {
      // Pode-se adicionar um delay para evitar uso excessivo da CPU
      vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarda 1 segundo antes de verificar novamente
    }
  }
}

void loop3(void *parameter)
{
  for (;;)
  {
    fetchRobot();
    fetchPaint();

    // playSpray();
    delay(500);
  }
}
void loop4(void *parameter)
{
  for (;;)
  {
    if (shouldUpdatePercentStatus == true)
    {
      updatePercentStatus();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(sparyPin1, OUTPUT);
  digitalWrite(sparyPin1, 0);

  // Conecte-se à rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  Serial.println("isAProgressPainting: " + String(isAProgressPainting));
  Serial.println("isPaintPaused: " + String(isPaintPaused));
  Serial.println("plataformHeight: " + String(plataformHeight));
  Serial.println("plataformWidth: " + String(plataformWidth));
  Serial.println("plataformToRobot: " + String(plataformToRobot));
  Serial.println("percentStatus: " + String(percentStatus));
  Serial.println(plataformToRobot);

  xTaskCreatePinnedToCore(loop3, "Loop3", 10000, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(loop1, "Loop1", 10000, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "Loop2", 10000, NULL, 1, NULL, 1);

  // Configurar os pinos como saídas
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);

  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);

  pinMode(motor3Pin1, OUTPUT);
  pinMode(motor3Pin2, OUTPUT);
}

void loop()
{
  // Este loop fica vazio
}
