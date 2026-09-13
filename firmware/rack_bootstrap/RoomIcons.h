#pragma once
// Original 24x24 line drawings; draw scaled on title cards or beside the room name.
static void drawRoomIcon(GFXcanvas16 &c,const char *name,int x,int y,int s,uint16_t color) {
  auto line=[&](int a,int b,int d,int e){c.drawLine(x+a*s,y+b*s,x+d*s,y+e*s,color);};
  auto rect=[&](int a,int b,int w,int h){c.drawRect(x+a*s,y+b*s,w*s,h*s,color);};
  auto circle=[&](int a,int b,int radius){c.drawCircle(x+a*s,y+b*s,radius*s,color);};
  if(!strcmp(name,"toilet")){rect(3,2,7,10);rect(9,11,12,3);line(9,14,13,19);line(20,14,16,19);rect(12,19,6,3);line(5,5,7,5);}
  else if(!strcmp(name,"bathroom")){line(3,10,21,10);line(3,10,7,17);line(21,10,17,17);line(7,17,17,17);line(12,17,12,22);line(10,10,10,4);line(10,4,15,4);line(15,4,15,7);}
  else if(!strcmp(name,"kitchen")){rect(5,1,14,22);line(5,9,18,9);line(8,4,8,6);line(8,12,8,16);}
  else if(!strcmp(name,"bedroom")){rect(2,11,20,8);rect(3,6,7,5);rect(13,6,7,5);line(2,5,2,22);line(21,5,21,22);}
  else if(!strcmp(name,"living")){rect(5,6,14,8);rect(2,11,4,9);rect(18,11,4,9);line(6,16,18,16);line(6,20,18,20);line(4,20,4,22);line(20,20,20,22);}
  else if(!strcmp(name,"garage")){line(1,9,12,2);line(12,2,23,9);rect(4,9,16,14);for(int h=12;h<22;h+=3)line(6,h,18,h);}
  else if(!strcmp(name,"server")){for(int h=2;h<22;h+=7){rect(3,h,18,6);circle(6,h+2,1);line(11,h+3,18,h+3);}}
  else if(!strcmp(name,"office")){rect(5,2,14,11);line(12,13,12,17);line(8,17,16,17);line(2,19,22,19);line(4,19,4,23);line(20,19,20,23);}
  else if(!strcmp(name,"balcony")){rect(6,2,12,12);line(12,2,12,14);line(2,15,22,15);line(2,22,22,22);for(int a=3;a<23;a+=4)line(a,15,a,22);}
  else if(!strcmp(name,"hall")){rect(5,2,14,21);line(9,4,9,21);circle(16,12,1);line(1,23,23,23);}
  else if(!strcmp(name,"garden")){circle(12,8,6);circle(6,12,4);circle(18,12,4);line(12,12,12,23);line(5,23,19,23);}
  else if(!strcmp(name,"laundry")){rect(3,1,18,22);line(3,6,20,6);circle(12,14,6);circle(6,4,1);line(12,4,17,4);}
  else if(!strcmp(name,"storage")){rect(3,8,18,15);line(3,8,7,2);line(7,2,17,2);line(17,2,21,8);line(12,2,12,14);line(8,14,16,14);}
  else if(!strcmp(name,"kids")){circle(12,12,8);circle(5,5,3);circle(19,5,3);circle(9,11,1);circle(15,11,1);line(10,16,14,16);}
  else {line(1,11,12,2);line(12,2,23,11);line(4,9,4,23);line(20,9,20,23);line(4,23,20,23);rect(9,14,6,9);}
}
