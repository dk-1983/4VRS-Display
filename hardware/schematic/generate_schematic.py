from pathlib import Path
import json, csv
from reportlab.graphics.shapes import Drawing, Line, Rect, String, Circle, Polygon, Group
from reportlab.graphics import renderPDF, renderSVG
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, white

OUT = Path(__file__).resolve().parent
OUT.mkdir(exist_ok=True)
W,H=1190,842
ink=HexColor('#17344c'); wire=HexColor('#177450'); sym=HexColor('#a52a36'); muted=HexColor('#536676')
pages=[]
def text(x,y,s,size=10,color=ink):
    d.add(String(x,H-y,s,fontName='Helvetica',fontSize=size,fillColor=color))
def line(x1,y1,x2,y2,color=wire,width=1):
    d.add(Line(x1,H-y1,x2,H-y2,strokeColor=color,strokeWidth=width))
def rect(x,y,w,h,color=sym,fill=None):
    d.add(Rect(x,H-y-h,w,h,strokeColor=color,fillColor=fill,strokeWidth=1))
def dot(x,y): d.add(Circle(x,H-y,2.2,fillColor=wire,strokeColor=wire))
def poly(points,color=sym,fill=None):
    d.add(Polygon([v for x,y in points for v in (x,H-y)],strokeColor=color,fillColor=fill))
def net(x,y,name,length=50):
    line(x,y,x+length,y); text(x+length+5,y+3,name,9,wire)
def gnd(x,y):
    line(x,y,x,y+8); line(x-9,y+8,x+9,y+8); line(x-6,y+12,x+6,y+12); line(x-3,y+16,x+3,y+16)
    text(x-11,y+29,'GND',8,wire)
def cap(x,top,bottom,ref,val):
    mid=(top+bottom)/2; line(x,top,x,mid-3);line(x-10,mid-3,x+10,mid-3,sym,1.5)
    line(x-10,mid+3,x+10,mid+3,sym,1.5);line(x,mid+3,x,bottom)
    text(x+14,mid-5,ref,9);text(x+14,mid+10,val,9)
def res(x,y,length,ref,val,vert=False):
    if vert:
        line(x,y,x,y+length/2-12);rect(x-5,y+length/2-12,10,24);line(x,y+length/2+12,x,y+length)
        text(x+10,y+length/2-3,ref,9);text(x+10,y+length/2+11,val,9)
    else:
        line(x,y,x+length/2-15,y);rect(x+length/2-15,y-5,30,10);line(x+length/2+15,y,x+length,y)
        text(x+length/2-15,y-24,ref,9);text(x+length/2-15,y-12,val,9)
def switch(x,y,ref,label):
    line(x,y,x+22,y);d.add(Circle(x+22,H-y,2,strokeColor=sym,fillColor=white))
    d.add(Circle(x+50,H-y,2,strokeColor=sym,fillColor=white));line(x+24,y-4,x+49,y-14,sym)
    line(x+50,y,x+75,y);text(x+22,y-28,ref+' '+label,9)
def box(x,y,w,h,title):
    rect(x,y,w,h,HexColor('#bac9d0'));text(x+12,y+22,title,12)
def page(num,title):
    global d
    d=Drawing(W,H);d.add(Rect(0,0,W,H,fillColor=white,strokeColor=None))
    rect(18,18,W-36,H-36,ink);text(36,47,'4VRS DISPLAY',22);text(270,46,title,16)
    text(36,69,'Electrical schematic | Rev A1 | 2026-09-14 | ESP32-WROVER + ILI9341 SPI 240x320',10)
    line(18,80,W-18,80,ink)
    text(36,815,'Design proposal based on supplied NADIM V5 / LCDWIKI drawings; additions are identified. Not a PCB layout.',9)
    text(1020,815,f'A3 landscape | {num} / 3',9)
    pages.append(d)

