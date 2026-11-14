function showMessage(id,msg,type='error'){const el=document.getElementById(id);if(!el)return;el.textContent=msg;el.className=`alert alert-${type}`;el.classList.remove('hidden')}
function initials(name='User'){return name.trim().split(/\s+/).slice(0,2).map(x=>x[0]).join('').toUpperCase()}
async function requireAuth(){
  if(!Auth.getToken()){ location.href='login.html'; return null; }
  try{ const data=await API.me(); localStorage.setItem('cc_user',JSON.stringify(data.user||data)); return data.user||data; }
  catch{ Auth.clearToken(); location.href='login.html'; return null; }
}
function bindLogout(){document.querySelectorAll('[data-logout]').forEach(btn=>btn.addEventListener('click',async()=>{try{await API.logout()}catch{} Auth.clearToken(); location.href='index.html';}));}
function setUserUI(user){document.querySelectorAll('[data-user-name]').forEach(x=>x.textContent=user?.name||'User');document.querySelectorAll('[data-user-initials]').forEach(x=>x.textContent=initials(user?.name));}
document.addEventListener('DOMContentLoaded',()=>{bindLogout();const u=JSON.parse(localStorage.getItem('cc_user')||'null');if(u)setUserUI(u);});
window.showMessage=showMessage;window.requireAuth=requireAuth;window.initials=initials;window.setUserUI=setUserUI;
