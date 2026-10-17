const $=s=>document.querySelector(s), app=$("#app");
const state={region:localStorage.region||"ALL",date:new Date().toISOString().slice(0,10)};
const ICON=`<svg viewBox="0 0 24 24" fill="none" stroke="#5aa2ff" stroke-width="2"><circle cx="12" cy="12" r="10" fill="#10204a"/><path d="M12 6l2 4 4 .6-3 2.9.7 4.1-3.7-2-3.7 2 .7-4.1-3-2.9 4-.6z" fill="#5aa2ff" stroke="none"/></svg>`;
const pts=n=>`<span class="pts">${ICON}${n.toLocaleString()}</span>`;
const isLive=t=>t.start<=Date.now()&&Date.now()<t.end;
const ymd=ms=>{const d=new Date(ms);return d.getFullYear()+"-"+String(d.getMonth()+1).padStart(2,"0")+"-"+String(d.getDate()).padStart(2,"0")};
const fmtT=ms=>new Date(ms).toLocaleTimeString([],{hour:"numeric",minute:"2-digit"});
const fmtD=ms=>new Date(ms).toLocaleDateString([],{month:"long",day:"numeric",year:"numeric"});
const dur=t=>{const m=Math.round((t.end-t.start)/6e4);return Math.floor(m/60)+" hours"+(m%60?" "+m%60+" minutes":"")};
const regionPills=(cur,fn,all)=>`<div class="pills">${(all?["ALL"]:[]).concat(REGIONS).map(r=>`<span class="pill ${r===cur?"on":""}" data-r="${r}">${r}</span>`).join("")}</div>`;
const card=t=>`<a class="card" href="#/t/${t.id}"><div class="row"><span class="badge ${isLive(t)?"live":""}">${isLive(t)?"● LIVE":t.region}</span>${pts(t.pts.qual)}</div>
<h3>${t.name} <span class="meta">[${t.region}]</span></h3><div class="meta">${t.session} · ${t.format}</div><div class="meta">${fmtD(t.start)} · ${fmtT(t.start)}</div></a>`;
const section=(title,list,empty)=>`<h2>${title} <span class="badge">${list.length}</span></h2>`+(list.length?`<div class="grid">${list.map(card).join("")}</div>`:`<div class="empty">${empty}</div>`);

function home(){
  const all=TOURNAMENTS.filter(t=>state.region==="ALL"||t.region===state.region).sort((a,b)=>a.start-b.start);
  const today=ymd(Date.now());
  const live=all.filter(isLive);
  const todays=all.filter(t=>!isLive(t)&&ymd(t.start)===today&&t.end>Date.now());
  const picked=all.filter(t=>ymd(t.start)===state.date);
  app.innerHTML=`<div class="bar"><div><h1>Tournaments</h1><div class="meta">Fortnite competitive events, all regions</div></div>
  <div class="datebox"><button id="prev">‹</button><input type="date" id="date" value="${state.date}"><button id="next">›</button><button id="tod">Today</button></div></div>
  ${regionPills(state.region,0,true)}
  ${section("🔴 Live now",live,"No tournaments are live right now.")}
  ${section("Happening today",todays,"Nothing else scheduled for today.")}
  ${section("On "+new Date(state.date+"T12:00").toLocaleDateString([],{weekday:"long",month:"long",day:"numeric"}),picked,"No tournaments on this date.")}`;
  app.querySelectorAll(".pill").forEach(p=>p.onclick=()=>{state.region=localStorage.region=p.dataset.r;home()});
  const setD=d=>{state.date=d;home()};
  $("#date").onchange=e=>e.target.value&&setD(e.target.value);
  const shift=n=>{const d=new Date(state.date+"T12:00");d.setDate(d.getDate()+n);setD(ymd(d))};
  $("#prev").onclick=()=>shift(-1);$("#next").onclick=()=>shift(1);$("#tod").onclick=()=>setD(ymd(Date.now()));
}