page(1,'Power, controller and display')
box(35,95,1120,195,'1. POWER - proposed lower-dropout supply; use a regulated 5 V / 1 A minimum source')
text(55,150,'J1 POWER',10);text(60,177,'2  +5V',9);text(60,237,'1  GND',9)
line(125,175,215,175);text(150,160,'+5V_IN',9,wire)
poly([(215,166),(215,184),(236,175)],sym);line(238,165,238,185,sym,1.5)
line(232,165,238,165,sym);line(238,185,244,185,sym)
line(238,175,475,175);text(214,148,'D1  1N5817',10);text(247,167,'K',8);text(298,158,'+5V_PROTECTED',9,wire)
line(125,235,125,248);gnd(125,248)
cap(330,175,245,'C1','10uF / 10V');dot(330,175);gnd(330,245)
cap(425,175,245,'C2','100nF');dot(425,175);gnd(425,245)
rect(475,142,195,68);text(495,160,'U2  AP7361C-33ER-13',11)
text(484,179,'3 IN',10);text(608,179,'OUT 2',10);text(530,200,'GND 1',10)
line(565,210,565,245);gnd(565,245);line(670,175,1115,175);text(1045,160,'+3V3',11,wire)
cap(740,175,245,'C3','10uF / 10V');dot(740,175);gnd(740,245)
cap(850,175,245,'C4','100nF');dot(850,175);gnd(850,245)
cap(950,175,245,'C5','100uF / 6.3V');dot(950,175);gnd(950,245)
text(931,202,'+',10,sym)
text(986,238,'C5: positive to +3V3',9);text(986,254,'U2 tab = OUT (+3V3)',9)
text(986,270,'SOT223R, NOT SOT223',9,sym)

box(35,305,1120,480,'2. SIGNALS - identical net names are electrically connected; numbers at U1 are module pad numbers')
rect(330,354,230,336);text(356,375,'U1  ESP32-WROVER(-I)',13)
left=[('2','3V3','+3V3'),('3','EN','EN'),('25','IO0','BOOT_N'),('4','IO36 / VP','BTN_A'),('5','IO39 / VN','BTN_B'),('7','IO35','BTN_C'),('34','IO3 / RXD0','UART_RX'),('35','IO1 / TXD0','UART_TX')]
for i,(pin,name,n) in enumerate(left):
    y=410+i*31;line(300,y,330,y);text(309,y-5,pin,8);text(341,y+3,name,10);line(190,y,300,y);text(195,y-5,n,9,wire)
right=[('16','IO13','TFT_CS'),('13','IO14','TFT_DC'),('24','IO2','TFT_RST'),('37','IO23','TFT_MOSI'),('30','IO18','TFT_SCK'),('31','IO19','TFT_MISO'),('26','IO4','TFT_BL')]
for i,(pin,name,n) in enumerate(right):
    y=410+i*31;line(560,y,690,y);text(568,y-5,pin,8);text(495,y+3,name,10);text(602,y-5,n,9,wire)
text(372,666,'GND 1, 15, 38, EPAD 39',10);line(445,690,445,705);gnd(445,705)
text(50,713,'UNUSED / RESERVED U1 pads:',9)
text(50,730,'6, 8-12, 14, 17-23, 27-29, 32, 33, 36.',9)
text(50,747,'Leave unconnected in this design.',9)
text(50,764,'Do not use flash / PSRAM pads as GPIO.',9,sym)
text(468,731,'GPIO2: no external pull-up; TFT reset is an input.',9)
text(468,748,'GPIO12: leave unused; do not force a new boot strap.',9)
text(468,765,'WROVER(-I): attach the appropriate antenna before RF use.',9)

rect(872,360,253,350);text(889,384,'DS1  TFT SPI 240x320 v1.3',12)
text(889,401,'ILI9341 module, no touch IC',11)
signals=[('+3V3','1','VCC'),('GND','2','GND'),('TFT_CS','3','CS'),('TFT_RST','4','RESET'),('TFT_DC','5','DC'),('TFT_MOSI','6','SDI / MOSI'),('TFT_SCK','7','SCK'),('TFT_BL','8','LED / PWM input'),('TFT_MISO','9','SDO / MISO')]
for i,(n,pin,name) in enumerate(signals):
    y=425+i*24;line(750,y,872,y);text(755,y-4,n,9,wire);text(851,y-4,pin,8);text(885,y+4,name,10)
