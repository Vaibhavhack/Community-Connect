document.addEventListener('DOMContentLoaded',async()=>{
 const user=await requireAuth(); if(!user)return;
 try{const d=await API.dashboard(); const x=d.dashboard||d; document.getElementById('stat-resources').textContent=x.totalResources??x.resourcesCount??0;document.getElementById('stat-active').textContent=x.activeRequests??x.activeRequestsCount??0;document.getElementById('stat-pending').textContent=x.pendingRequests??x.pendingRequestsCount??0;document.getElementById('stat-accepted').textContent=x.acceptedRequests??x.acceptedRequestsCount??0;
 const list=document.getElementById('recent-list'); const notes=x.recentNotifications||[]; list.innerHTML=notes.length?notes.slice(0,5).map(n=>`<div class="list-item"><div><div class="list-title">${escapeHtml(n.message||'Notification')}</div><div class="muted">${escapeHtml(n.timestamp||'')}</div></div><span class="badge">${escapeHtml(n.type||'Info')}</span></div>`).join(''):'<div class="empty">No recent notifications.</div>';
 }catch(e){showMessage('page-alert',e.message)}
});
function escapeHtml(v=''){return String(v).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
