# Stage 8 - Sample Data Plan

The demo data should be clearly separated from real student data. Never overwrite `users.txt`, `resources.txt`, `requests.txt` or `notifications.txt` merely because the server starts.

Recommended demonstration records:

## Users

1. Aditi Sharma - C++, DSA, Web Development - Pune / Kothrud
2. Rohan Mehta - Java, SQL, Academic Projects - Delhi / Rohini
3. Neha Verma - Python, Machine Learning, Notes - Dehradun / Rajpur Road
4. Arjun Singh - C++, OOP, Competitive Programming - Jaipur / Malviya Nagar

Use a single known demonstration password only in a local classroom/demo environment, and never use it for a real account.

## Resources

- C++ Data Structures Notes
- Advanced Java Previous Year Questions
- DBMS Previous Year Question Papers
- Academic Project Report Templates
- Programming Practice Problems

## Help Requests

Create examples with:
- one High-priority request
- one Medium-priority request
- one Low-priority request

This makes the heap ordering easy to demonstrate during viva.

## Notifications

Generate notifications through real request actions rather than manually editing the notification file. This demonstrates that the notification module is integrated with the request state machine.

## Important

The currently available uploaded archive is Stage 6 and does not contain the Stage 7 HTTP server source. Therefore an automatic seed command for the final HTTP build is intentionally not claimed here. Once the Stage 7 archive is restored, the sample records should be inserted through a one-time `--seed-demo` command that first checks whether the demo username/email already exists.