text(887,665,'10-14: NC (touch controller absent)',9)
text(887,682,'On-module J1: CLOSED for +3V3 VCC',9,sym)
text(887,699,'microSD header: unconnected in Rev A1',9)

page(2,'Reset, service connector and backlight')
box(35,95,535,302,'3. RESET / BOOT - added passive parts for defined startup')
for x,ref,n in [(100,'R1','BOOT_N'),(325,'R2','EN')]:
    text(x-15,145,'+3V3',10,wire);line(x,151,x,160);res(x,160,75,ref,'10k',True)
    dot(x,235);line(x,235,x+32,235);text(x+38,228,n,9,wire);line(x,235,x,290);switch(x,290,'SW1' if x==100 else 'SW2','PGM' if x==100 else 'RESET');gnd(x+75,290)
cap(485,235,315,'C6','1uF');line(325,235,485,235);dot(485,235);gnd(485,315)
text(55,367,'Hold PGM, pulse RESET, release PGM to enter the UART bootloader.',10)
text(55,385,'Normal start: RESET only. Do not short the EN pull-up resistor.',10)
box(590,95,565,302,'4. UART SERVICE - 3.3 V logic, manual reset')
rect(635,160,155,142);text(645,183,'J2 / P10  UART',12)
for i,(pin,n,desc) in enumerate([('1','GND','Adapter GND'),('2','UART_RX','From adapter TX output'),('3','UART_TX','To adapter RX input')]):
    y=215+i*30;text(647,y+3,pin,10);line(790,y,930,y);text(805,y-5,n,10,wire);text(945,y+3,desc,10)
text(615,335,'Power the device through J1; do not join two supply outputs.',10)
text(615,355,'Signal direction is authoritative: adapter connector labels may vary.',10)
text(615,375,'UART log: 115200 baud. GPIO1 = ESP TX; GPIO3 = ESP RX.',10)

box(35,415,535,352,'5. OUR MODULE - GPIO4 drives an existing transistor input')
start=len(d.contents)
text(55,465,'TFT_BL',10,wire);line(112,462,170,462);text(113,450,'DS1.8',9)
res(170,462,90,'DS1.R6','1k');line(260,462,303,462)
line(303,443,303,486,sym,2);line(303,453,330,433,sym);line(303,476,330,499,sym)
poly([(323,486),(325,497),(315,493)],sym,sym)
line(330,499,330,516);gnd(330,516);line(330,433,330,420+20)
line(330,433,365,433);res(365,433,90,'DS1.R5','10R');net(455,433,'LEDK',35)
text(350,485,'DS1.Q1',10);text(350,502,'S8050',10)
g=Group(*d.contents[start:]); d.contents[start:]=[];g.translate(0,-60);d.add(g)
text(55,625,'Equivalent circuit inside the LCDWIKI module (not extra parts).',10)
text(55,642,'LEDA is supplied by the module +3V3 rail.',10)
text(55,659,'GPIO4 -> DS1 pin 8 directly; HIGH enables the backlight.',10)
text(55,676,'Firmware PWM: 5 kHz / 10 bit. No external NPN is fitted here.',10)
text(55,693,'DS1 pin 8 is a logic input, not the raw LED anode.',10,sym)
text(270,732,'Optional R3: PWM low at reset.',10)
res(130,734,65,'R3','100k');text(55,737,'TFT_BL',9,wire);line(110,734,130,734);gnd(195,734)

