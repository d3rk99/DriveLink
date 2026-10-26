using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace DriveLinkRestart {
public class FilmActor {
 public FilmActor(){car=-1;}
 public string name {get;set;} public string skin {get;set;} public string[] layers {get;set;}
 public int car {get;set;} public string hand {get;set;} public string back {get;set;}
}
public class FilmEvent {
 public string kind {get;set;} public double km {get;set;} public string who {get;set;} public string detail {get;set;}
 public int car {get;set;} public int donor {get;set;}
}
public class RoadFilm {
 public string car {get;set;} public string[] cars {get;set;} public double km {get;set;}
 public string biomeFrom {get;set;} public string biomeTo {get;set;} public string biomeVia {get;set;}
 public string skip {get;set;} public FilmActor[] crew {get;set;} public FilmEvent[] events {get;set;}
 public static string Biome(string world) {
  string w=(world??"").ToLowerInvariant();
  if(w.Contains("sakhal")||w.Contains("namalsk")||w.Contains("winter")||w.Contains("snow")) return "snow";
  if(w.Contains("takistan")||w.Contains("desert")||w.Contains("zagoria")||w.Contains("arid")) return "desert";
  if(w.Contains("banov")||w.Contains("field")||w.Contains("steppe")) return "field";
  return "forest";
 }
 public string[] Looks {get {return cars!=null&&cars.Length>0?cars:(String.IsNullOrEmpty(car)?new string[0]:new[]{car});}}
 public bool Valid() {
  if(events==null||events.Length==0||events.Length>128||crew==null||crew.Length>32||Looks.Length>8||Looks.Any(x=>String.IsNullOrEmpty(x)||x.Length>80)) return false;
  foreach(var e in events) if(e==null||Double.IsNaN(e.km)||Double.IsInfinity(e.km)||e.km<0||e.km>100000||
   !(new[]{"drive","walk","swim","flat","fuel","attack","ration","died","move","find"}).Contains(e.kind)) return false;
  return crew.All(a=>a!=null && (a.layers==null||a.layers.Length<=20));
 }
 // Same event durations and total cap as DL_Film.SceneSeconds / Duration.
 public static double Seconds(FilmEvent e) {
  if(e.kind=="drive"||e.kind=="walk"||e.kind=="swim") return Math.Max(3,Math.Min(8,e.km*.15));
  if(e.kind=="flat") return 10; if(e.kind=="fuel") return 11; if(e.kind=="attack") return 8;
  if(e.kind=="move") return 4;
  if(e.kind=="find") return e.detail=="gas"?13+3.5*(Math.Max(1,e.donor)-1):(e.detail=="water"?9:5);
  return 3;
 }
 public double Duration {get {return Math.Min(45,6+(events??new FilmEvent[0]).Sum(e=>Seconds(e)));}}
 public int SceneAt(double elapsed,out double progress) {
  progress=0;
  if(elapsed<2||elapsed>=Duration) return -1;
  double cursor=2;
  for(int i=0;i<events.Length;i++) {double duration=Seconds(events[i]); if(elapsed<cursor+duration){progress=(elapsed-cursor)/duration;return i;} cursor+=duration;}
  return -1;
 }
 public double ScrollAt(double elapsed) {
  double cursor=2,scroll=0,stop=Math.Min(elapsed,Duration);
  foreach(var e in events) {
   double span=Math.Max(0,Math.Min(Seconds(e),stop-cursor));
   if(e.kind=="drive")scroll+=span*55;
   if(e.kind=="walk"||e.kind=="swim")scroll+=span*18;
   cursor+=Seconds(e);
  }
  if(elapsed>Duration)scroll+=(elapsed-Duration)*(Looks.Length==0?18:55);
  return scroll;
 }
 public string Caption(FilmEvent e) {
  if(e==null) return "";
  string who=crew.Length==1?"You":e.who??"A traveller";
  string distance=Math.Floor(e.km+.5)+" km";
  bool convoy=Looks.Length>1,boat=Looks.Any(x=>x.StartsWith("boat")),sky=Looks.Any(x=>x.StartsWith("plane"));
  if(e.kind=="drive") return convoy?"The convoy rolls on: "+distance:(sky?"In the air: ":"On the road: ")+distance;
  if(e.kind=="walk") return convoy?"The convoy keeps to walking pace: "+distance:"On foot: "+distance;
  if(e.kind=="swim") return convoy?"The boats keep to swimming pace: "+distance:"Swimming: "+distance;
  if(e.kind=="flat") {
   var donor=crew.FirstOrDefault(a=>a.car==e.donor);
   if(e.detail=="spare"&&donor!=null&&e.donor>=0) return "Blew a tyre. "+donor.name+"'s spare fits: "+who+" puts it on.";
   if(e.detail=="spare") return "Blew a tyre. "+who+" put on the spare.";
   if(e.detail=="kit") return "Blew a tyre. "+who+" patched it.";
   return "Blew a tyre - no spare. The car stays here.";
  }
  if(e.kind=="fuel") {
   if(e.detail=="found") return "Out of gas. "+who+" found some fuel!";
   if(e.detail=="shared") return "Running dry. "+who+" brings a jerry can.";
   if(e.detail=="landed") return "Out of fuel: an emergency landing.";
   if(boat) return convoy?"Out of gas. Nothing found: the boat is left to drift.":"Out of gas. Nothing found: over the side, swimming from here.";
   return convoy?"Out of gas. Nothing found: the car stays here.":"Out of gas. Nothing found: on foot from here.";
  }
  if(e.kind=="find") {
   if(e.detail=="gas") return boat?"A fuel dock! Every tank and can filled.":"A working gas station! Every tank and can filled.";
   if(e.detail=="water") return boat?"Rain! Everyone drinks, every bottle filled.":"A well! Everyone drinks, every bottle filled.";
   if(e.detail=="food") return who+" found food for the road.";
  }
  if(e.kind=="move") return e.detail=="swim"?who+" swims on beside the boats: no seat left.":(e.detail=="walk"?who+" walks on: no seat left.":who+" climbs into another car.");
  if(e.kind=="attack") return e.detail=="died"?who+" didn't survive the attack.":"Infected! "+who+" fought it off.";
  if(e.kind=="ration") return "Food and water ran low: everyone rations.";
  if(e.kind=="died") return who+(e.detail=="drowned"?" drowned.":" didn't make it.");
  return "";
 }
}

// Native renderer of the shared journey data. Draws the original converted sprite sheets.
// Scene choreography is implemented here; it does not execute or simulate gameplay outcomes.
public sealed class FilmRenderer : IDisposable {
 readonly RoadFilm film; readonly string assets;
 readonly Dictionary<string,Bitmap> images=new Dictionary<string,Bitmap>();
 public FilmRenderer(RoadFilm value,string path) {film=value;assets=path;}
 public RoadFilm Film {get {return film;}}
 Bitmap Image(string id) {
  if(String.IsNullOrEmpty(id)||!Regex.IsMatch(id,@"^[a-zA-Z0-9_/]+$")) return null;
  Bitmap result; if(images.TryGetValue(id,out result)) return result;
  string path=Path.Combine(assets,id.Replace('/',Path.DirectorySeparatorChar)+"_ca.png");
  result=File.Exists(path)?new Bitmap(path):null; images[id]=result; return result;
 }
 static Color Tint(string value) {
  switch(value) {
   case "black":return Color.FromArgb(72,72,78);case "white":return Color.FromArgb(250,250,250);
   case "gray":case "grey":return Color.FromArgb(165,165,168);case "red":case "wine":case "maroon":return Color.FromArgb(215,55,50);
   case "blue":return Color.FromArgb(70,115,230);case "lightblue":return Color.FromArgb(135,190,245);
   case "darkblue":case "bluedark":case "navy":return Color.FromArgb(45,65,150);
   case "green":return Color.FromArgb(85,165,75);case "darkgreen":case "greendark":return Color.FromArgb(55,100,55);
   case "olive":return Color.FromArgb(130,140,75);case "lime":return Color.FromArgb(160,235,65);
   case "yellow":return Color.FromArgb(250,215,60);case "orange":return Color.FromArgb(250,140,40);
   case "pink":return Color.FromArgb(250,140,195);case "purple":case "violet":return Color.FromArgb(150,85,205);
   case "brown":return Color.FromArgb(145,100,60);case "beige":case "tan":case "khaki":return Color.FromArgb(220,195,145);
   default:return Color.White;
  }
 }
 void Sprite(Graphics g,string id,RectangleF target,Rectangle source,Color tint) {
  var bitmap=Image(id); if(bitmap==null)return;
  if(source.Width==0) source=new Rectangle(0,0,bitmap.Width,bitmap.Height);
  if(source.Bottom>bitmap.Height||source.Right>bitmap.Width)return;
  using(var attributes=new ImageAttributes()) {
   var matrix=new ColorMatrix();matrix.Matrix00=tint.R/255f;matrix.Matrix11=tint.G/255f;matrix.Matrix22=tint.B/255f;matrix.Matrix33=tint.A/255f;
   attributes.SetColorMatrix(matrix);
   g.DrawImage(bitmap,Rectangle.Round(target),source.X,source.Y,source.Width,source.Height,GraphicsUnit.Pixel,attributes);
  }
 }
 void Person(Graphics g,FilmActor actor,float x,float y,double t,bool walk,int pose,bool dead,bool swim) {
  int row=walk?4:0,frame=walk?(int)(t*10)%4:0;
  if(pose==1)row=8;
  if(pose==2){row=12;frame=(int)(t*9)%3;}
  if(pose==3){row=16;frame=(int)(t*6)%4;}
  if(dead){row=18;frame=1;}
  var rect=new RectangleF(x-16,y-16,32,swim?20:32);
  Sprite(g,"chr/chr_"+(actor.skin??"def1"),rect,new Rectangle(frame*32,row*32,32,swim?20:32),Color.White);
  if(!dead) foreach(string layer in actor.layers??new string[0]) {
   string[] parts=layer.Split('@');string id=parts[0]+(parts.Length>1?"w":"");
   Sprite(g,"chr/chr_"+id,rect,new Rectangle(frame*32,row*32,32,swim?20:32),parts.Length>1?Tint(parts[1]):Color.White);
  }
  if(!dead && !String.IsNullOrEmpty(actor.back)) Sprite(g,"chr/chr_"+actor.back,rect,new Rectangle(frame*32,(walk?21:17)*32,32,32),Color.White);
  if(!dead && !String.IsNullOrEmpty(actor.hand)) Sprite(g,"chr/chr_"+actor.hand,rect,new Rectangle(frame*32,row*32,32,32),Color.White);
 }
 string CarImage(string look) {
  string[] parts=(look??"4x4").Split('@');string body=parts[0];
  if(body=="uaz"||body=="volga")return body;
  string fallback=body=="sedan"||body=="bike"||body=="moped"?"red":(body=="boat"?"blue":(body=="humvee"?"camo":(body=="plane"?"cessna":"green")));
  string result="car_"+body+"_"+(parts.Length>1?parts[1].Replace("gray","grey"):fallback);
  return Image(result)!=null?result:"car_"+body+"_"+fallback;
 }
 void Ground(Graphics g,string biome,float height,float scroll,float alpha,bool water,bool sky) {
  string id=water?"ground_water":(sky?"sky_":"ground_")+biome;
  var bitmap=Image(id); if(bitmap==null)bitmap=Image("ground_forest"); if(bitmap==null)return;
  for(float y=-(scroll%256);y<height;y+=256) Sprite(g,id,new RectangleF(0,y,256,256),new Rectangle(),Color.FromArgb((int)(255*alpha),255,255,255));
 }
 public RectangleF[] TravelVehicleBounds(float height) {
  var looks=film.Looks;
  var sizes=looks.Select(look=>look.StartsWith("truck")?new SizeF(58,111):
   (look.StartsWith("boat")?new SizeF(47,92):
   (look.StartsWith("bike")||look.StartsWith("moped")?new SizeF(30,58):
   (look.StartsWith("plane")?new SizeF(64,56):new SizeF(47,76))))).ToArray();
  if(sizes.Length==0)return new RectangleF[0];
  const float gap=24;
  float total=sizes.Sum(s=>s.Height)+gap*(sizes.Length-1);
  float fit=Math.Min(1,Math.Max(1,height-32)/total),top=(height-total*fit)/2;
  var result=new List<RectangleF>();
  foreach(var size in sizes){result.Add(new RectangleF(128-size.Width*fit/2,top,size.Width*fit,size.Height*fit));top+=(size.Height+gap)*fit;}
  return result.ToArray();
 }
 public FilmActor[] TravelWalkers() {
  // Legacy single-vehicle films have actor.car=-1 even for the driver.
  if(!String.IsNullOrEmpty(film.car)&&(film.cars==null||film.cars.Length==0))return new FilmActor[0];
  return (film.crew??new FilmActor[0]).Where(a=>film.Looks.Length==0||a.car<0).ToArray();
 }
 public string DrawTravel(Graphics g,RectangleF bounds,double elapsed) {
  var state=g.Save();
  try {
   g.SetClip(bounds);g.TranslateTransform(bounds.X,bounds.Y);
   float scale=bounds.Width/256f,height=bounds.Height/scale;g.ScaleTransform(scale,scale);
   g.InterpolationMode=InterpolationMode.NearestNeighbor;g.PixelOffsetMode=PixelOffsetMode.Half;
   bool water=film.Looks.Any(x=>x.StartsWith("boat")),sky=film.Looks.Any(x=>x.StartsWith("plane"));
   string biome=String.IsNullOrEmpty(film.biomeFrom)?"forest":film.biomeFrom;
   float scroll=(float)(elapsed*(film.Looks.Length>0?55:18));
   Ground(g,biome,height,scroll,1,water,sky);
   if(!water&&!sky)for(int i=0;i<14;i++) {
    float y=(i*53-scroll)%(height+110);if(y<0)y+=height+110;y-=55;
    Sprite(g,"props",new RectangleF(i%2==0?8:211,y,38,50),new Rectangle((i%4)*64,0,64,64),Color.White);
   }
   var rectangles=TravelVehicleBounds(height);
   for(int i=0;i<rectangles.Length;i++) {
    string id=CarImage(film.Looks[i]);var bitmap=Image(id);if(bitmap==null)continue;
    string look=film.Looks[i];int sw=look.StartsWith("truck")?58:47,sh=look.StartsWith("truck")?111:(look.StartsWith("boat")?92:76);
    if(look.StartsWith("bike")||look.StartsWith("moped")){sw=30;sh=58;}if(look.StartsWith("plane")){sw=64;sh=56;}
    Sprite(g,id,rectangles[i],new Rectangle(0,0,Math.Min(sw,bitmap.Width),Math.Min(sh,bitmap.Height)),Color.White);
   }
   var walkers=TravelWalkers();int rows=(walkers.Length+2)/3;
   if(rows>0){var castState=g.Save();float fit=Math.Min(1,Math.Max(1,height-32)/(rows*32));g.TranslateTransform(film.Looks.Length>0?84:128,height/2);g.ScaleTransform(fit,fit);for(int i=0;i<walkers.Length;i++)Person(g,walkers[i],-22+(i%3)*22,-(rows-1)*16+(i/3)*32,elapsed,true,0,false,water);g.Restore(castState);}
   return "Travelling to the destination…";
  }finally{g.Restore(state);}
 }
 public string Draw(Graphics g,RectangleF bounds,double elapsed) {
  double phase;int scene=film.SceneAt(elapsed,out phase);var e=scene>=0?film.events[scene]:null;
  var state=g.Save();g.SetClip(bounds);g.TranslateTransform(bounds.X,bounds.Y);
  float scale=bounds.Width/256f, height=bounds.Height/scale;g.ScaleTransform(scale,scale);
  g.InterpolationMode=InterpolationMode.NearestNeighbor;g.PixelOffsetMode=PixelOffsetMode.Half;
  bool complete=elapsed>=film.Duration,walking=e!=null&&(e.kind=="walk"||e.kind=="swim");
  bool cruising=e==null||e.kind=="drive"||walking;
  bool water=film.Looks.Any(x=>x.StartsWith("boat")),sky=film.Looks.Any(x=>x.StartsWith("plane"));
  float scroll=(float)film.ScrollAt(elapsed);
  double progress=Math.Min(1,elapsed/Math.Max(1,film.Duration-4));
  string from=film.biomeFrom??"forest",to=String.IsNullOrEmpty(film.biomeTo)?from:film.biomeTo;
  Ground(g,from,height,scroll,1,water,sky);
  if(!String.IsNullOrEmpty(film.biomeVia)) Ground(g,film.biomeVia,height,scroll,(float)Math.Sin(progress*Math.PI),water,sky);
  Ground(g,to,height,scroll,(float)progress,water,sky);
  // Deterministic roadside dressing. It never changes journey outcomes.
  if(!water&&!sky) for(int i=0;i<14;i++) {
   float y=(i*53-scroll)%(height+110);if(y<0)y+=height+110;y-=55;
   int cell=i%4; Sprite(g,"props",new RectangleF(i%2==0?8:211,y,38,50),new Rectangle(cell*64,0,64,64),Color.White);
  }
  var rides=film.crew.Select(a=>a.car).ToArray();var dead=new HashSet<string>();var abandoned=new HashSet<int>();
  int finished=scene<0?(complete?film.events.Length:0):scene;
  for(int n=0;n<finished;n++) {
   var prior=film.events[n];
   if(prior.kind=="died"||(prior.kind=="attack"&&prior.detail=="died"))dead.Add(String.IsNullOrEmpty(prior.who)?(film.crew.Length>0?film.crew[0].name:""):prior.who);
   if((prior.kind=="flat"&&prior.detail!="spare"&&prior.detail!="kit")||(prior.kind=="fuel"&&prior.detail=="lost")) {
    int lost=Math.Max(0,prior.car);abandoned.Add(lost);for(int k=0;k<rides.Length;k++)if(rides[k]==lost)rides[k]=-1;
   }
   if(prior.kind=="move") for(int k=0;k<rides.Length;k++)if(film.crew[k].name==prior.who)rides[k]=prior.car;
  }
  for(int c=0;c<film.Looks.Length;c++) {
   if(abandoned.Contains(c))continue;
   string look=film.Looks[c],id=CarImage(look);var bitmap=Image(id);if(bitmap==null)continue;
   float bodyH=look.StartsWith("truck")?111:(look.StartsWith("boat")?92:76),bodyW=look.StartsWith("truck")?58:47;
   float y=height*.57f+c*68;
   if(look.StartsWith("bike")||look.StartsWith("moped")){bodyH=58;bodyW=30;}
   if(look.StartsWith("plane")){bodyW=64;bodyH=56;}
   Sprite(g,id,new RectangleF(128-bodyW/2,y-bodyH/2,bodyW,bodyH),new Rectangle(0,0,(int)Math.Min(bodyW,bitmap.Width),(int)Math.Min(bodyH,bitmap.Height)),Color.White);
   if(e!=null&&e.kind=="flat"&&Math.Max(0,e.car)==c) using(var pen=new Pen(Color.OrangeRed,1))g.DrawEllipse(pen,151,y+19,7,7);
  }
  for(int k=0;k<film.crew.Length;k++) {
   var actor=film.crew[k];bool subject=e!=null&&(String.IsNullOrEmpty(e.who)?k==0:e.who==actor.name);
   bool action=e!=null&&e.kind!="drive"&&e.kind!="walk"&&e.kind!="swim";
   if(rides[k]>=0&&!walking&&!(subject&&action))continue;
   float x=106+(k%3)*22, y=height*.57f+(k/3)*26;
   if(rides[k]>=0&&action){x=165;y+=12;}
   int pose=action&&subject?(e.kind=="attack"?1:3):0;
   bool isDead=dead.Contains(actor.name)||(subject&&e!=null&&(e.kind=="died"||(e.kind=="attack"&&e.detail=="died"))&&phase>.7);
   Person(g,actor,x,y,elapsed,cruising&&!isDead,pose,isDead,e!=null&&e.kind=="swim");
   if(subject&&e!=null&&e.kind=="attack"&&phase<.85) {
    float zx=x+38-(float)(phase*28); Sprite(g,"chr/chr_zednormalskin1",new RectangleF(zx,y-16,32,32),new Rectangle(((int)(elapsed*8)%4)*32,8*32,32,32),Color.White);
   }
   if(subject&&action&&e.kind!="attack"&&e.kind!="died") {
    string prop=e.kind=="flat"?"props":(e.kind=="fuel"||e.detail=="gas"?"fueltank":(e.detail=="water"?"waterpump":"props"));
    Sprite(g,prop,new RectangleF(x+9,y+4,16,20),e.kind=="flat"?new Rectangle(192,192,64,64):new Rectangle(),Color.White);
   }
  }
  g.Restore(state);
  return complete?"The journey story is complete. Waiting for the destination…":film.Caption(e);
 }
 public void Dispose(){foreach(var bitmap in images.Values)if(bitmap!=null)bitmap.Dispose();images.Clear();}
}
}
