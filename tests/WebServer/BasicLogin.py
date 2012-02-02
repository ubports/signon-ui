import BaseHTTPServer
import cgi

class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        s.end_headers()

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write("""
<html><head><title>Login here</title></head>
<body>
<form method="POST" action="http://localhost:%(port)s/login.html">
  Username: <input type="text" name="username" size="15" /><br />
  Password: <input type="password" name="password" size="15" /><br />
  <p><input type="submit" value="Login" /></p>
</form>
</body>
</html>
""" % { 'port': self.server.server_port })

    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile, 
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                     })
        self.send_response(301)
        self.send_header("Location",
            "http://localhost:%(port)s/logged#%(username)s%(password)s" % {
                'port': self.server.server_port,
                'username': form['username'].value,
                'password': form['password'].value
            })
        self.end_headers()

class Server(BaseHTTPServer.HTTPServer):
    def __init__(self, port=8000):
        BaseHTTPServer.HTTPServer.__init__(self, ('', port), Handler)

if __name__ == '__main__':
    httpd = Server()
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