box(590,415,565,352,'6. ALTERNATIVE ONLY - display with separate LEDA / LEDK')
start=len(d.contents)
text(610,470,'GPIO4',10,wire);line(660,466,690,466);res(690,466,90,'R20','1k');line(780,466,850,466)
line(850,447,850,490,sym,2);line(850,456,885,432,sym);line(850,479,885,510,sym)
poly([(878,496),(881,509),(869,504)],sym,sym);line(885,510,885,535);gnd(885,535)
text(905,485,'Q20  MMBT2222A',10);text(905,503,'B=1, E=2, C=3',9)
line(885,432,930,432);res(930,432,80,'R21','see calculation');net(1010,432,'LEDK',30)
dot(810,466);res(810,466,69,'R22','100k',True);gnd(810,535)
g=Group(*d.contents[start:]);d.contents[start:]=[];g.translate(0,-60);d.add(g)
text(615,645,'LEDA -> rated supply; LEDK -> R21 -> Q20 collector.',10)
text(615,661,'Example: one 3.0 V LED string, 3.3 V supply, 10 mA target.',10)
text(615,677,'R21 >= (3.3 - 3.0 - 0.1) / 0.010 = 20 ohm; choose 22 ohm.',10)
text(615,693,'Recalculate for actual Vf, current and supply tolerances.',10)
text(615,709,'Parallel strings need individual limiting or a LED driver.',10)
text(615,729,'Not for our DS1 LED input. Do not connect both circuits.',11,sym)
text(615,749,'R20 limits GPIO/base current; the original drawing omitted it.',10)

page(3,'Assembly notes, options and component list')
box(35,95,1120,225,'7. OPTIONAL BUTTONS - hardware provision; navigation is not implemented in firmware 0.4.3')
for i,(ref,n,pin) in enumerate([('A','BTN_A','GPIO36'),('B','BTN_B','GPIO39'),('C','BTN_C','GPIO35')]):
    x=100+i*355;text(x-15,144,'+3V3',10,wire);res(x,153,65,'R'+str(4+i),'10k',True)
    net(x,218,n,42);dot(x,218);line(x,218,x,257);switch(x,257,'SW'+str(3+i),ref);gnd(x+75,257)
    text(x+120,240,pin+' input only',10);text(x+120,261,'External pull-up required',9)

box(35,337,535,440,'8. MAIN ASSEMBLY BOM')
rows=[('U1','ESP32-WROVER(-I), PSRAM, module'),('U2','AP7361C-33ER-13, 3.3 V, SOT223R'),('D1','1N5817, series reverse-polarity diode'),('J1 / J2','2-pin power / 3-pin UART connector'),('DS1','TFT SPI ILI9341 240x320, LED logic input'),('C1 / C3','10uF X7R, 10 V; effective C >= 4.7uF'),('C2 / C4','100nF X7R, 10 V or higher'),('C5','100uF electrolytic, >= 6.3 V'),('C6','1uF X7R, >= 6.3 V'),('R1 / R2','10k, 1%, 0.1 W or higher'),('R3','100k, 1%, optional PWM pull-down'),('SW1 / SW2','Normally-open momentary PGM / RESET'),('R4-R6 / SW3-SW5','Optional: 10k / normally-open buttons')]
for i,(a,b) in enumerate(rows):
    yy=387+i*26;text(50,yy,a,10);text(170,yy,b,10);line(48,yy+8,552,yy+8,HexColor('#dbe4e8'),.5)
text(50,748,'Q20 / R20-R22 belong only to the alternative, not the main BOM.',9)

box(590,337,565,440,'9. BUILD CONDITIONS AND REFERENCES')
notes=[
'U2 is a proposed change from the original L1117-33 supply.',
'Check the exact SOT223R package: 1=GND, 2/tab=OUT, 3=IN.',
'Ordinary SOT223 AP7361C has a different pinout.',
'Place C1/C2 close to U2 IN; C3 close to U2 OUT.',
'Place C4/C5 close to U1 power pins with a short ground return.',
'Allow at least 500 mA for ESP32 plus LCD/backlight current.',
'LDO heat: P=(Vin-3.3)*I; 4.7 V at 0.5 A gives about 0.7 W.',
'Provide copper area and check temperature inside the enclosure.',
'For sustained high load, use a qualified 3.3 V buck supply.',
'DS1 VCC=3.3 V: close its supply jumper J1 (0R bypass).',
'If changing DS1 VCC to 5 V, open its J1 first.',
'Touch pins and microSD connector are intentionally not wired.',
'GPIO numbers are not ESP32 chip QFN pad numbers.',
'No ERC/SPICE or new PCB hardware validation has been run.',
'Confirm supply, startup and current on the assembled hardware.'
]
for i,t in enumerate(notes):text(607,383+i*20,t,10)
text(607,704,'Manufacturer documentation (clickable in PDF):',10)
for y,t in [(725,'LCDWIKI MSP2402: module schematic and manual'),(742,'Espressif: WROVER module / hardware design guidelines'),(759,'Diodes: AP7361C datasheet; original 1117: see TI datasheet')]:text(607,y,t,9)

