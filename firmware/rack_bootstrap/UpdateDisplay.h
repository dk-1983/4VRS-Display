#pragma once
// All drawing runs on the Arduino loop task. The HTTPS worker only publishes
// atomic stage/counter values: no shared SPI access and no TLS-time canvas allocation.
namespace UpdateDisplay {
static bool visible=false;
static RackUpdate::ScreenStage lastStage=RackUpdate::ScreenStage::Idle;
static unsigned lastPercent=101;
static uint32_t paintedAt=0,finishedAt=0;

inline void releaseCanvas() {
  if(mqttCanvas){delete mqttCanvas;mqttCanvas=nullptr;}
  mqttShowing=false;mqttPaintRow=320;
}
inline const char *stageText(RackUpdate::ScreenStage stage) {
  using S=RackUpdate::ScreenStage;
  switch(stage){
    case S::Checking:return WebSettings::label("ПРОВЕРКА РЕЛИЗА","CHECKING RELEASE");
    case S::Downloading:return WebSettings::label("ЗАГРУЗКА И ЗАПИСЬ","DOWNLOADING");
    case S::Verifying:return WebSettings::label("ПРОВЕРКА ОБРАЗА","VERIFYING IMAGE");
    case S::Restarting:return WebSettings::label("ПЕРЕЗАПУСК","RESTARTING");
    case S::Error:return WebSettings::label("ОШИБКА ОБНОВЛЕНИЯ","UPDATE FAILED");
    case S::Blocked:return WebSettings::label("ОБНОВЛЕНИЕ ЗАПРЕЩЕНО","UPDATE BLOCKED");
    default:return WebSettings::label("ПОДГОТОВКА","PREPARING");
  }
}
inline void show(RackUpdate::ScreenStage stage,unsigned percent) {
  if(!displayStarted)return;
  percent=std::min(percent,100u);
  const bool first=!visible,changed=first||stage!=lastStage;
  if(!changed&&(percent==lastPercent||uint32_t(millis()-paintedAt)<250))return;
  if(first){
    releaseCanvas();display.fillScreen(ILI9341_BLACK);visible=true;
    drawLabel(display,"4VRS",20,25,ILI9341_CYAN,2);
    drawLabel(display,WebSettings::label("ОБНОВЛЕНИЕ","FIRMWARE"),20,70,ILI9341_WHITE,2);
    drawLabel(display,WebSettings::label("ПРОШИВКИ","UPDATE"),20,95,ILI9341_WHITE,2);
    display.drawRect(20,210,200,18,ILI9341_CYAN);
  }
  if(changed){
    display.fillRect(20,137,212,15,ILI9341_BLACK);
    drawLabel(display,stageText(stage),20,140,ILI9341_CYAN);
    display.fillRect(20,250,212,40,ILI9341_BLACK);
    if(stage!=RackUpdate::ScreenStage::Error&&stage!=RackUpdate::ScreenStage::Blocked){
      drawLabel(display,WebSettings::label("НЕ ОТКЛЮЧАЙТЕ","DO NOT DISCONNECT"),20,255,ILI9341_WHITE);
      drawLabel(display,WebSettings::label("ПИТАНИЕ","POWER"),20,273,ILI9341_WHITE);
    }
  }
  display.fillRect(20,166,200,30,ILI9341_BLACK);
  const bool numeric=stage==RackUpdate::ScreenStage::Downloading||stage==RackUpdate::ScreenStage::Verifying||stage==RackUpdate::ScreenStage::Restarting;
  char value[8];snprintf(value,sizeof(value),"%u%%",percent);
  drawLabel(display,numeric?value:"...",80,168,ILI9341_WHITE,3);
  display.fillRect(22,212,196,14,ILI9341_BLACK);
  if(numeric&&percent)display.fillRect(22,212,196*percent/100,14,ILI9341_CYAN);
  lastStage=stage;lastPercent=percent;paintedAt=millis();
}
inline bool tick() {
  if(RackUpdate::busy||RackUpdate::restartRequested){
    releaseCanvas();RackUpdate::canvasReleased=true;finishedAt=0;
    auto stage=RackUpdate::screenStage.load();
    show(stage,stage==RackUpdate::ScreenStage::Restarting?100:RackUpdate::downloadPercent());
    return true;
  }
  if(visible){
    const auto stage=RackUpdate::screenStage.load();
    if(stage==RackUpdate::ScreenStage::Error||stage==RackUpdate::ScreenStage::Blocked){
      show(stage,RackUpdate::downloadPercent());
      if(!finishedAt)finishedAt=millis();
      if(uint32_t(millis()-finishedAt)<4000)return true;
    }
    visible=false;finishedAt=0;displayRow=0;mqttShowing=false;mqttPaintRow=320;
  }
  return false;
}
} // namespace UpdateDisplay
