#pragma once
static const char HOME_PAGE[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><style>body{font:17px system-ui;max-width:700px;margin:40px auto;padding:0 20px;background:#101820;color:#edf4ff}a{color:#7ce0ff}nav{display:grid;gap:16px;margin-top:28px}nav a{display:block;padding:22px;border:1px solid #32526a;border-radius:12px;text-decoration:none;background:#182735}nav strong{display:block;font-size:21px}nav span{display:block;color:#c1ccda;margin-top:8px}dt{color:#a8bdd0;margin-top:16px}dd{margin:4px 0;overflow-wrap:anywhere}</style><title>4VRS Display</title><h1>4VRS Display</h1><p>Управление вашим дисплеем</p><nav><a href="/network"><strong>Настройки IPv4</strong><span>DHCP, IP, маска, шлюз и DNS</span></a><a href="/updates"><strong>GitHub · Updates</strong></a><a href="/mqtt"><strong>Настройки MQTT</strong><span>Подключение к брокеру и Home Assistant</span></a><a href="/demo.bmp"><strong>Встроенная заставка</strong><span>Посмотреть изображение по умолчанию</span></a><a href="/settings"><strong>Настройки интерфейса</strong><span>Язык и данные входа</span></a><a href="/about"><strong>О модуле</strong><span>Версия, сеть и диагностика</span></a></nav></html>)HTML";
static const char ABOUT_PAGE[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><style>body{font:17px system-ui;max-width:700px;margin:40px auto;padding:0 20px;background:#101820;color:#edf4ff}a{color:#7ce0ff}nav{display:grid;gap:16px;margin-top:28px}nav a{display:block;padding:22px;border:1px solid #32526a;border-radius:12px;text-decoration:none;background:#182735}nav strong{display:block;font-size:21px}nav span{display:block;color:#c1ccda;margin-top:8px}dt{color:#a8bdd0;margin-top:16px}dd{margin:4px 0;overflow-wrap:anywhere}</style><title>О модуле · 4VRS</title><a href="/">← Главная</a><h1>О модуле</h1><p>4VRS Server Room Display</p><dl><dt>Прошивка</dt><dd>__VERSION__</dd><dt>Устройство</dt><dd id="device">—</dd><dt>IP-адрес</dt><dd id="ip">—</dd><dt>Точка настройки Wi-Fi</dt><dd id="ap">—</dd><dt>Обновление по воздуху</dt><dd>ArduinoOTA · UDP 3232</dd><dt>Лицензия собственного кода</dt><dd>MIT</dd></dl><p id="notice"></p>
<section aria-labelledby="display-heading">
<h2 id="display-heading">Проверка дисплея</h2>
<p>Проверка связи с контроллером дисплея по SDO/MISO (GPIO19). Для вывода изображения этот провод не нужен.</p>
<button id="check-display" type="button">Проверить дисплей</button>
<p id="display-result" role="status" aria-live="polite">Нажмите кнопку для проверки.</p>
<p>Ответ контроллера не подтверждает исправность матрицы и подсветки: изображение нужно оценить визуально.</p>
<details id="display-details" hidden><summary>Регистры дисплея</summary><pre id="display-registers"></pre></details>
<p><a href="/display">Данные дисплея (JSON)</a></p>
</section>
<p><a href="/health">Подробная диагностика</a></p>
<style>button{font:inherit;color:#edf4ff;background:#23445d;border:1px solid #7ce0ff;border-radius:8px;padding:12px 18px;cursor:pointer}button:disabled{opacity:.6;cursor:wait}button:focus-visible,summary:focus-visible{outline:2px solid #7ce0ff;outline-offset:4px}pre{white-space:pre-wrap;overflow-wrap:anywhere}section{margin-top:28px;border-top:1px solid #32526a}#display-result{padding:12px;border-left:3px solid #7ce0ff}#display-result[data-state="response_observed"]{border-color:#65d99a}#display-result[data-state="no_response"],#display-result[data-state="unstable"],#display-result[data-state="error"]{border-color:#ffc56b}</style>
<script>
const displayButton=document.getElementById("check-display");
displayButton.addEventListener("click",async()=>{
  if(displayButton.disabled)return;
  const result=document.getElementById("display-result");
  const details=document.getElementById("display-details");
  const registers=document.getElementById("display-registers");
  displayButton.disabled=true;
  details.hidden=true;
  registers.textContent="";
  result.dataset.state="checking";
  result.textContent="Проверяем связь с дисплеем…";
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),8000);
  try{
    const response=await fetch("/display",{cache:"no-store",signal:controller.signal});
    if(!response.ok)throw Error("HTTP");
    const data=await response.json();
    const state=data&&data.registers&&data.registers.state;
    const messages={
      response_observed:"Контроллер дисплея отвечает. Повторные чтения регистров совпадают.",
      no_response:"Ответ не получен. Если SDO/MISO не подключён, это ожидаемо и не означает неисправность дисплея. Иначе проверьте соединение и питание.",
      unstable:"Ответы различаются. Проверьте соединения SPI и питание дисплея.",
      not_sampled:"Данные ещё не прочитаны. Повторите проверку после запуска дисплея."
    };
    if(!Object.prototype.hasOwnProperty.call(messages,state))throw Error("Unexpected response");
    result.dataset.state=state;
    result.textContent=messages[state];
    registers.textContent=JSON.stringify(data,null,2);
    details.hidden=false;
  }catch(error){
    result.dataset.state="error";
    result.textContent="Не удалось выполнить проверку. Проверьте связь с модулем и повторите попытку.";
  }finally{
    clearTimeout(timeout);
    displayButton.disabled=false;
  }
});
fetch("/health",{cache:"no-store"}).then(r=>{if(!r.ok)throw Error();return r.json()}).then(h=>{document.getElementById("device").textContent=h.hostname;document.getElementById("ip").textContent=h.ip;document.getElementById("ap").textContent=h.ap?"Включена":"Выключена"}).catch(()=>document.getElementById("notice").textContent="Не удалось обновить сведения о сети.")</script></html>)HTML";