pdf=canvas.Canvas(str(OUT/'4vrs-display-schematic-A1.pdf'),pagesize=(W,H))
pdf.setTitle('4VRS Display - electrical schematic Rev A1')
for i,p in enumerate(pages,1):
    renderPDF.draw(p,pdf,0,0)
    if i==3:
        for y,url in [(725,'https://www.lcdwiki.com/2.4inch_SPI_Module_ILI9341_SKU:MSP2402'),(742,'https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html'),(759,'https://www.diodes.com/datasheet/download/AP7361C.pdf')]:
            pdf.linkURL(url,(603,H-y-3,1145,H-y+12),relative=0)
    pdf.showPage()
    renderSVG.drawToFile(p,str(OUT/f'4vrs-display-schematic-A1-sheet-{i}.svg'))
pdf.save()

nets={
'+5V_IN':['J1.2','D1.A'], '+5V_PROTECTED':['D1.K','U2.3','C1.1','C2.1'],
'+3V3':['U2.2','U2.TAB','U1.2','DS1.1','C3.1','C4.1','C5.+','R1.1','R2.1','R4.1','R5.1','R6.1'],
'GND':['J1.1','J2.1','U2.1','U1.1','U1.15','U1.38','U1.39','DS1.2','C1.2','C2.2','C3.2','C4.2','C5.-','C6.2','SW1.2','SW2.2','R3.2','SW3.2','SW4.2','SW5.2'],
'BOOT_N':['U1.25','R1.2','SW1.1'], 'EN':['U1.3','R2.2','C6.1','SW2.1'],
'TFT_CS':['U1.16','DS1.3'], 'TFT_DC':['U1.13','DS1.5'], 'TFT_RST':['U1.24','DS1.4'],
'TFT_MOSI':['U1.37','DS1.6'], 'TFT_SCK':['U1.30','DS1.7'], 'TFT_MISO':['U1.31','DS1.9'],
'TFT_BL':['U1.26','DS1.8','R3.1'], 'UART_TX':['U1.35','J2.3'], 'UART_RX':['U1.34','J2.2'],
'BTN_A':['U1.4','R4.2','SW3.1'], 'BTN_B':['U1.5','R5.2','SW4.1'], 'BTN_C':['U1.7','R6.2','SW5.1']}
seen={}
for n,parts in nets.items():
    for p in parts:
        assert p not in seen,(p,n,seen.get(p));seen[p]=n
used={int(p.split('.')[1]) for p in seen if p.startswith('U1.')}
nc=set([6,*range(8,13),14,*range(17,24),27,28,29,32,33,36])
assert used.isdisjoint(nc)
assert used|nc==set(range(1,40))
(OUT/'connections.json').write_text(json.dumps({'revision':'A1','optional_components':['R3','R4','R5','R6','SW3','SW4','SW5'],'nets':nets,'U1_unconnected_pads':sorted(nc),'DS1_unconnected_pins':[10,11,12,13,14]},indent=2)+'\n')
with (OUT/'bom.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.writer(f);w.writerow(['Reference','Specification']);w.writerows(rows)
print('Created 3-page PDF, 3 SVG sheets, BOM and connectivity list.')
print('Connectivity check: every U1 module pad accounted for; no pin assigned to two nets.')
