const $=s=>document.querySelector(s), app=$("#app");
const state={region:localStorage.region||"ALL",date:null,q:""};
const ICON=`<svg viewBox="0 0 24 24" fill="none" stroke="#5aa2ff" stroke-width="2"><circle cx="12" cy="12" r="10" fill="#10204a"/><path d="M12 6l2 4 4 .6-3 2.9.7 4.1-3.7-2-3.7 2 .7-4.1-3-2.9 4-.6z" fill="#5aa2ff" stroke="none"/></svg>`;
const pts=n=>`<span class="pts">${ICON}${n.toLocaleString()}</span>`;
const isLive=t=>t.start<=Date.now()&&Date.now()<t.end;
const ymd=ms=>{const d=new Date(ms);return d.getFullYear()+"-"+String(d.getMonth()+1).padStart(2,"0")+"-"+String(d.getDate()).padStart(2,"0")};
const fmtT=ms=>new Date(ms).toLocaleTimeString([],{hour:"numeric",minute:"2-digit"});
const fmtD=ms=>new Date(ms).toLocaleDateString([],{month:"long",day:"numeric",year:"numeric"});
const dur=t=>{const m=Math.round((t.end-t.start)/6e4);return Math.floor(m/60)+" hours"+(m%60?" "+m%60+" minutes":"")};
const regionPills=(cur,fn,all)=>`<div class="pills">${(all?["ALL"]:[]).concat(REGIONS).map(r=>`<span class="pill ${r===cur?"on":""}" data-r="${r}">${r}</span>`).join("")}</div>`;
const rel=ms=>{const m=Math.round(Math.abs(ms)/6e4),h=Math.round(m/60),d=Math.round(h/24);return m<60?m+" min"+(m===1?"":"s"):h<48?h+" hour"+(h===1?"":"s"):d+" days"};
const when=t=>isLive(t)?"ends in "+rel(t.end-Date.now()):t.end<Date.now()?"ended "+rel(Date.now()-t.end)+" ago":"in "+rel(t.start-Date.now());
const ART=[["#0b2a7a","#2f7bff"],["#06122e","#3a6bff"],["#1a0b5e","#5aa2ff"],["#021a3a","#00b4ff"],["#0a1f66","#7a5cff"]];
const card=(t,small)=>{const c=ART[(t.name.length+t.region.length)%ART.length];
 return `<a class="tcard ${small?"sm":""}" href="#/t/${t.id}"><div class="art" style="background:linear-gradient(150deg,${c[0]},${c[1]})"><i></i><span class="mark">${t.name.split(" ").map(w=>w[0]).join("").slice(0,3)}</span></div>
 <span class="chip ${isLive(t)?"lv":""}">${when(t)}</span>${small?`<span class="chip r">${t.matches} Events</span>`:`<span class="chip r">${t.region}</span>`}
 <div class="cap"><b>${t.name}</b><span>${small?(isLive(t)?"Running: ":"Upcoming: ")+t.session:t.session}</span><em>${ICON}${t.pts.qual}</em></div></a>`};
const section=(title,list,empty,small,right)=>`<section class="sect"><div class="sh"><h2><span class="chev">⌄</span>${title}</h2><span class="meta">${right||""}</span></div>`+(list.length?`<div class="tgrid ${small?"sm":""}">${list.map(t=>card(t,small)).join("")}</div>`:`<div class="empty">${empty}</div>`)+`</section>`;

function home(){
  const q=(state.q||"").toLowerCase();
  const all=TOURNAMENTS.filter(t=>(state.region==="ALL"||t.region===state.region)&&(t.name+t.session).toLowerCase().includes(q)).sort((a,b)=>a.start-b.start);
  const open=all.filter(t=>t.end>Date.now());
  const quick=open.filter(t=>isLive(t)||t.start-Date.now()<6*H);
  const rest=state.date?all.filter(t=>ymd(t.start)===state.date):open;
  app.innerHTML=`<h1>Pump Tournaments</h1>
  <div class="tools"><input id="q" class="search" placeholder="Search for tournaments" value="${state.q||""}">
   <div class="datebox"><button id="prev">‹</button><input type="date" id="date" value="${state.date||ymd(Date.now())}"><button id="next">›</button><button id="tod">${state.date?"Show all":"Today"}</button></div>
   <div class="regs">${["ALL"].concat(REGIONS).map(r=>`<span class="rg ${r===state.region?"on":""}" data-r="${r}">${r}</span>`).join("")}</div></div>
  ${section("Quick Access",quick,"Nothing live or starting in the next few hours.",false,"Live + starting soon")}
  ${section(state.date?"Events on "+new Date(state.date+"T12:00").toLocaleDateString([],{weekday:"long",month:"long",day:"numeric"}):"Upcoming Events",rest,"No tournaments on this date.",true,"Pick a date to see other days")}`;
  app.querySelectorAll(".rg").forEach(p=>p.onclick=()=>{state.region=p.dataset.r;try{localStorage.region=p.dataset.r}catch(e){}home()});
  $("#q").oninput=e=>{state.q=e.target.value;const p=e.target.selectionStart;home();const n=$("#q");n.focus();n.setSelectionRange(p,p)};
  const setD=d=>{state.date=d;home()};
  $("#date").onchange=e=>e.target.value&&setD(e.target.value);
  const base=()=>state.date||ymd(Date.now());
  const shift=n=>{const d=new Date(base()+"T12:00");d.setDate(d.getDate()+n);setD(ymd(d))};
  $("#prev").onclick=()=>shift(-1);$("#next").onclick=()=>shift(1);$("#tod").onclick=()=>setD(state.date?null:ymd(Date.now()));
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
