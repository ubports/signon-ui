#!/usr/bin/env ruby

require 'webrick'

class Simple < WEBrick::HTTPServlet::AbstractServlet

    def do_GET(request, response)
        status, content_type, body = print_login_form(request)
        response.status = status
        response['Content-Type'] = content_type
        response.body = body
    end

    def print_login_form(request)
        body = '
<html><head><title>Login here</title></head>
<body>
<form method="POST" action="/login.html">
  Username: <input type="text" name="username" size="15" /><br />
  Password: <input type="password" name="password" size="15" /><br />
  <p><input type="submit" value="Login" /></p>
</form>
</body>
</html>
'
        return 200, "text/html", body
    end

    def do_POST(request, response)
        username = request.query['username']
        password = request.query['password']
        if username.empty? or password.empty?
            puts "Missing data"
            status, content_type, body = print_login_form(request)
            response.status = status
            response['Content-Type'] = content_type
            response.body = body
            return
        end

        response.status = 301
        response['Location'] = "/logged\##{username}#{password}"
    end
end

if $0 == __FILE__ then
    server = WEBrick::HTTPServer.new(:Port => 8000)
    server.mount "/", Simple
    trap "INT" do server.shutdown end
    trap "TERM" do server.shutdown end
    server.start
end

