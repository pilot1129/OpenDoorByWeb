var http = require('http');
var fs = require('fs'); // 파일 읽기,쓰기 모듈

function send404Message(response)
{
    response.writeHead(404, {"Content-Type":"text/plain"});

    //응답할 데이터
    response.write("404 ERROR... ");

    //응답 끝
    response.end();
}

//요청을 어떻게 처리할지 작성
function onRequest(request, response)
{
    if(request.method == 'GET' && request.url == '/')
    {
        response.writeHead(200,{"Content-Type":"text/html"});
        fs.createReadStream("./LoginPage.html").pipe(response);
    }
    else
    {
        send404Message(response);
    }
}

//요청처리와 포트 설정
http.createServer(onRequest).listen(8888);
console.log("Server Created...");