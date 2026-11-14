document.addEventListener('DOMContentLoaded',()=>{document.querySelectorAll('[data-nav]').forEach(a=>{if(a.getAttribute('href')===location.pathname.split('/').pop())a.classList.add('active')})});
