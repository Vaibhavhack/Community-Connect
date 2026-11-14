# Community Connect Frontend

Vanilla HTML/CSS/JavaScript frontend for the Community Resource and Help Exchange Platform.

The frontend expects the Stage 5 C++ HTTP server to expose the `/api/...` endpoints documented in the project architecture. The same C++ server should serve this directory as its static web root.

If the frontend is served from the same origin as the C++ server, `js/api.js` uses relative API URLs automatically.
