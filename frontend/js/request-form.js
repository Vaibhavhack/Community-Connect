(async()=>{
 const me=await fillNav();if(!me)return;
 const form=document.getElementById('request-form'),msg=document.getElementById('form-msg'),resourceId=qs('resourceId');
 try{const c=await apiGet('/api/categories');const arr=Array.isArray(c)?c:(c.categories||[]);document.getElementById('category').innerHTML='<option value="">General help</option>'+arr.map(x=>`<option>${esc(typeof x==='string'?x:x.name)}</option>`).join('');}catch(_){}
 if(resourceId){document.getElementById('resourceId').value=resourceId;document.getElementById('resource-note').textContent=`Requesting resource: ${qs('title')||'selected resource'}`;}
 form.onsubmit=async e=>{e.preventDefault();hideMessage(msg);const data={resourceId:Number(resourceId.value||0),title:title.value.trim(),description:description.value.trim(),requiredSkills:requiredSkills.value.trim(),category:category.value,priority:priority.value};try{await apiPost('/api/requests',data);location.href='requests.html';}catch(e){showMessage(msg,e.message)}};
})();
