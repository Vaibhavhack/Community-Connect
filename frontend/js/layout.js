function setupNav(){
 const toggle=document.querySelector('.mobile-toggle'); const links=document.querySelector('.nav-links');
 if(toggle) toggle.addEventListener('click',()=>links?.classList.toggle('open'));
 const current=location.pathname.split('/').pop()||'index.html';
 document.querySelectorAll('.nav-links a').forEach(a=>{if(a.getAttribute('href')===current)a.classList.add('active');});
}
function setupModal(){document.querySelectorAll('[data-close-modal]').forEach(b=>b.addEventListener('click',()=>document.querySelector(b.dataset.closeModal||'.modal')?.classList.add('hidden')))}
function shell(active=''){
 return `<header class="topbar"><div class="container nav"><a class="brand" href="dashboard.html"><span class="brand-mark">CC</span>Community Connect</a><button class="mobile-toggle icon-btn">☰</button><nav class="nav-links"><a href="dashboard.html">Dashboard</a><a href="resources.html">Resources</a><a href="requests.html">Requests</a><a href="search.html">Match</a><a href="notifications.html">Notifications</a><a href="profile.html">Profile</a><a href="#" data-logout>Logout</a></nav><div class="nav-user"><span class="avatar" data-user-avatar>U</span><span class="muted" data-user-name>User</span></div></div></header>`;
}
function injectShell(){const el=document.querySelector('[data-shell]');if(el)el.outerHTML=shell();setupNav();bindLogout();}
