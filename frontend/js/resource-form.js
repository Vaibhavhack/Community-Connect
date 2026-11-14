(async()=>{
 const me=await fillNav();if(!me)return;
 const form=document.getElementById('resource-form'), msg=document.getElementById('form-msg'), id=qs('id');
 try{const c=await apiGet('/api/categories');const arr=Array.isArray(c)?c:(c.categories||[]);document.getElementById('category').innerHTML=arr.map(x=>`<option>${esc(typeof x==='string'?x:x.name)}</option>`).join('');}catch(e){showMessage(msg,e.message);return}
 if(id){document.getElementById('form-title').textContent='Edit resource';try{const r=await apiGet('/api/resources/'+id);document.getElementById('title').value=r.title||'';document.getElementById('description').value=r.description||'';document.getElementById('category').value=r.category||'';}catch(e){showMessage(msg,e.message)}}
 form.onsubmit=async e=>{e.preventDefault();hideMessage(msg);const data={title:title.value.trim(),description:description.value.trim(),category:category.value};try{if(id)await apiPut('/api/resources/'+id,data);else await apiPost('/api/resources',data);location.href='resources.html';}catch(e){showMessage(msg,e.message)}};
})();
