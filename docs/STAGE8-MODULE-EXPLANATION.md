# Stage 8 - Module Explanation for Viva / PBL Presentation

## Module 1 - User Management

Responsible for registration, login, logout and profile management.

Important points:
- Email and username uniqueness are checked through hash indexes.
- Passwords are not stored as plain text.
- Each password gets a unique salt.
- PBKDF2-HMAC-SHA256 is used through the platform cryptographic API.
- Session tokens identify authenticated users.

## Module 2 - Resource Management

A user can create, view, edit, delete and change availability of educational resources.

The backend validates the category against the nine predefined educational categories.

Ownership is checked on every mutation.

## Module 3 - Help Requests

Users can create requests with High, Medium or Low priority.

The custom binary heap determines which pending request is most urgent.

The request state machine is:

```text
Pending -> Accepted -> Completed
Pending -> Rejected
Pending -> Cancelled
```

Invalid transitions are rejected by the backend.

## Module 4 - Search and Matching

Resources can be searched by text/category and users can be searched by skills.

Matching compares required skills with user skills and ranks stronger matches higher.

No machine learning is required.

## Module 5 - Notifications

Notifications are generated for resource requests and request lifecycle changes.

Each notification belongs to one receiver, and authorization is checked before it can be read or marked as read.

## Contact Privacy

Contact details are not merely hidden in HTML.

The backend checks whether the current user is a party to an accepted/completed request before returning the other person's private contact information.

This is important because a user cannot bypass the restriction by manually editing a browser request.

## File Handling

The application uses C++ file streams and `.txt` records.

Records are escaped before writing so descriptions containing delimiters do not corrupt the file format.

Updates use temporary files and replacement rather than blindly overwriting the original record file.

## Frontend / Backend Communication

The frontend uses vanilla JavaScript and `fetch()`.

Conceptually:

```text
Browser
   |
   | HTTP + JSON
   v
C++ HTTP API
   |
   v
Service classes
   |
   +---- Priority Queue / Hash Maps / Vectors / Searching
   |
   v
FileManager
   |
   v
users.txt / resources.txt / requests.txt / notifications.txt
```
