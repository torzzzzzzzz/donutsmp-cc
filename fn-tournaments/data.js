// Mock data – swap for a real API later. Times are relative to "now" so the demo always has live/today events.
const REGIONS=["OCE","ASIA","ME","EU","BR","NAC","NAW"];
const H=3600e3, now=Date.now();
const mk=(id,name,session,region,startH,durH,format,matches,pts)=>({id,name,session,region,start:now+startH*H,end:now+(startH+durH)*H,format,matches,pts,
  map:"Reload - Nitemare Island",mode:"Points-Based Matchmaking (ELO)",platforms:"Mobile, PC, PlayStation, Switch, Xbox"});
const TOURNAMENTS=[
 mk("sfc-nac-1","Street Fighter Cup Reload","Session 1","NAC",-1,2.5,"Duos (max 40 players)",12,{qual:300,top:[300,200,100]}),
 mk("sfc-eu-1","Street Fighter Cup Reload","Session 1","EU",-.5,2.5,"Duos (max 40 players)",12,{qual:250,top:[250,150,75]}),
 mk("sfc-nac-2","Street Fighter Cup Reload","Session 2","NAC",3,2.5,"Duos (max 40 players)",12,{qual:300,top:[300,200,100]}),
 mk("zb-naw-1","Zero Build Showdown","Finals","NAW",5,3,"Trios (max 60 players)",10,{qual:400,top:[400,250,150]}),
 mk("cc-br-1","Champion Cash Cup","Day 1","BR",2,3,"Solos",8,{qual:500,top:[500,300,200]}),
 mk("oce-1","OCE Weekly Cup","Round 1","OCE",.5,2,"Duos",6,{qual:150,top:[150,100,50]}),
 mk("asia-1","ASIA Elite Series","Semifinal","ASIA",26,3,"Duos",10,{qual:350,top:[350,200,100]}),
 mk("me-1","ME Frenzy Cup","Session 1","ME",50,2.5,"Trios",8,{qual:200,top:[200,120,60]}),
 mk("sfc-nac-0","Street Fighter Cup Reload","Warm-up","NAC",-30,2.5,"Duos (max 40 players)",12,{qual:100,top:[100,60,30]}),
];
const STREAMERS=["Nova","ClixLite","ArcaneFN","Jynx","Peterbot","Zaylo","Mongraal2","Kairo"];
const TEAMS=["Team Apex","Nite Owls","Void Squad","Blue Ridge","Storm Cell","Pulse","Helix","Orbit"];