const rnd=(s,i)=>{let x=Math.sin(s*9301+i*49297)*233280;return x-Math.floor(x)};
function streams(t){
  const live=isLive(t);
  const list=STREAMERS.slice(0,live?6:4).map((n,i)=>({n,v:Math.round(150+rnd(t.id.length,i)*9000)}));
  return `<h2>📺 Twitch streams ${live?`<span class="badge live">● LIVE</span>`:`<span class="badge">Upcoming</span>`}</h2><div class="streams">`+
  list.map(s=>`<a class="stream" target="_blank" rel="noopener" href="https://twitch.tv/${s.n.toLowerCase()}"><div class="thumb">🎮${live?`<span class="badge live">LIVE</span><span class="v">👁 ${s.v.toLocaleString()}</span>`:""}</div><div class="s"><b>${s.n}</b><span class="meta">${live?"Streaming "+t.name:"Expected to stream"}</span></div></a>`).join("")+`</div>`;
}
function tabBody(t,tab){
  const rows=TEAMS.map((n,i)=>({n,p:Math.round(300-i*28+rnd(i,t.id.length)*20)}));
  if(tab==="Event Info")return `<div class="two"><div class="panel"><h2 style="margin-top:0">Qualification rewards</h2><div class="qual">
   <div class="qbox"><div class="meta">Qualify at</div><div class="big">${ICON}${t.pts.qual}</div><div class="meta">tournament points</div></div>
   ${t.pts.top.map((p,i)=>`<div class="qbox"><div class="meta">Top ${[1,5,10][i]}</div><div class="big">${ICON}${p}</div><div class="meta">points</div></div>`).join("")}</div></div>
   <div class="panel"><h2 style="margin-top:0">About</h2><p class="meta">Compete for tournament points. Reach the qualification threshold to advance. Description provided by Epic Games and could be outdated.</p></div></div>${streams(t)}`;
  if(tab==="Leaderboards"||tab==="Team Stats")return `<div class="panel"><table><tr><th>#</th><th>Team</th><th>Points</th></tr>${rows.map((r,i)=>`<tr><td>${i+1}</td><td>${r.n}</td><td>${r.p}</td></tr>`).join("")}</table></div>`;
  if(tab==="Player Stats")return `<div class="panel"><table><tr><th>Player</th><th>Elims</th><th>Avg place</th></tr>${STREAMERS.map((n,i)=>`<tr><td>${n}</td><td>${20+i*3}</td><td>${(3+rnd(i,2)*8).toFixed(1)}</td></tr>`).join("")}</table></div>`;
  if(tab==="Event Stats")return `<div class="panel two"><div><div class="meta">Matches</div><b style="font-size:26px">${t.matches}</b></div><div><div class="meta">Qualify threshold</div>${pts(t.pts.qual)}</div></div>`;
  return `<div class="panel"><table><tr><th>Time</th><th>Event</th></tr><tr><td>${fmtT(t.start)}</td><td>Matchmaking opens</td></tr><tr><td>${fmtT(t.end)}</td><td>Session ends</td></tr></table></div>`;
}
function detail(id,tab="Event Info"){
  const t=TOURNAMENTS.find(x=>x.id===id); if(!t){app.innerHTML="<p>Not found. <a href='#/'>Back</a></p>";return}
  const state2=isLive(t)?"Live now":t.end<Date.now()?"Ended":"Upcoming";
  app.innerHTML=`<a class="back" href="#/">← Back to Tournaments</a>
  <div class="pills">${REGIONS.map(r=>`<span class="pill ${r===t.region?"on":""}" data-r="${r}">${r}</span>`).join("")}</div>
  <h1>${t.name} <small>[${t.region}]</small></h1><p class="sub">${t.session}</p>
  <div class="info"><div class="mode">${t.mode}</div><b>Map:</b> ${t.map}<br><b>Format:</b> ${t.format} | <b>Matches:</b> ${t.matches}<br><b>Platforms:</b> ${t.platforms}<br>
  <b>Date:</b> ${fmtD(t.start)} at ${fmtT(t.start)} | <b>Duration:</b> ${dur(t)} | ${state2}<br><b>Qualifies at:</b> ${pts(t.pts.qual)}</div>
  <div class="btns"><span class="btn">Show Requirements</span><span class="btn ${isLive(t)?"":"dis"}">Open In-Game →</span></div>
  <div class="tabs">${["Event Info","Leaderboards","Team Stats","Player Stats","Event Stats","Timeline"].map(x=>`<span class="tab ${x===tab?"on":""}" data-t="${x}">${x}</span>`).join("")}</div>${tabBody(t,tab)}`;
  app.querySelectorAll(".tab").forEach(e=>e.onclick=()=>detail(id,e.dataset.t));
  // region pills: jump to the home list filtered by region
  app.querySelectorAll(".pills .pill").forEach(p=>p.onclick=()=>{state.region=localStorage.region=p.dataset.r;location.hash="#/"});
}
function route(){const m=location.hash.match(/^#\/t\/(.+)/);window.scrollTo(0,0);m?detail(m[1]):home()}
addEventListener("hashchange",route);route();
