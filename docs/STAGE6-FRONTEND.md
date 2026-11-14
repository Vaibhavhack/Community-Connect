# Stage 6 - Community Connect Frontend

Stage 6 adds the complete vanilla HTML/CSS/JavaScript interface. The frontend is intentionally framework-free so it remains easy to explain in a college PBL viva.

## Pages

- `index.html` - landing page
- `register.html` - registration
- `login.html` - login
- `dashboard.html` - dashboard/statistics/urgent request/recent notifications
- `profile.html` - profile view/edit
- `resources.html` - resource browsing, category filtering and availability filtering
- `resource-details.html` - resource details and request action
- `add-resource.html` - create/edit resource
- `create-request.html` - create help request with priority
- `requests.html` - request tracking and owner actions
- `search.html` - resource and skill search
- `notifications.html` - notification list/read controls

## JavaScript modules

- `api.js` - common HTTP client, bearer token and escaping helpers
- `auth.js` - session checks/logout/navigation identity
- `layout.js` - shared navigation
- `dashboard.js` - dashboard data
- `resources.js` - resource browsing/search/filtering
- `resource-form.js` - create/edit resource
- `detail.js` - resource detail
- `request-form.js` - create request
- `requests.js` - request lifecycle actions
- `search.js` - ranked resource/user search
- `notifications.js` - notification UI
- `profile.js` - profile editing
- `login.js`, `register.js` - authentication forms

## API contract used by the frontend

The UI calls the Stage 5 C++ API with `fetch()` and sends the session token as:

`Authorization: Bearer <token>`

Routes used:

- `POST /api/auth/register`
- `POST /api/auth/login`
- `POST /api/auth/logout`
- `GET /api/auth/me`
- `GET/PUT /api/profile`
- `GET /api/categories`
- `GET /api/search/resources`
- `GET /api/search/users`
- `GET /api/resources/:id`
- `POST /api/resources`
- `PUT /api/resources/:id`
- `GET /api/requests`
- `POST /api/requests`
- `POST /api/requests/:id/accept`
- `POST /api/requests/:id/reject`
- `POST /api/requests/:id/cancel`
- `POST /api/requests/:id/complete`
- `GET /api/requests/:id/contact`
- `GET /api/dashboard`
- `GET /api/notifications`
- `POST /api/notifications/:id/read`
- `POST /api/notifications/read-all`

## Security note

The frontend only sends the authenticated session token and does not contain password hashing or contact-privacy logic. Contact information is requested only after an accepted request. The C++ backend remains the authority and must reject unauthorized contact requests even if someone edits the browser request manually.

## Running

The intended Stage 5 server serves the `frontend/` directory from `http://127.0.0.1:8080/`. Do not open the HTML using `file://` when testing the real API because the application is designed around the local C++ HTTP server.
