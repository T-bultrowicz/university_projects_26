# server.py
import socket
import time
import threading
import os
from urllib.parse import unquote_plus

MIME_TYPES = {
    ".html": "text/html",
    ".css":  "text/css",
    ".js":   "application/javascript",
    ".png":  "image/png",
    ".jpg":  "image/jpeg",
    ".ico":  "image/x-icon",
}

def read_file(path):
    public_root = os.path.abspath("public")
    abs_path    = os.path.abspath(os.path.join("public", path.lstrip("/")))
    if not abs_path.startswith(public_root + os.sep):
        return b"<h1>403 Forbidden</h1>", "403 Forbidden", "text/html"

    ext  = os.path.splitext(abs_path)[1].lower()
    mime = MIME_TYPES.get(ext, "application/octet-stream")
    try:
        with open(abs_path, "rb") as f:
            return f.read(), "200 OK", mime
    except FileNotFoundError:
        return b"<h1>404 Not Found</h1>", "404 Not Found", "text/html"


def parse_request(request_data):
    if not request_data:
        return ""
    request_data = request_data.split("\n")
    firstline = request_data[0].split(" ")
    return firstline[1]


def generate_response(content, status="200 OK", mime="text/html"):
    if isinstance(content, str):
        content = content.encode()
    response_line    = f"HTTP/1.1 {status}\r\n"
    response_headers = (
        f"Content-Type: {mime}\r\n"
        f"Cache-Control: no-store, must-revalidate\r\n"
        f"Content-Length: {len(content)}\r\n\r\n"
    )
    return response_line.encode() + response_headers.encode() + content

VISITOR_COUNT = 0
lock = threading.Lock()

def parse_post_body(post_body):
    parts = post_body.split("\r\n\r\n", 1)
    if (len(parts) < 2):
        return {}
    parts = parts[1].split("&")
    res = {}
    for part in parts:
        key, value = part.split("=", 1)
        res[unquote_plus(key, encoding="utf-8")] = unquote_plus(value, encoding="utf-8")

    return res

def thank_page(user_name):
    try:
        with open("./public/thanks.html", "r", encoding="utf-8") as f:
            content = f.read()
        content = content.replace("USER_NAME", user_name)
        return content.encode("utf-8")
    except FileNotFoundError:
        return b"<h1>404 Not Found</h1>"

def handle_client(client):
    global VISITOR_COUNT

    # Receive raw bytes (buffer size 1024)
    request_data = client.recv(1024).decode('utf-8')

    path = parse_request(request_data)
    content, status, mime = read_file(path if path != "/" else "/index.html")
    response = generate_response(content, status, mime)
    if path =="/submit":
        parsed_form = parse_post_body(request_data)
        response = generate_response(thank_page(parsed_form["name"]))

    # our response
    if path == "/favicon.ico":
        response = generate_response("", "404 Not Found")
    else:
        with lock:
            VISITOR_COUNT += 1

    client.sendall(response)
    client.close()

def start_server():
    # 1. Create a socket object (IPv4, TCP)
    # AF_INET = IPv4, SOCK_STREAM = TCP
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Allow the port to be reused immediately (prevents "Address already in use" errors)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind the socket to 'localhost' and port 8000
    server_socket.bind(('0.0.0.0', 8000))

    # Start listening for connections (backlog of 5)
    server_socket.listen(5)
    print("Server running on http://localhost:8000 ...")

    while True:
        # Accept a new connection
        client_connection, client_address = server_socket.accept()
        print(f"Connection received!")
        robotnik = threading.Thread(target=handle_client, args=([client_connection]))
        robotnik.start()


if __name__ == '__main__':
    start_server()
