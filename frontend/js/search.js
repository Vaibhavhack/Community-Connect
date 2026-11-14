const escS=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

document.addEventListener('DOMContentLoaded',async()=>{
  if(!['search.html'].includes(location.pathname.split('/').pop()))return;
  const u=await requireAuth();
  if(!u)return;

  document.getElementById('search-form').addEventListener('submit',async e=>{
    e.preventDefault();
    const q=document.getElementById('search-input').value.trim();
    const type=document.getElementById('search-type').value;
    const out=document.getElementById('search-results');

    if(!q){
      out.innerHTML='<div class="empty">Enter a search term.</div>';
      return;
    }

    try{
      const d=type==='users'?await API.searchUsers(q):await API.searchResources(q);
      const arr=d.results||d.resources||d.users||d||[];

      if(!arr.length){
        out.innerHTML='<div class="empty">No matching results.</div>';
        return;
      }

      out.innerHTML=arr.map(x=>{
        if(type==='users'){
          return `<div class="list-item">
            <div class="list-main">
              <div class="list-title">${escS(x.name)}</div>
              <div class="muted">${escS(x.city||x.locality||'Community member')}</div>
              <p>${escS(x.skills||x.interests||'')}</p>
            </div>
            <span class="badge">${escS(x.relevance??x.score??'Match')}</span>
          </div>`;
        }

        const title=escS(x.title||'Resource');
        const category=escS(x.category||'');
        const description=escS(x.description||'');
        const id=encodeURIComponent(x.id);
        const queryTitle=encodeURIComponent(x.title||'');
        const queryCategory=encodeURIComponent(x.category||'');

        return `<div class="list-item">
          <div class="list-main">
            <div class="list-title">${title}</div>
            <div class="muted">${category}</div>
            <p>${description}</p>
          </div>
          <div class="actions">
            <span class="badge">${escS(x.relevance??x.score??'Match')}</span>
            <a class="btn btn-secondary" href="resource-details.html?id=${id}">View</a>
            <a class="btn btn-primary" href="create-request.html?resourceId=${id}&title=${queryTitle}&category=${queryCategory}">Request Help</a>
          </div>
        </div>`;
      }).join('');
    }catch(e){
      showMessage('page-alert',e.message);
    }
  });
});
