const API_BASE = window.COMMUNITY_API_BASE || '';
const tokenKey = 'cc_session_token';

function getToken(){ return localStorage.getItem(tokenKey); }
function setToken(token){ if(token) localStorage.setItem(tokenKey, token); }
function clearToken(){ localStorage.removeItem(tokenKey); localStorage.removeItem('cc_user'); }
function authHeaders(){ const token=getToken(); return token ? {'Authorization':`Bearer ${token}`} : {}; }

async function api(path, options={}){
  const opts={...options, headers:{'Content-Type':'application/json', ...authHeaders(), ...(options.headers||{})}};
  if(opts.body && typeof opts.body !== 'string') opts.body=JSON.stringify(opts.body);
  const res=await fetch(`${API_BASE}${path}`,opts);
  let data=null; const text=await res.text();
  try{ data=text ? JSON.parse(text) : null; }catch{ data={message:text}; }
  if(!res.ok){ const err=new Error(data?.message || data?.error || `Request failed (${res.status})`); err.status=res.status; err.data=data; throw err; }
  return data;
}

const API={
 health:()=>api('/api/health'),
 categories:()=>api('/api/categories'),
 register:(body)=>api('/api/auth/register',{method:'POST',body}),
 login:(body)=>api('/api/auth/login',{method:'POST',body}),
 logout:()=>api('/api/auth/logout',{method:'POST'}),
 me:()=>api('/api/auth/me'),
 profile:()=>api('/api/profile'),
 updateProfile:(body)=>api('/api/profile',{method:'PUT',body}),
 resources:(query='')=>api(`/api/resources${query}`),
 resource:(id)=>api(`/api/resources/${encodeURIComponent(id)}`),
 createResource:(body)=>api('/api/resources',{method:'POST',body}),
 updateResource:(id,body)=>api(`/api/resources/${encodeURIComponent(id)}`,{method:'PUT',body}),
 deleteResource:(id)=>api(`/api/resources/${encodeURIComponent(id)}`,{method:'DELETE'}),
 resourceAvailability:(id,available)=>api(`/api/resources/${encodeURIComponent(id)}/availability`,{method:'PATCH',body:{available}}),
 requests:(query='')=>api(`/api/requests${query}`),
 request:(id)=>api(`/api/requests/${encodeURIComponent(id)}`),
 createRequest:(body)=>api('/api/requests',{method:'POST',body}),
 updateRequest:(id,body)=>api(`/api/requests/${encodeURIComponent(id)}`,{method:'PUT',body}),
 acceptRequest:(id)=>api(`/api/requests/${encodeURIComponent(id)}/accept`,{method:'POST'}),
 rejectRequest:(id)=>api(`/api/requests/${encodeURIComponent(id)}/reject`,{method:'POST'}),
 cancelRequest:(id)=>api(`/api/requests/${encodeURIComponent(id)}/cancel`,{method:'POST'}),
 completeRequest:(id)=>api(`/api/requests/${encodeURIComponent(id)}/complete`,{method:'POST'}),
 requestContact:(id)=>api(`/api/requests/${encodeURIComponent(id)}/contact`),
 requestMatches:(id)=>api(`/api/requests/${encodeURIComponent(id)}/matches`),
 priorityNext:()=>api('/api/requests/priority/next'),
 searchResources:(q)=>api(`/api/search/resources?q=${encodeURIComponent(q)}`),
 searchUsers:(skill)=>api(`/api/search/users?skill=${encodeURIComponent(skill)}`),
 dashboard:()=>api('/api/dashboard'),
 notifications:()=>api('/api/notifications'),
 readNotification:(id)=>api(`/api/notifications/${encodeURIComponent(id)}/read`,{method:'POST'}),
 readAllNotifications:()=>api('/api/notifications/read-all',{method:'POST'})
};

window.API=API; window.Auth={getToken,setToken,clearToken};
